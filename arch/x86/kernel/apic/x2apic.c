/* SPDX-License-Identifier: MIT */
#include <davix/panic.h>
#include <davix/printk.h>
#include <davix/mm.h>
#include <asm/apic.h>
#include <asm/page.h>
#include <asm/msr.h>

static inline u32 x2apic_read(unsigned long offset)
{
	return read_msr(0x800 + (offset >> 4));
}

static inline void x2apic_write(unsigned long offset, u32 value)
{
	write_msr(0x800 + (offset >> 4), value);
}

static void x2apic_init_other(void)
{
	/*
	 * For whatever reason, this needs to be done as two separate writes.
	 */
	write_msr(MSR_APIC_BASE,
		apic_base | _APIC_BASE_ENABLE);
	write_msr(MSR_APIC_BASE,
		apic_base | _APIC_BASE_ENABLE | _APIC_BASE_X2APIC_MODE);
}

static void x2apic_init(void)
{
	x2apic_init_other();
}

static unsigned x2apic_read_id(void)
{
	return x2apic_read(APIC_ID);
}

struct apic x2apic_impl = {
	.init = x2apic_init,
	.init_other = x2apic_init_other,
	.read_id = x2apic_read_id
};
