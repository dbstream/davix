/* SPDX-License-Identifier: MIT */
#include <davix/acpi.h>
#include <davix/smp.h>
#include <davix/printk.h>
#include <davix/panic.h>

static void print_madt(struct acpi_subtable_header *entry, void *arg)
{
	(void) arg;

	switch(entry->type) {
	case ACPI_MADT_TYPE_LOCAL_APIC: {
		struct acpi_madt_local_apic *apic =
			(struct acpi_madt_local_apic *) entry;

		debug("madt: local apic %u (flags 0x%x) corresponds to ACPI CPU %u\n",
			apic->id,
			apic->lapic_flags,
			apic->processor_id);
		return;
	}
	case ACPI_MADT_TYPE_LOCAL_X2APIC: {
		struct acpi_madt_local_x2apic *apic =
			(struct acpi_madt_local_x2apic *) entry;

		debug("madt: local x2apic %u (flags 0x%x) corresponds to ACPI CPU %u\n",
			apic->local_apic_id,
			apic->lapic_flags,
			apic->uid);
		return;
	}
	default: {
		debug("madt: unhandled subtable type %u\n", entry->type);
		return;
	}
	}
}

void init_smp(void)
{
	struct acpi_table_madt *madt = NULL;

	acpi_get_table(ACPI_SIG_MADT, 0, (struct acpi_table_header **) &madt);
	if(!madt)
		panic("init_smp(): no MADT table found.");

	acpi_parse_table(madt, print_madt, NULL);
	acpi_put_table((struct acpi_table_header *) madt);
}
