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

static uint32_t bsp_apic_id;
static acpi_madt *madt;

static int cpu_count = 1;

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

static uacpi_iteration_decision
madt_cpus (acpi_entry_hdr *entry, void *arg)
{
	uint32_t apic_id, acpi_uid, flags;

	if (entry->type == ACPI_MADT_ENTRY_TYPE_LAPIC) {
		acpi_madt_lapic *lapic = (acpi_madt_lapic *) entry;
		apic_id = lapic->id;
		acpi_uid = lapic->uid;
		flags = lapic->flags;
	} else if (entry->type == ACPI_MADT_ENTRY_TYPE_LOCAL_X2APIC) {
		acpi_madt_x2apic *x2apic = (acpi_madt_x2apic *) entry;
		apic_id = x2apic->id;
		acpi_uid = x2apic->uid;
		flags = x2apic->flags;
	} else
		return UACPI_ITERATION_DECISION_CONTINUE;

	if (apic_id == bsp_apic_id) {
		if (arg)
			cpu_to_acpi_uid_array[0] = acpi_uid;
		return UACPI_ITERATION_DECISION_CONTINUE;
	}

	if (cpu_count >= CONFIG_MAX_NR_CPUS)
		return UACPI_ITERATION_DECISION_BREAK;

	if (!(flags & ACPI_PIC_ENABLED))
		return UACPI_ITERATION_DECISION_CONTINUE;

	if (arg) {
		cpu_to_apic_array[cpu_count] = apic_id;
		cpu_to_acpi_uid_array[cpu_count] = acpi_uid;
		cpu_present.set (cpu_count);
	}

	cpu_count++;
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

	bsp_apic_id = apic_read_id ();
	cpu_to_apic_array[0] = bsp_apic_id;

	cpu_count = 1;
	acpi_parse_madt (madt, madt_cpus, nullptr);
	set_nr_cpus (cpu_count);

	cpu_count = 1;
	acpi_parse_madt (madt, madt_cpus, (void *) 1);

	madt = nullptr;
	uacpi_table_unref (&madt_table);
}
