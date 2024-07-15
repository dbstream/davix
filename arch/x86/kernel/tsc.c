/**
 * Timestamp Counter (TSC)
 * Copyright (C) 2024  dbstream
 */
#include <davix/printk.h>
#include <asm/cpulocal.h>
#include <asm/features.h>
#include <asm/hpet.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/tsc.h>

uint64_t tsc_hz;

struct tsc_conv {
	uint64_t khz;
	uint64_t ns_offset;
};

static __CPULOCAL struct tsc_conv tsc_conv;

static inline void
set_tsc_conv (uint64_t tsc, uint64_t ns)
{
	struct tsc_conv conv;
	conv.khz = tsc_hz / 1000UL;

	uint64_t ns2 = (1000000UL * tsc) / conv.khz;
	conv.ns_offset = ns - ns2;

	/**
	 * Currently, this write is not atomic. If we are hit by IRQ or NMI as
	 * we are updating tsc_conv, it can read bogus nanosecond timestamps.
	 * Therefore we want to implement some kind of generation counter, where
	 * we write to a separate tsc_conv and then publish it by incrementing
	 * the generation counter.
	 */
	this_cpu_write (&tsc_conv, conv);
}

static uint64_t
calculate_tsc_hz (uint64_t tsc_delta, uint64_t ns)
{
	tsc_delta *= 1000000UL;
	tsc_delta /= ns;
	tsc_delta *= 1000UL;
	return tsc_delta;
}

/**
 * Get a stable reading of the TSC and the HPET. We read the TSC two times and
 * read the HPET in-between. If the difference between the TSC values is too
 * big, we assume that something happened (NMI, SMM), and retry.
 *
 * Stores the HPET value in *hpet, and returns the TSC value. Zero is returned
 * on failure.
 */
static uint64_t
read_tsc_hpet (uint64_t *hpet)
{
	unsigned long maxdelta = 100000UL;

	for (int i = 0; i < 10; i++) {
		uint64_t tsc1 = rdtsc ();
		*hpet = hpet_read_counter ();
		uint64_t tsc2 = rdtsc ();

		uint64_t delta = tsc2 - tsc1;
		if (delta < maxdelta)
			return tsc2;
	}

	printk (PR_WARN "read_tsc_hpet: calibration failed due to inconsistent readings\n");
	return 0;
}

static void
hpet_calibrate_tsc_early (void)
{
	uint64_t tsc1, tsc2, hpet1, hpet2;

	uint64_t hpet_50ms = (50UL * 1000UL * 1000UL * 1000UL * 1000UL) / hpet_period;

	tsc1 = read_tsc_hpet (&hpet1);
	if (!tsc1)
		return;

	do {
		arch_relax ();
	} while (hpet_read_counter () < (hpet1 + hpet_50ms));

	tsc2 = read_tsc_hpet (&hpet2);
	if (!tsc2)
		return;

	uint64_t tsc_delta = tsc2 - tsc1;
	uint64_t hpet_delta = hpet2 - hpet1;

	uint64_t ns = hpet_delta * hpet_period_ns;
	ns += (hpet_delta * hpet_period_frac) / 1000000UL;

	tsc_hz = calculate_tsc_hz (tsc_delta, ns);
	printk (PR_NOTICE "Fast TSC calibration using HPET: %lu.%03luMHz\n",
		tsc_hz / 1000000UL,
		(tsc_hz / 1000UL) % 1000UL);
}

bool
x86_init_tsc (bool use_hpet)
{
	/**
	 * We currently depend on the HPET for calibration. In the future, we
	 * may support using the PMTimer or the PIT for calibration.
	 */
	if (!use_hpet)
		return false;

	if (!bsp_has (FEATURE_TSC))
		return false;

	hpet_calibrate_tsc_early ();

	if (!tsc_hz)
		return false;

	set_tsc_conv (rdtsc (), 0);
	return true;
}

nsecs_t
x86_tsc_nsecs (void)
{
	bool flag = irq_save ();
	uint64_t tsc = rdtsc ();
	struct tsc_conv conv = this_cpu_read (&tsc_conv);
	irq_restore (flag);

	return conv.ns_offset + ((1000000UL * tsc) / conv.khz);
}
