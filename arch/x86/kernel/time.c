/* SPDX-License-Identifier: MIT */
#include <davix/acpi.h>
#include <davix/time.h>
#include <davix/printk.h>
#include <davix/panic.h>

/*
 * Currently, only HPET is supported.
 *
 * Why HPET?
 * Because,
 * 1) The HPET is very simple to get started with. Read some values from ACPI
 *    tables, like the HPET speed, and you're already up and running.
 * 2) The HPET is present in almost all modern systems, and is therefore a good
 *    first time source for the kernel.
 */

static bool time_inited = 0;

static volatile u64 *hpet_address = NULL;

static unsigned long hpet_period; /* HPET timer tick period in femtoseconds */

static unsigned long hpet_div1;
static unsigned long hpet_mul;
static unsigned long hpet_div2;

static inline u64 hpet_read(unsigned long offset)
{
	return hpet_address[offset / 8];
}

static inline void hpet_write(unsigned long offset, u64 value)
{
	hpet_address[offset / 8] = value;
}

nsecs_t ns_since_boot(void)
{
	if(!time_inited)
		return 0;

	unsigned long hpet_value = hpet_read(0xf0);

	/*
	 * We basically return (hpet_value * hpet_period) / 1000000, but we need
	 * to be careful so we don't overflow when we multiply hpet_value and
	 * hpet_period.
	 */
	return (nsecs_t) (((hpet_value / hpet_div1) * hpet_mul) / hpet_div2);
}

void ndelay(nsecs_t ns)
{
	if(!time_inited)
		return;

	nsecs_t then = ns_since_boot() + ns;
	do {
		relax();
	} while(ns_since_boot() < then);
}

void time_init(void)
{
	struct acpi_table_hpet *hpet;
	acpi_get_table(ACPI_SIG_HPET, 0, (struct acpi_table_header **) &hpet);

	if(!hpet)
		panic("time: HPET not present!");

	if(hpet->address.space_id != ACPI_ADR_SPACE_SYSTEM_MEMORY)
		panic("time: HPET address space: %u, which is not supported.",
			hpet->address.space_id);

	hpet_address = (volatile u64 *)
		acpi_os_map_memory(hpet->address.address, PAGE_SIZE);
	if(!hpet_address)
		panic("time: couldn't vmap() the HPET.");

	u64 cap_id_register = hpet_read(0);

	if(!(cap_id_register & (1 << 13)))
		panic("time: old, 32-bit HPET, not supported.");

	hpet_period = cap_id_register >> 32; /* in femtoseconds */
	info("time: HPET period %lu.%06luns\n",
		hpet_period / 1000000, (hpet_period % 1000000));

	/*
	 * Depending on the HPET frequency, we divide & multiply in different
	 * ways in ns_since_boot().
	 */
	if(hpet_period >= 50000) {	/* >= 0.05ns */
		hpet_div1 = 1;
		hpet_mul = hpet_period / 1000;
		hpet_div2 = 1000;
	} else {			/* <0.05ns */
		hpet_div1 = 1000;
		hpet_mul = hpet_period;
		hpet_div2 = 1000;
	}

	/* Disable the main counter. */
	hpet_write(0x10, 0);

	/*
	 * Disable all HPET comparators, as we will not use them.
	 */
	unsigned int ntimers = (cap_id_register >> 8) & 0x1f;
	for(unsigned int i = 0; i < ntimers; i++)
		hpet_write(0x100 + 0x20 * i, 0);

	/* Reset the main counter and start it. */
	hpet_write(0xf0, 1);
	hpet_write(0x10, 1);

	time_inited = 1;
}
