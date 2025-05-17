/**
 * Local APIC driver.
 * Copyright (C) 2025-present  dbstream
 */
#include <asm/apic_def.h>
#include <asm/apic.h>
#include <asm/asm.h>
#include <asm/cpufeature.h>
#include <asm/kmap_fixed.h>
#include <asm/mmio.h>
#include <asm/msr_bits.h>
#include <davix/printk.h>

static bool apic_is_x2apic;

static inline int
apic_msr (int reg)
{
	return 0x800 + (reg >> 4);
}

static inline volatile void *
apic_reg (int reg)
{
	return (volatile void *) (kmap_fixed_address (KMAP_FIXED_IDX_LOCAL_APIC) + reg);
}

uint32_t
apic_read (int reg)
{
	if (apic_is_x2apic)
		return read_msr (apic_msr (reg));
	else
		return mmio_read32 (apic_reg (reg));
}

void
apic_write (int reg, uint32_t value)
{
	if (apic_is_x2apic)
		write_msr (apic_msr (reg), value);
	else
		mmio_write32 (apic_reg (reg), value);
}

uint32_t
apic_read_id (void)
{
	if (apic_is_x2apic)
		return read_msr (apic_msr (APIC_ID));
	else
		return mmio_read32 (apic_reg (APIC_ID)) >> 24;
}

void
apic_write_icr (uint32_t value, uint32_t target)
{
	if (apic_is_x2apic)
		write_msr (apic_msr (APIC_ICR_LOW), value | ((uint64_t) target << 32));
	else {
		/**
		 * Save and restore the old value of APIC_ICR_HIGH.  This
		 * protects us against any interrupt which may occur in between
		 * the write to APIC_ICR_HIGH and APIC_ICR_LOW in case we
		 * preempted someone who is also issuing an interprocessor
		 * interrupt.
		 */
		uint32_t old = mmio_read32 (apic_reg (APIC_ICR_HIGH));
		mmio_write32 (apic_reg (APIC_ICR_HIGH), target << 24);
		mmio_write32 (apic_reg (APIC_ICR_LOW), value);
		mmio_write32 (apic_reg (APIC_ICR_HIGH), old);
	}
}

void
apic_wait_icr (void)
{
	if (apic_is_x2apic)
		/** x2APIC does not use the delivery status bit.  */
		return;

	while (mmio_read32 (apic_reg (APIC_ICR_LOW)) & APIC_IRQ_PENDING)
		__builtin_ia32_pause ();
}

void
apic_send_IPI (uint32_t value, uint32_t target)
{
	apic_wait_icr ();
	apic_write_icr (value, target);
	apic_wait_icr ();
}

static uintptr_t xAPIC_base;

void
set_xAPIC_base (uintptr_t addr)
{
	xAPIC_base = addr;
}

static void
setup_apic_base_xAPIC (void)
{
	write_msr (MSR_APIC_BASE, xAPIC_base | _APIC_BASE_ENABLED);
}

static void
setup_apic_base_x2APIC (void)
{
	/**
	 * Local APIC mode transitions from no APIC directly to x2APIC are
	 * illegal, and the same with mode transitions from x2APIC "back to"
	 * xAPIC.
	 */
	uint64_t state = read_msr (MSR_APIC_BASE);
	if (!(state & _APIC_BASE_ENABLED))
		write_msr (MSR_APIC_BASE, xAPIC_base | _APIC_BASE_ENABLED);

	write_msr (MSR_APIC_BASE, xAPIC_base | _APIC_BASE_ENABLED | _APIC_BASE_X2APIC);
}

void
apic_init (void)
{
	if (!xAPIC_base) {
		printk (PR_WARN "APIC: no one has set xAPIC_base; using default value.\n");
		set_xAPIC_base (0xfee00000);
	}

	printk (PR_INFO "APIC: xAPIC_base=0x%tx\n", xAPIC_base);

	if (has_feature (FEATURE_X2APIC)) {
		printk (PR_INFO "APIC: using x2APIC mode.\n");
		apic_is_x2apic = 1;
		setup_apic_base_x2APIC ();
	} else {
		printk (PR_INFO "APIC: using xAPIC mode.\n");
		apic_is_x2apic = 0;
		setup_apic_base_xAPIC ();
		kmap_fixed_install (KMAP_FIXED_IDX_LOCAL_APIC,
				make_io_pte (xAPIC_base, pcm_uncached));
	}
}
