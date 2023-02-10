/* SPDX-License-Identifier: MIT */
#include <davix/panic.h>
#include <davix/printk.h>
#include <davix/mm.h>
#include <asm/apic.h>
#include <asm/page.h>
#include <asm/msr.h>

static void *xapic_map_address;

static inline u32 xapic_read(unsigned long offset)
{
	return *(volatile u32 *) ((unsigned long) xapic_map_address + offset);
}

static inline void xapic_write(unsigned long offset, u32 value)
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

struct apic xapic_impl = {
	.init = xapic_init,
	.init_other = xapic_init_other,
	.read_id = xapic_read_id
};
