/**
 * Kernel clocksource.
 * Copyright (C) 2025-present  dbstream
 */
#include <asm/asm.h>
#include <asm/cpufeature.h>
#include <asm/io.h>
#include <asm/kmap_fixed.h>
#include <asm/mmio.h>
#include <asm/percpu.h>
#include <asm/smp.h>
#include <asm/time.h>
#include <davix/atomic.h>
#include <davix/irql.h>
#include <davix/panic.h>
#include <davix/printk.h>
#include <davix/time.h>
#include <uacpi/acpi.h>
#include <uacpi/tables.h>

static bool use_tsc = false;
static bool use_hpet = false;

static volatile void *hpet_regs;

static uint64_t
hpet_read (uintptr_t offset)
{
	return mmio_read64 (mmio_ptr_offset (hpet_regs, offset));
}

static void
hpet_write (uintptr_t offset, uint64_t value)
{
	mmio_write64 (mmio_ptr_offset (hpet_regs, offset), value);
}

/** HPET registers */
enum : uintptr_t {
	HPET_CAP_ID		= 0x00,
	HPET_CONF		= 0x10,
	HPET_IRQ_STATUS		= 0x20,
	HPET_MAIN_COUNTER	= 0xf0
};

constexpr static inline uintptr_t
HPET_TMR_CONF_CAP (int n)
{
	return 0x100 + 0x20 * n;
}

constexpr static inline uintptr_t
HPET_TMR_COMP_VAL (int n)
{
	return 0x108 + 0x20 * n;
}

constexpr static inline uintptr_t
HPET_TMR_FSB_IRQ (int n)
{
	return 0x110 + 0x20 * n;
}

static bool hpet_is_32bit;

/**
 * HPET frequency is calculated from the hpet_period in init_hpet.
 */
static uint32_t hpet_period;
static uint64_t hpet_frequency;

/**
 * Divide the hpet_period into nanosecond and fractional part.
 */
static uint32_t hpet_period_ns;
static uint32_t hpet_period_frac;

/**
 * Emulate a 64-bit HPET when the underlying hardware is 32-bit.  We do this by
 * tracking the last read "augmented" HPET value.  On a HPET timer read, we take
 * the bitwise union of the real HPET value and the high bits of hpet_last_read.
 * If time "went backwards", we increment hpet_last_read by 2^32.
 */
static uint64_t hpet_last_read;
constexpr uint64_t hpet_32bit_mask = 0xffffffffUL;

static uint64_t
hpet_read_counter (void)
{
	uint64_t value = hpet_read (HPET_MAIN_COUNTER);
	if (!hpet_is_32bit)
		return value;

	uint64_t last_read = atomic_load_relaxed (&hpet_last_read);
	value &= hpet_32bit_mask;
	value |= last_read & ~hpet_32bit_mask;

	if (value < last_read)
		/**
		 * Time went backwards, which means the HPET main counter rolled
		 * over.
		 */
		value += 1UL << 32;

	/**
	 * We don't have to update hpet_last_read on _every_ read of the main
	 * counter.  Do it when we are half the way to rolling over.
	 */
	if (last_read - value > (hpet_32bit_mask >> 1))
		atomic_cmpxchg (&hpet_last_read, &last_read, value,
				mo_relaxed, mo_relaxed);

	return value;
}

