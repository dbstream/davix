/**
 * xAPIC and x2APIC (Advanced Programmable Interrupt Controller) support.
 * Copyright (C) 2024  dbstream
 */
#include <davix/printk.h>
#include <asm/acpi.h>
#include <asm/apic.h>
#include <asm/early_memmap.h>
#include <asm/features.h>
#include <asm/irq.h>
#include <asm/msr.h>
#include <asm/sections.h>
#include <asm/smp.h>

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
		APIC_REG (APIC_ICR_HIGH) = target;
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
	apic_write (APIC_SPI, 0xff | APIC_SPI_ENABLE);

	x86_madt_parse_apic_nmis (acpi_madt_nmi);
}
