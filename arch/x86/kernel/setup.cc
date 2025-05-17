/**
 * arch_init
 * Copyright (C) 2025-present  dbstream
 */
#include <asm/apic_def.h>
#include <asm/apic.h>
#include <asm/io.h>
#include <asm/page_defs.h>
#include <asm/time.h>
#include <davix/acpi_table.h>
#include <davix/cpuset.h>
#include <davix/panic.h>
#include <davix/printk.h>
#include <davix/start_kernel.h>
#include <uacpi/tables.h>
#include <uacpi/uacpi.h>

/**
 * Remap the legacy PIC so it doesn't affect us.
 *
 * NOTE: there is no way to "disable" the legacy PIC, and even when all lines
 * are masked, spurious interrupts can still be signalled.  Therefore we remap
 * the legacy PIC so that the spurious interrupt lines up with that of the
 * local APIC.
 */
static void
disable_legacy_PIC (void)
{
	io_outb (0x20, 0x11);
	io_outb (0x80, 0);
	io_outb (0xa0, 0x11);
	io_outb (0x80, 0);
	io_outb (0x21, 248);	/* 8259A master IRQ offset */
	io_outb (0x80, 0);
	io_outb (0xa1, 248);	/* 8259A slave IRQ offset */
	io_outb (0x80, 0);
	io_outb (0x21, 0x04);
	io_outb (0x80, 0);
	io_outb (0xa1, 0x02);
	io_outb (0x80, 0);
	io_outb (0x21, 0x01);
	io_outb (0x80, 0);
	io_outb (0xa1, 0x01);
	io_outb (0x80, 0);
	io_outb (0x21, 0xff);
	io_outb (0xa1, 0xff);
	io_outb (0x80, 0);
}

uint32_t cpu_to_apic_array[CONFIG_MAX_NR_CPUS];
uint32_t cpu_to_acpi_uid_array[CONFIG_MAX_NR_CPUS];

static acpi_madt *madt;

static uacpi_iteration_decision
madt_lapic_addr_override (acpi_entry_hdr *entry, void *arg)
{
	(void) arg;

	if (entry->type == ACPI_MADT_ENTRY_TYPE_LAPIC_ADDRESS_OVERRIDE) {
		acpi_madt_lapic_address_override *lapic_override =
			(acpi_madt_lapic_address_override *) entry;

		set_xAPIC_base (lapic_override->address);
	}

	return UACPI_ITERATION_DECISION_CONTINUE;
}

void
arch_init (void)
{
	/**
	 * Setup early table access.
	 * NB: this uses the boot_pagetables memory as the temporary buffer.
	 */
	uacpi_setup_early_table_access ((void *) (KERNEL_START + 0x4000), 0x2000UL);

	x86_init_time ();

	uacpi_table madt_table;
	uacpi_status status = uacpi_table_find_by_signature (ACPI_MADT_SIGNATURE,
			&madt_table);

	if (status != UACPI_STATUS_OK) {
		panic ("uacpi_table_find_by_signature(APIC) returned %s",
				uacpi_status_to_string (status));
	}

	madt = (acpi_madt *) madt_table.ptr;

	set_xAPIC_base (madt->local_interrupt_controller_address);
	acpi_parse_madt (madt, madt_lapic_addr_override, nullptr);

	disable_legacy_PIC ();
	apic_init ();
}