static void
init_hpet (void)
{
	uacpi_table hpet_table;
	uacpi_status status = uacpi_table_find_by_signature (ACPI_HPET_SIGNATURE, &hpet_table);
	if (status == UACPI_STATUS_NOT_FOUND)
		return;
	else if (status != UACPI_STATUS_OK) {
		printk (PR_ERROR "HPET: uacpi_table_find_by_signature(\"HPET\") returned %s",
				uacpi_status_to_string (status));
		return;
	}

	acpi_hpet *hpet = (acpi_hpet *) hpet_table.ptr;
	uint8_t asid = hpet->address.address_space_id;
	uintptr_t addr = hpet->address.address;
	uacpi_table_unref (&hpet_table);

	if (asid != ACPI_AS_ID_SYS_MEM) {
		printk (PR_WARN "HPET: not in SYS_MEM; ignoring\n");
		return;
	}

	hpet_regs = kmap_fixed (addr, PAGE_SIZE, make_io_pteval (pcm_uncached));
	if (!hpet_regs) {
		printk (PR_ERROR "HPET: could not map register I/O window\n");
		return;
	}

	uint64_t cap_id = hpet_read (HPET_CAP_ID);
	hpet_is_32bit = !(cap_id & (11UL << 13));

	hpet_period = cap_id >> 32;
	int num_comparators = ((cap_id >> 8) & 31) + 1;

	hpet_write (HPET_CONF, 0);
	hpet_write (HPET_MAIN_COUNTER, 0);

	/**
	 * Initialize comparators.  We do not use the comparators for anything,
	 * so we can simply zero them all.
	 */
	for (int i = 0; i < num_comparators; i++)
		hpet_write (HPET_TMR_CONF_CAP (i), 0);

	hpet_period_ns = hpet_period / 1000000U;
	hpet_period_frac = hpet_period % 1000000U;

	printk (PR_INFO "HPET: period=%u.%06uns  bits=%u\n",
			hpet_period_ns, hpet_period_frac,
			hpet_is_32bit ? 32 : 64);
	if (hpet_period < 100000U || hpet_period > 100000000U) {
		printk (PR_WARN "HPET: period is %u femtoseconds, which is unsane; ignoring\n",
				hpet_period);
		kunmap_fixed ((void *) hpet_regs);
		return;
	}

	hpet_frequency = 1000000000000000UL / hpet_period;
	printk (".. frequency=%luMHz\n", hpet_frequency / 1000000);

	hpet_write (HPET_CONF, 1);
	use_hpet = true;
}

/**
 * Convert the value from a HPET reading to nanoseconds.
 */
static nsecs_t
hpet_conv_nsecs (uint64_t value)
{
	uint64_t ns = hpet_period_ns * value;
	ns += (hpet_period_frac * value) / 1000000;
	return ns;
}

static nsecs_t
hpet_nsecs (void)
{
	return hpet_conv_nsecs (hpet_read_counter ());
}

static uint64_t tsc_khz = 0;

/**
 * tsc->ns conversion: ns = 10^6 tsc/khz + offset
 */
struct tscconv {
	uint64_t khz;
	uint64_t offset;
};

static inline nsecs_t
tsc2ns (uint64_t tscval, struct tscconv tscconv)
{
	return ((1000000UL * tscval) / tscconv.khz) + tscconv.offset;
}

struct tsc_pcpu {
	struct tscconv conv[2];
	uint64_t generation;
};

static DEFINE_PERCPU (struct tsc_pcpu, tsc_pcpu);

static void
write_tscconv (struct tscconv conv)
{
	uint64_t nextgen = percpu_read (tsc_pcpu.generation) + 1;
	percpu_write (tsc_pcpu.conv[nextgen % 2], conv);
	barrier ();
	percpu_write (tsc_pcpu.generation, nextgen);
}

static uint64_t
read_tsc_tscconv (struct tscconv *tscconv)
{
	uint64_t tsc;
	uint64_t gen1, gen2 = percpu_read (tsc_pcpu.generation);
	do {
		barrier ();
		gen1 = gen2;
		tsc = rdtsc ();
		*tscconv = percpu_read (tsc_pcpu.conv[gen1 % 2]);
		barrier ();
		gen2 = percpu_read (tsc_pcpu.generation);
	} while (gen2 != gen1);
	return tsc;
}

/**
 * Update the tscconv struct on this CPU.
 */
static void
set_tsc_conv (uint64_t tsc, nsecs_t ns)
{
	struct tscconv tscconv;
	tscconv.khz = tsc_khz;
	tscconv.offset = 0;

	nsecs_t ns2 = tsc2ns (tsc, tscconv);
	tscconv.offset = ns - ns2;

	write_tscconv (tscconv);
}

