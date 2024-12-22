/**
 * xAPIC and x2APIC (Advanced Programmable Interrupt Controller) support.
 * Copyright (C) 2024  dbstream
 */
#include <davix/printk.h>
#include <davix/time.h>
#include <asm/acpi.h>
#include <asm/apic.h>
#include <asm/early_memmap.h>
#include <asm/features.h>
#include <asm/interrupt.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/msr.h>
#include <asm/sections.h>
#include <asm/smp.h>

/**
 * In some cases, the 8259 PIC can spook around and generate unwanted IRQs
 * that conflict with the 32 architecturally-reserved vectors. This function
 * masks and remaps the PIC so that it won't be a problem.
 *
 * Remap all interrupts from both PICs (IRQs 0..15) to vectors 32..39,
 * overlapping. This overlap does not matter as we don't use the PIC.
 */
__INIT_TEXT
static void
remap_PIC (void)
{
	io_outb (0x20, 0x11);
	io_outb (0x80, 0);
	io_outb (0xa0, 0x11);
	io_outb (0x80, 0);
	io_outb (0x21, 32);	/* PIC1 offset */
	io_outb (0x80, 0);
	io_outb (0xa1, 32);	/* PIC2 offset */
	io_outb (0x80, 0);
	io_outb (0x21, 0x04);
	io_outb (0x80, 0);
	io_outb (0xa1, 0x02);
	io_outb (0x80, 0);
	io_outb (0x21, 0x01);
	io_outb (0x80, 0);
	io_outb (0xa1, 0x01);
	io_outb (0x80, 0);

	io_outb (0x21, 0xff);	/* mask all IRQs on PIC1 */
	io_outb (0xa1, 0xff);	/* mask all IRQs on PIC2 */
	io_outb (0x80, 0);
}

int apic_is_x2apic;

uint32_t cpu_to_apicid[CONFIG_MAX_NR_CPUS];
uint32_t cpu_to_acpi_uid[CONFIG_MAX_NR_CPUS];

#define APIC_MSR(offset) (0x800 + ((offset) >> 4))
#define APIC_REG(offset) (*(volatile uint32_t *) (MAPPED_APIC_BASE + (offset)))

uint32_t
apic_read (uint32_t offset)
{
	if (apic_is_x2apic)
		return read_msr (APIC_MSR (offset));
	else
		return APIC_REG (offset);
}

void
apic_write (uint32_t offset, uint32_t value)
{
	if (apic_is_x2apic)
		write_msr (APIC_MSR (offset), value);
	else
		APIC_REG (offset) = value;
}

uint32_t
apic_read_id (void)
{
	if (apic_is_x2apic)
		return read_msr (APIC_MSR (APIC_ID));
	else
		return APIC_REG (APIC_ID) >> 24;
}

void
apic_write_icr (uint32_t value, uint32_t target)
{
	if (apic_is_x2apic)
		write_msr (APIC_MSR (APIC_ICR_LOW), value | ((uint64_t) target << 32));
	else {
		/**
		 * Save and restore the old value of APIC_ICR_HIGH. This
		 * protects against interrupts occuring after APIC_ICR_HIGH
		 * is written but before APIC_ICR_LOW is written.
		 */
		uint32_t old = APIC_REG (APIC_ICR_HIGH);
		APIC_REG (APIC_ICR_HIGH) = target << 24;
		APIC_REG (APIC_ICR_LOW) = value;
		APIC_REG (APIC_ICR_HIGH) = old;
	}
}

void
apic_wait_icr (void)
{
	if (apic_is_x2apic)
		/* x2APIC does not use the delivery status bit */
		return;

	while (APIC_REG (APIC_ICR_LOW) & APIC_IRQ_PENDING)
		arch_relax ();
}

/**
 * Send an interprocessor interrupt by writing target:value to the ICR. Also
 * wait on the ICR before and after sending the interrupt.
 */
void
apic_send_IPI (uint32_t value, uint32_t target)
{
	apic_wait_icr ();
	apic_write_icr (value, target);
	apic_wait_icr ();
}

static unsigned long xAPIC_base;

__INIT_TEXT
void
set_xAPIC_base (unsigned long addr)
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
	/*
	 * Transitions directly from no-APIC to x2APIC are illegal, as are any
	 * transitions from x2APIC to xAPIC. It is possible for the bootloader
	 * or firmware to have left us with any one of these configurations, so
	 * detect which.
	 */
	unsigned long state = read_msr (MSR_APIC_BASE);
	if (!(state & _APIC_BASE_ENABLED))
		write_msr (MSR_APIC_BASE, xAPIC_base | _APIC_BASE_ENABLED);

	write_msr (MSR_APIC_BASE,
		xAPIC_base | _APIC_BASE_ENABLED | _APIC_BASE_X2APIC);
}

