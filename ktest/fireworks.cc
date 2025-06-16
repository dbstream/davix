/**
 * Fireworks test.
 * Copyright (C) 2025-present  dbstream
 *
 * This file implements the "fireworks test", which is a stress test for the
 * scheduler.  The test entry point starts a number of threads, where each
 * thread simulates a "particle" on the framebuffer.
 *
 * This fireworks test is inspired by a similar test written by iProgramInCpp.
 */
#include <davix/fbcon.h>
#include <davix/fbcon_internal.h>
#include <davix/kmalloc.h>
#include <davix/kthread.h>
#include <davix/printk.h>
#include <davix/sched.h>
#include <davix/spinlock.h>

static struct fbcon *fwork_fb;

static uint32_t width, height;

static spinlock_t flush_lock;
static uint32_t flush_sometimes = 0;

static void
plot (int32_t x, int32_t y, uint32_t color, bool flush)
{
	if (x < 0 || y < 0)
		return;
	uint32_t x_ = x;
	uint32_t y_ = y;
	if (x_ >= width || y_ >= height)
		return;

	uint8_t *pixel = fbcon_get_pixel (fwork_fb, x_, y_);
	fbcon_put_pixel (fwork_fb, pixel, color);

	if (!flush)
		return;

	if ((atomic_fetch_inc (&flush_sometimes, mo_relaxed) & 0x7f) != 0)
		return;

	disable_dpc ();
	if (flush_lock.raw_trylock ()) {
		fbcon_flush (fwork_fb);
		flush_lock.raw_unlock ();
	}
	enable_dpc ();
}

static inline uint64_t
rol64 (uint64_t x, int k)
{
	return (x << k) | (x >> (64 - k));
}

struct xoshiro256pp_state {
	uint64_t s[4];
};

static uint64_t
xoshiro256pp (xoshiro256pp_state &state)
{
	uint64_t result = rol64(state.s[0] + state.s[3], 23) + state.s[0];
	uint64_t t = state.s[1] << 17;

	state.s[2] ^= state.s[0];
	state.s[3] ^= state.s[1];
	state.s[1] ^= state.s[2];
	state.s[0] ^= state.s[3];

	state.s[2] ^= t;
	state.s[3] = rol64 (state.s[3], 45);

	return result;
}

static void
seed_rng (xoshiro256pp_state &state, uint64_t seed)
{
	for (int i = 0; i < 4; i++) {
		state.s[i] = seed;
		seed *= 0xdeadbeef87654321;
	}
}

static uint32_t
random_color (xoshiro256pp_state &state)
{
	uint32_t r = 128 + (xoshiro256pp (state) % 128);
	uint32_t g = 128 + (xoshiro256pp (state) % 128);
	uint32_t b = 128 + (xoshiro256pp (state) % 128);

	return fbcon_get_color (fwork_fb, r, g, b);
}

static void
poor_mans_sleep (nsecs_t ns)
{
	nsecs_t target = ns_since_boot () + ns;
	do {
		schedule ();
	} while (ns_since_boot () < target);
}

struct fixed48_16_t {
	int64_t raw_value;

	constexpr fixed48_16_t (void) = default;

	constexpr fixed48_16_t (int32_t value)
	{
		raw_value = (int64_t) value * 65536;
	}

	constexpr fixed48_16_t (int64_t value, int dummy)
	{
		(void) dummy;
		raw_value = value;
	}

	constexpr operator int32_t (void) const
	{
		return raw_value / 65536;
	}

	constexpr fixed48_16_t
	operator+ (fixed48_16_t other) const
	{
		fixed48_16_t ret;
		ret.raw_value = raw_value + other.raw_value;
		return ret;
	}

	constexpr fixed48_16_t
	operator- (fixed48_16_t other) const
	{
		fixed48_16_t ret;
		ret.raw_value = raw_value - other.raw_value;
		return ret;
	}

	constexpr fixed48_16_t
	operator* (fixed48_16_t other) const
	{
		fixed48_16_t ret;
		ret.raw_value = (raw_value * other.raw_value) / 65536;
		return ret;
	}

	constexpr fixed48_16_t &
	operator= (int32_t value)
	{
		raw_value = (int64_t) value * 65536;
		return *this;
	}
};

static inline fixed48_16_t
rand_fixed48_16 (xoshiro256pp_state &rng)
{
	fixed48_16_t ret;
	ret.raw_value = xoshiro256pp (rng) % 65536;
	return ret;
}

static inline fixed48_16_t
rand_fixed48_16_biased (xoshiro256pp_state &rng)
{
	fixed48_16_t ret;
	int64_t a = xoshiro256pp (rng) % 65536;
	int64_t b = xoshiro256pp (rng) % 65536;
	if (a < b) a = b;
	ret.raw_value = a;
	return ret;
}

static inline fixed48_16_t
rand_fixed48_16_sign (xoshiro256pp_state &rng)
{
	if (xoshiro256pp (rng) & 1)
		return rand_fixed48_16 (rng);
	else
		return fixed48_16_t(-1) * rand_fixed48_16 (rng);
}

static inline int64_t
cos_p (int64_t x)
{
	bool b = false;
	if (x > 102943) {
		x = 205886 - x;
		b = true;
	}
	int64_t x2_2 = (x * x) / 131072;
	int64_t x4_24 = (x2_2 * x2_2) / 393216;
	int64_t v = 65536 - x2_2 + x4_24;
	if (b)
		v = -v;
	return v;
}