static nsecs_t
tsc_nsecs (void)
{
	uint64_t tsc;
	struct tscconv tscconv;

	{
		/**
		 * read_tsc_tscconv is safe against preemption but not migration
		 */
		scoped_dpc g;
		tsc = read_tsc_tscconv (&tscconv);
	}

	return tsc2ns (tsc, tscconv);
}

/**
 * Get a stable reading of the TSC and the HPET.  We read the TSC two times and
 * read the HPET in-between.  If the difference between the TSC values is too
 * big, we assume something happened (NMI, SMI) and retry.
 */
static uint64_t
read_tsc_ref (uint64_t *ref)
{
	constexpr uint64_t maxdelta = 100000UL;
	constexpr uint64_t gooddelta = 1000UL;

	uint64_t besttsc, bestref, bestdelta = -1UL;
	for (int i = 0; i < 10; i++) {
		uint64_t tsc1 = rdtsc_strong ();
		uint64_t ref = hpet_read_counter ();
		uint64_t tsc2 = rdtsc_strong ();

		uint64_t delta = tsc2 - tsc1;
		if (delta < bestdelta) {
			besttsc = tsc2;
			bestref = ref;
			bestdelta = delta;
		}
		if (bestdelta < gooddelta)
			break;
	}

	if (bestdelta > maxdelta) {
		printk (PR_WARN "read_tsc_ref: too big TSC delta; calibration failed\n");
		return 0;
	}

	*ref = bestref;
	return besttsc;
}

/**
 * Convert the reference value from read_tsc_ref into nanoseconds.
 */
static nsecs_t
tsc_ref_to_nsecs (uint64_t ref)
{
	return hpet_conv_nsecs (ref);
}

/**
 * Spin for 50ms using the reference timer.
 */
static void
tsc_ref_mdelay (uint64_t ref, msecs_t ms)
{
	uint64_t delta = (ms * 1000UL * 1000UL * 1000UL * 1000UL) / hpet_period;
	uint64_t target = ref + delta;

	do {
		__builtin_ia32_pause ();
	} while (hpet_read_counter () < target);
}

/**
 * Calculate the time-stamp counter frequency against a nanosecond delta.
 */
static uint64_t
calculate_tsc_khz (uint64_t tsc_delta, nsecs_t ref_delta)
{
	tsc_delta *= 1000000UL;
	tsc_delta /= ref_delta;
	return tsc_delta;
}

static void
tsc_calibrate_early (void)
{
	uint64_t tsc1, tsc2, ref1, ref2;

	tsc1 = read_tsc_ref (&ref1);
	if (!tsc1)
		return;

	tsc_ref_mdelay (ref1, 50);
	tsc2 = read_tsc_ref (&ref2);
	if (!tsc2)
		return;

	uint64_t tsc_delta = tsc2 - tsc1;
	uint64_t ref_delta = ref2 - ref1;

	nsecs_t ns = tsc_ref_to_nsecs (ref_delta);
	if (ns < 50UL * 1000UL * 1000UL) {
		printk (PR_WARN "tsc_calibrate_early: failed to wait for 50 nanoseconds\n");
		return;
	}

	tsc_khz = calculate_tsc_khz (tsc_delta, ns);
	printk ("Early TSC calibration using HPET: %lu.%03luMHz\n",
			tsc_khz / 1000UL, tsc_khz % 1000UL);
	if (tsc_khz < 1000UL) {
		printk (PR_WARN "tsc_calibrate_early: TSC is unreasonably slow; disabling TSC\n");
		tsc_khz = 0;
		return;
	}

	set_tsc_conv (tsc2, tsc_ref_to_nsecs (ref2));
}

