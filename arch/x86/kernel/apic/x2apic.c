/* SPDX-License-Identifier: MIT */
#include <davix/panic.h>
#include <davix/printk.h>
#include <davix/mm.h>
#include <asm/apic.h>
#include <asm/page.h>
#include <asm/msr.h>

static u32 x2apic_read(unsigned long offset)
{
	return read_msr(0x800 + (offset >> 4));
}

static void x2apic_write(unsigned long offset, u32 value)
{
	write_msr(0x800 + (offset >> 4), value);
}

static void x2apic_init_other(void)
{
	/*
	 * The rules for APIC state transitions is that:
	 * 1) a direct transition from disabled to x2APIC mode is illegal.
	 * 2) a direct transition from x2APIC mode to xAPIC mode is illegal.
	 *   (Intel SDM)
	 */
	unsigned long val = read_msr(MSR_APIC_BASE);
	if(val & _APIC_BASE_X2APIC_MODE)
		return;

	if(val & _APIC_BASE_ENABLE)
		goto already_xapic;

	write_msr(MSR_APIC_BASE,
		apic_base | _APIC_BASE_ENABLE);
already_xapic:
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

static void x2apic_write_icr(u32 value, u32 dst)
{
	write_msr(0x800 + (APIC_ICR_LOW >> 4), value | ((u64) dst << 32));
}

struct apic x2apic_impl = {
	.init = x2apic_init,
	.init_other = x2apic_init_other,
	.read_id = x2apic_read_id,
	.read = x2apic_read,
	.write = x2apic_write,
	.write_icr = x2apic_write_icr
};
