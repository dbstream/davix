/**
 * Local APIC driver.
 * Copyright (C) 2025-present  dbstream
 */
#include <asm/apic_def.h>
#include <asm/apic.h>
#include <asm/asm.h>
#include <asm/cpufeature.h>
#include <asm/interrupt.h>
#include <asm/kmap_fixed.h>
#include <asm/mmio.h>
#include <asm/msr_bits.h>
#include <davix/printk.h>
#include <davix/time.h>

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

void
apic_eoi (void)
{
	apic_write (APIC_EOI, 0);
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

static void
setup_local_apic (void);

static void
calibrate_apic_timer (void);

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

	setup_local_apic ();
	calibrate_apic_timer ();
}

static void
setup_local_apic (void)
{
	/** Soft-disable then soft-enable the APIC.  */
	apic_write (APIC_SPI, 0);
	apic_write (APIC_SPI, VECTOR_SPURIOUS | APIC_SPI_ENABLE | APIC_SPI_FCC_DISABLE);
}

static uint64_t apic_khz;

static void
calibrate_apic_timer (void)
{
	/**
	 * Writing zero to the timer initial count effectively disables the
	 * local APIC timer.
	 */
	apic_write (APIC_TMR_ICR, 0);

	/**
	 * Setup the clock divisor to be 16.
	 */
	apic_write (APIC_TMR_DIV, 3);

	/**
	 * One-shot mode.  Mask the timer.
	 */
	apic_write (APIC_LVTTMR, APIC_IRQ_MASK | VECTOR_APIC_TIMER);

	nsecs_t t0 = ns_since_boot ();
	if (!t0) [[unlikely]] {
		printk (PR_WARN "APIC: no reference timer for calibration available\n");
		apic_khz = 1000000; // guess 1000MHz
		return;
	}

	/**
	 * Start the countdown.
	 */
	apic_write (APIC_TMR_ICR, 0xffffffffU);

	nsecs_t t1, target = t0 + 100UL * 1000UL * 1000UL;  /** 100ms  */
	do {
		__builtin_ia32_pause ();
		t1 = ns_since_boot ();
	} while (t1 < target);
	uint32_t ccr = apic_read (APIC_TMR_CCR);

	/**
	 * Disable the APIC timer.
	 */
	apic_write (APIC_TMR_ICR, 0);

	uint32_t delta_ticks = 0xffffffffU - ccr;
	nsecs_t delta_ns = t1 - t0;

	/**
	 * delta_ticks was measured with a clock divisor of 16.
	 */
	delta_ticks *= 16;

	apic_khz = (1000000UL * delta_ticks) / delta_ns;
	printk (PR_NOTICE "APIC: calibrated the local APIC timer clock frequency to %lu.%03luMHz\n",
			apic_khz / 1000UL, apic_khz % 1000UL);
}

void
apic_start_timer (void)
{
	apic_write (APIC_TMR_ICR, 0);
	apic_write (APIC_TMR_DIV, 3);
	apic_write (APIC_LVTTMR, APIC_TMR_PERIODIC | VECTOR_APIC_TIMER);
	apic_write (APIC_TMR_ICR, (1000 * apic_khz) / (16 * /* frequency: */ 1));
}
