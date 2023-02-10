/* SPDX-License-Identifier: MIT */
#include <davix/acpi.h>
#include <davix/panic.h>
#include <davix/printk.h>
#include <davix/resource.h>
#include <asm/apic.h>
#include <asm/page.h>
#include <asm/cpuid.h>

struct apic *active_apic_interface;

unsigned long apic_base = 0xfee00000;

static void apic_parse_madt(struct acpi_subtable_header *entry, void *arg)
{
	(void) arg;

	if(entry->type == ACPI_MADT_TYPE_LOCAL_APIC_OVERRIDE) {
		struct acpi_madt_local_apic_override *override =
			(struct acpi_madt_local_apic_override *) entry;

		apic_base = override->address;
	}
}

void apic_init(struct acpi_table_madt *madt)
{
	unsigned long a, b, c, d;
	do_cpuid(1, a, b, c, d);
	if(c & __ID_00000001_ECX_X2APIC) {
		info("apic_init(): x2apic supported.\n");
		active_apic_interface = &x2apic_impl;
	} else {
		info("apic_init(): x2apic not supported.\n");
		active_apic_interface = &xapic_impl;
	}

	if(madt->address)
		apic_base = madt->address;

	acpi_parse_table(madt, apic_parse_madt, NULL);

	struct resource *apic_resource = alloc_resource_at(&system_memory,
		apic_base, PAGE_SIZE, "memory-mapped APIC registers");

	if(!apic_resource)
		panic("apic_init(): couldn't allocate struct resource for APIC registers (memory already in use?)");

	active_apic_interface->init();
}