/**
 * Perform early APIC initializations.
 */
__INIT_TEXT
void
early_apic_init (void)
{
	if (!xAPIC_base) {
		printk (PR_WARN "apic: No one has set xAPIC_base, using default value.\n");
		set_xAPIC_base (0xfee00000);
	}

	printk (PR_INFO "apic: xAPIC_base=%#lx\n", xAPIC_base);

	/* Disable the 8259 PIC. */
	remap_PIC ();

	if (bsp_has (FEATURE_X2APIC)) {
		printk (PR_INFO "apic: Using x2APIC mode.\n");
		apic_is_x2apic = 1;
		setup_apic_base_x2APIC ();
	} else {
		printk (PR_INFO "apic: Using xAPIC mode.\n");
		early_memmap_fixed (IDX_APIC_BASE,
			xAPIC_base | PAGE_KERNEL_DATA | PG_UC);
		setup_apic_base_xAPIC ();
	}
}

static void
acpi_madt_nmi (uint32_t acpi_uid, int lint, uint16_t flags)
{
	if (acpi_uid != cpu_to_acpi_uid[this_cpu_id ()] && acpi_uid != -1U)
		return;

	if (lint < 0 || lint > 1)
		return;

	uint32_t reg = lint ? APIC_LVTLINT1 : APIC_LVTLINT0;
	uint32_t value = APIC_DM_NMI;
	if (mps_inti_trigger_mode_level (flags, false))
		value |= APIC_TRIGGER_MODE_LEVEL;

	apic_write (reg, value);
}

static uint64_t apic_hz;

__INIT_TEXT
static void
apic_calibrate_timer (void)
{
	/**
	 * Writing zero to the timer initial count effectively disables the
	 * APIC timer.
	 */
	apic_write (APIC_TMR_ICR, 0);

	/**
	 * Setup the clock divisor to 16. A divisor of 1 can be problematic
	 * according to online sources.
	 */
	apic_write (APIC_TMR_DIV, 3);

	/**
	 * One-shot mode. Mask the timer.
	 */
	apic_write (APIC_LVTTMR, APIC_IRQ_MASK | VECTOR_APIC_TIMER);

	nsecs_t t0 = ns_since_boot ();
	if (!t0) {
		printk (PR_WARN "apic: no reference timer for calibration available\n");
		return;
	}

	/**
	 * Start the countdown.
	 */
	apic_write (APIC_TMR_ICR, 0xffffffffU);
	do {
		arch_relax ();
	} while (ns_since_boot () < t0 + 100UL * 1000UL * 1000UL); /* 100ms */

	uint32_t ccr = apic_read (APIC_TMR_CCR);

	/**
	 * Disable the APIC timer.
	 */
	apic_write (APIC_TMR_ICR, 0);

	uint32_t delta = 0xffffffffU - ccr;
	apic_hz = (uint64_t) delta * 160UL;

	printk (PR_INFO "apic: calibrated APIC timer clock frequency to %lu.%03luMhz\n",
		apic_hz / 1000000, (apic_hz / 1000) % 1000);

	if (apic_hz < 10000UL || apic_hz > 1000000000000UL) {
		printk (PR_WARN "apic: timer frequency is outside of allowed range, disabling...\n");
		apic_hz = 0;
	}
}

static bool has_calibrated_APIC;

/**
 * Initialize the APIC on this CPU. When this function is called, MSR_APIC_BASE
 * is expected to already be set-up.
 *
 * Not __INIT_TEXT because this function is called once for each CPU that is
 * brought online.
 */
void
apic_init (void)
{
	/* Soft-disable then soft-enable the APIC. */
	apic_write (APIC_SPI, 0);
	apic_write (APIC_SPI, APIC_SPI_ENABLE | VECTOR_SPURIOUS);

	x86_madt_parse_apic_nmis (acpi_madt_nmi);

	/**
	 * apic_calibrate_timer is called once on the BSP to calculate the bus
	 * speed.
	 */
	if (!has_calibrated_APIC) {
		has_calibrated_APIC = true;
		apic_calibrate_timer ();
	}

	if (apic_hz) {
		apic_write (APIC_TMR_ICR, 0);
		apic_write (APIC_TMR_DIV, 3);
		apic_write (APIC_LVTTMR, APIC_TMR_PERIODIC | VECTOR_APIC_TIMER);
		apic_write (APIC_TMR_ICR, apic_hz / (16 * 10));
	}
}
