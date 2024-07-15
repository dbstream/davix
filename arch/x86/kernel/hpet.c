/**
 * High-Precision Event Timer driver.
 * Copyright (C) 2024  dbstream
 */
#include <acpi/tables.h>
#include <davix/atomic.h>
#include <davix/endian.h>
#include <davix/printk.h>
#include <davix/stdbool.h>
#include <davix/stdint.h>
#include <davix/time.h>
#include <asm/early_memmap.h>
#include <asm/hpet.h>
#include <asm/sections.h>

static unsigned long hpet_virt_addr;

static uint64_t
hpet_read (unsigned long offset)
{
	return *(volatile uint64_t *) (hpet_virt_addr + offset);
}

static void
hpet_write (unsigned long offset, uint64_t value)
{
	*(volatile uint64_t *) (hpet_virt_addr + offset) = value;
}

/* HPET registers */
#define HPET_CAP_ID		0x00
#define HPET_CONF		0x10
#define HPET_IRQ_STATUS		0x20
#define HPET_MAIN_COUNTER	0xf0
#define HPET_TMR_CONF_CAP(n)	(0x100 + 0x20 * (n))
#define HPET_TMR_COMP_VAL(n)	(0x108 + 0x20 * (n))
#define HPET_TMR_FSB_IRQ(n)	(0x110 + 0x20 * (n))

static bool hpet_is_32bit;

/**
 * HPET frequency is calculated from the hpet_period in init_hpet.
 */
uint32_t hpet_period;
static uint64_t hpet_frequency;

/**
 * Divide the hpet_period into nanosecond and fractional part.
 */
uint32_t hpet_period_ns;
uint32_t hpet_period_frac;

/**
 * Emulate a 64-bit HPET when the underlying hardware is 32-bit. We do this by
 * tracking the last read "fake HPET value". On a HPET timer read, we take the
 * binary OR between the real HPET value and the high 32 bits of hpet_last_read.
 * If time "went backwards", we increment hpet_last_read by 2**32.
 */
static uint64_t hpet_last_read;

uint64_t
hpet_read_counter (void)
{
	uint64_t value = hpet_read (HPET_MAIN_COUNTER);
	if (!hpet_is_32bit)
		return value;

	uint64_t last_read = atomic_load_relaxed (&hpet_last_read);
	value &= 0xffffffffUL;
	value |= last_read & ~0xffffffffUL;

	if (value < last_read)
		/**
		 * The HPET value rolled over.
		 */
		value += 1UL << 32;

	if (last_read - value >= 0x1000000UL)
		/**
		 * Don't update hpet_last_read on every counter read,
		 * in order to alleviate the cost of cache bouncing.
		 */
		atomic_cmpxchg_relaxed (&hpet_last_read, &last_read, value);

	return value;
}

__INIT_TEXT
bool
x86_init_hpet (void)
{
	struct acpi_table_hpet *hpet = acpi_find_table (ACPI_SIG_HPET, 0);
	if (!hpet)
		return false;

	printk (PR_INFO "HPET: address_space=%u address=%#lx access_size=%u\n",
		hpet->address.address_space, le64toh (hpet->address.address),
		hpet->address.access_size);

	if (hpet->address.address_space != ACPI_ADDRESS_SPACE_MEMORY) {
		printk (PR_WARN "x86/time: HPET is available but is not ADDRESS_SPACE_MEMORY\n");
		return false;
	}

	hpet_virt_addr = (unsigned long)
		early_memmap (le64toh (hpet->address.address),
			0x1000, PAGE_KERNEL_DATA | PG_UC);
	if (!hpet_virt_addr) {
		printk (PR_ERR "x86/time: Failed to early_memmap the HPET\n");
		return false;
	}

	uint64_t cap_id = hpet_read (HPET_CAP_ID);
	hpet_is_32bit = !(cap_id & (1UL << 13));

	hpet_period = cap_id >> 32;
	if (hpet_period < 100000U || hpet_period > 100000000U) {
		printk (PR_ERR "x86/time: hpet_period is %u femtoseconds, outside of allowed range\n",
			hpet_period);
		early_memunmap ((void *) hpet_virt_addr);
		return false;
	}

	hpet_write (HPET_CONF, 0);
	hpet_write (HPET_MAIN_COUNTER, 0);

	/**
	 * Initialize comparators. Because we never use the comparators for
	 * anything, simply zero HPET_TMR_CONF_CAP for all comparators.
	 */
	for (unsigned int i = 0; i < ((cap_id >> 8) & 31); i++)
		hpet_write (HPET_TMR_CONF_CAP (i), 0);

	hpet_period_ns = hpet_period / 1000000U;
	hpet_period_frac = hpet_period % 1000000U;

	printk ("HPET: period=%u.%06uns bits=%u\n", hpet_period_ns, hpet_period_frac, hpet_is_32bit ? 32 : 64);

	hpet_frequency = 1000000000000000UL / hpet_period;
	printk ("... frequency=%luhz\n", hpet_frequency);

	hpet_write (HPET_CONF, 1);	/* Enable the timer. */
	return true;
}

nsecs_t
x86_hpet_nsecs (void)
{
	uint64_t raw_value = hpet_read_counter ();
	uint64_t ns = hpet_period_ns * raw_value;
	ns += (hpet_period_frac * raw_value) / 1000000UL;
	return ns;
}