static inline fixed48_16_t
cos (fixed48_16_t value)
{
	int64_t x = value.raw_value;
	if (x < 0)
		x = -x;

	x = x % 411772;
	bool b = false;
	if (x > 205887) {
		x -= 205887;
		b = true;
	}

	int64_t v = cos_p (x);
	if (b)
		v = -v;

	fixed48_16_t ret;
	ret.raw_value = v;
	return ret;
}

static inline fixed48_16_t
sin (fixed48_16_t value)
{
	fixed48_16_t offset;
	offset.raw_value = 308831;
	return cos (value + offset);
}

static constexpr fixed48_16_t dt(1048, 0);

struct firework_data {
	uint32_t x, y;
	fixed48_16_t ax, ay;
	fixed48_16_t vx, vy;
	uint32_t color;
	fixed48_16_t range;
	uint64_t rngseed;
};

static void
particle_func (void *arg)
{
	xoshiro256pp_state rng;
	firework_data data;

	firework_data *pdata = (firework_data *) arg;
	data.x = pdata->x;
	data.y = pdata->y;
	data.ax = pdata->ax;
	data.ay = pdata->ay;
	fixed48_16_t range = pdata->range;
	seed_rng (rng, pdata->rngseed);
	kfree (pdata);

	range = range * rand_fixed48_16_biased (rng);

	fixed48_16_t angle;
	angle.raw_value = xoshiro256pp (rng) % 411774;
	data.vx = cos (angle) * range;
	data.vy = sin (angle) * range;

	data.color = random_color (rng);
	int expire_in = 2000 + (xoshiro256pp (rng) % 1000);

	for (int i = 0; i < expire_in; ) {
		plot (data.x, data.y, data.color, true);

		poor_mans_sleep (16666000ULL);
		i += 16;

		plot (data.x, data.y, 0, false);
		data.ax = data.ax + (data.vx * dt);
		data.ay = data.ay + (data.vy * dt);

		data.x = data.ax;
		data.y = data.ay;

		data.vy = data.vy + (fixed48_16_t (10) * dt);
	}

	kthread_exit ();
}

static void
spawn_particle (firework_data *data)
{
	Task *task = kthread_create ("fwork-particle", particle_func, data);
	if (task)
		kthread_start (task);
	else
		kfree (data);
}

static void
explodeable_func (void *arg)
{
	xoshiro256pp_state rng;
	firework_data data;

	seed_rng (rng, (uintptr_t) arg);

	int32_t offset = (width * 400) / 1024;

	data.x = width / 2;
	data.y = height - 1;
	data.ax = data.x;
	data.ay = data.y;
	data.vx = fixed48_16_t(offset) * rand_fixed48_16_sign (rng);
	data.vy = -400 - (xoshiro256pp (rng) % 400);
	data.color = random_color (rng);
	data.range = 100 + (xoshiro256pp (rng) % 100);

	int expire_in = 500 + (xoshiro256pp (rng) % 500);
	for (int i = 0; i < expire_in; ) {
		plot (data.x, data.y, data.color, true);
		poor_mans_sleep (16666000ULL);
		i += 16;

		plot (data.x, data.y, 0, false);

		data.ax = data.ax + (data.vx * dt);
		data.ay = data.ay + (data.vy * dt);

		data.x = data.ax;
		data.y = data.ay;

		data.vy = data.vy + (fixed48_16_t (10) * dt);
	}

	int particle_count = 100 + (xoshiro256pp (rng) % 100);
	for (int i = 0; i < particle_count; i++) {
		firework_data *pdata = (firework_data *)
			kmalloc (sizeof (*pdata), ALLOC_KERNEL);

		if (!pdata)
			break;

		*pdata = data;
		pdata->rngseed = xoshiro256pp (rng);
		spawn_particle (pdata);
	}

	kthread_exit ();
}

static void
spawn_explodeable (uint64_t seed)
{
	Task *task = kthread_create ("fwork-explosive", explodeable_func,
			(void *) (uintptr_t) seed);

	if (task)
		kthread_start (task);
}

static void
spawner_thread (void *arg)
{
	(void) arg;

	xoshiro256pp_state rng;
	seed_rng (rng, ns_since_boot ());

	for (;;) {
		int spawn_count = xoshiro256pp (rng) % 5 + 1;
		for (int i = 0; i < spawn_count; i++)
			spawn_explodeable (xoshiro256pp (rng));

		poor_mans_sleep (2000000000 + (xoshiro256pp (rng) % 2000000000));
	}
}

void
ktest_fireworks (void)
{
	fwork_fb = fbcon_find_one ();
	if (!fwork_fb) {
		printk (PR_INFO "fireworks: No framebuffer was found; skipping fireworks test\n");
		return;
	}

	width = fwork_fb->width;
	height = fwork_fb->height;

	printk (PR_NOTICE "Running fireworks test on a %ux%u display...\n",
			width, height);

	Task *task = kthread_create ("fwork-spawner", spawner_thread, nullptr);
	if (task)
		kthread_start (task);
	else
		printk (PR_ERROR "Failed to start fireworks test!\n");
}