void
x86_init_time (void)
{
	init_hpet ();
	if (!use_hpet)
		/**
		 * We need the HPET for TSC calibration.
		 */
		panic ("HPET not usable!");
	else
		printk (PR_INFO "Switched to HPET clock source.\n");

	if (has_feature (FEATURE_TSC)) {
		bool flag = raw_irq_save ();
		tsc_calibrate_early ();
		raw_irq_restore (flag);

		if (tsc_khz) {
			use_tsc = true;
			printk (PR_INFO "Switched to TSC clock source.\n");
		}
	}
}

nsecs_t
ns_since_boot (void)
{
	if (use_tsc)
		return tsc_nsecs ();
	if (use_hpet)
		return hpet_nsecs ();

	return 0;
}

void
ndelay (nsecs_t ns)
{
	if (!use_tsc && !use_hpet)
		return;

	nsecs_t target = ns_since_boot () + ns;
	do {
		barrier ();
		__builtin_ia32_pause ();
	} while (ns_since_boot () < target);
}

enum {
	TSC_SYNC_WAIT_FOR_VICTIM = 0,
	TSC_SYNC_WAIT_FOR_CONTROL,
	TSC_SYNC_CONTROL_READY,
	TSC_SYNC_VICTIM_READY,
	TSC_SYNC_DONE
};

static struct {
	int value = TSC_SYNC_WAIT_FOR_VICTIM;
	int pad[31];
} tsc_sync_timeline alignas(128);
static nsecs_t tsc_sync_ns;
static uint64_t tsc_sync_value;

void
x86_synchronize_tsc_control (void)
{
	if (!has_feature (FEATURE_TSC))
		return;

	bool flag = raw_irq_save ();

	tscconv conv;
	read_tsc_tscconv (&conv);

	while (atomic_load_relaxed (&tsc_sync_timeline.value) != TSC_SYNC_WAIT_FOR_CONTROL)
		__builtin_ia32_pause ();

	atomic_store_relaxed (&tsc_sync_timeline.value, TSC_SYNC_CONTROL_READY);
	uint64_t tsc = rdtsc_strong ();
	tsc_sync_value = tsc;
	tsc_sync_ns = tsc2ns (tsc, conv);

	while (atomic_load_relaxed (&tsc_sync_timeline.value) != TSC_SYNC_VICTIM_READY)
		__builtin_ia32_pause ();

	atomic_store_release (&tsc_sync_timeline.value, TSC_SYNC_DONE);
	raw_irq_restore (flag);

	while (atomic_load_acquire (&tsc_sync_timeline.value) != TSC_SYNC_WAIT_FOR_VICTIM)
		__builtin_ia32_pause ();
}

static DEFINE_PERCPU(uint64_t, tsc_sync_delta);

void
x86_synchronize_tsc_victim (void)
{
	if (!has_feature (FEATURE_TSC))
		return;

	atomic_store_relaxed (&tsc_sync_timeline.value, TSC_SYNC_WAIT_FOR_CONTROL);
	while (atomic_load_relaxed (&tsc_sync_timeline.value) != TSC_SYNC_CONTROL_READY)
		__builtin_ia32_pause ();

	uint64_t tsc = rdtsc_strong ();
	atomic_store_relaxed (&tsc_sync_timeline.value, TSC_SYNC_VICTIM_READY);

	while (atomic_load_acquire (&tsc_sync_timeline.value) != TSC_SYNC_DONE)
		__builtin_ia32_pause ();

	uint64_t control_tsc = tsc_sync_value;
	set_tsc_conv (tsc, tsc_sync_ns);
	atomic_store_release (&tsc_sync_timeline.value, TSC_SYNC_WAIT_FOR_VICTIM);

	percpu_write (tsc_sync_delta, tsc - control_tsc);
}

void
tsc_sync_dump (void)
{
	if (!has_feature (FEATURE_TSC))
		return;

	printk (PR_INFO "TSC: sync delta for CPU%u: %ld\n",
			this_cpu_id (), (int64_t) percpu_read (tsc_sync_delta));
}
