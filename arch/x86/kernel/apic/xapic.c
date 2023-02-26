/* SPDX-License-Identifier: MIT */
#include <davix/panic.h>
#include <davix/printk.h>
#include <davix/mm.h>
#include <asm/apic.h>
#include <asm/page.h>
#include <asm/msr.h>

static void *xapic_map_address;

static u32 xapic_read(unsigned long offset)
{
	return *(volatile u32 *) ((unsigned long) xapic_map_address + offset);
}

static void xapic_write(unsigned long offset, u32 value)
{
	*(volatile u32 *) ((unsigned long) xapic_map_address + offset) = value;
}

static void xapic_init_other(void)
{
	write_msr(MSR_APIC_BASE, apic_base | _APIC_BASE_ENABLE);
}

static void xapic_init(void)
{
	xapic_map_address = vmap(apic_base, PAGE_SIZE,
		PG_READABLE | PG_WRITABLE, PG_UNCACHED);

	if(!xapic_map_address)
		panic("xapic_init(): couldn't vmap() the APIC MMIO region (%p)\n",
			apic_base);

	xapic_init_other();
}

static unsigned xapic_read_id(void)
{
	return xapic_read(APIC_ID) >> 24;
}

static void xapic_write_icr(u32 value, u32 dst)
{
	xapic_write(APIC_ICR_HIGH, dst << 24);
	xapic_write(APIC_ICR_LOW, value);

	/*
	 * Perhaps we should do this in the inverse order? i.e. wait for the
	 * ICR to be idle before writing to it.
	 */
	do {
		relax();
	} while(xapic_read(APIC_ICR_LOW) & (1 << 12));
}

struct apic xapic_impl = {
	.init = xapic_init,
	.init_other = xapic_init_other,
	.read_id = xapic_read_id,
	.read = xapic_read,
	.write = xapic_write,
	.write_icr = xapic_write_icr
};
