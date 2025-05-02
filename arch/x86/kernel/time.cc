/**
 * Kernel clocksource.
 * Copyright (C) 2025-present  dbstream
 */
#include <asm/io.h>
#include <asm/kmap_fixed.h>
#include <asm/mmio.h>
#include <asm/time.h>
#include <davix/atomic.h>
#include <davix/printk.h>
#include <davix/time.h>
#include <uacpi/acpi.h>
#include <uacpi/tables.h>

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

static uint64_t
hpet_nsecs (void)
{
	uint64_t raw_value = hpet_read_counter ();
	uint64_t ns = hpet_period_ns * raw_value;
	ns += (hpet_period_frac * raw_value) / 1000000;
	return ns;
}

void
x86_init_time (void)
{
	init_hpet ();
}

nsecs_t
ns_since_boot (void)
{
	if (use_hpet)
		return hpet_nsecs ();

	return 0;
}
