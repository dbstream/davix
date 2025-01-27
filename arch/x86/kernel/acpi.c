/**
 * x86-specific support for ACPI.
 * Copyright (C) 2024  dbstream
 */
#include <acpi/acpi.h>
#include <acpi/tables.h>
#include <davix/align.h>
#include <davix/endian.h>
#include <davix/printk.h>
#include <asm/acpi.h>
#include <asm/apic.h>
#include <asm/early_memmap.h>
#include <asm/mm_init.h>
#include <asm/sections.h>

static struct acpi_table_madt *madt;

__INIT_TEXT
void *
arch_early_map_acpi_table (unsigned long addr, unsigned long size)
{
	return early_memmap (addr, size, PAGE_KERNEL_DATA);
}

__INIT_TEXT
void
arch_early_unmap_acpi_table (void *virt)
{
	early_memunmap (virt);
}

__INIT_TEXT
static int
parse_apics_callback (struct acpi_subtable_header *header, void *p)
{
	void (*callback)(uint32_t, uint32_t, int) = p;

	uint32_t apic_id, acpi_uid, flags;
	if (header->type == ACPI_MADT_LOCAL_APIC) {
		struct acpi_madt_local_apic *lapic = (void *) header;
		apic_id = lapic->apic_id;
		acpi_uid = lapic->acpi_processor_uid;
		flags = le32toh (lapic->flags);
	} else if (header->type == ACPI_MADT_LOCAL_X2APIC) {
		struct acpi_madt_local_x2apic *lapic = (void *) header;
		apic_id = le32toh (lapic->apic_id);
		acpi_uid = le32toh (lapic->acpi_processor_uid);
		flags = le32toh (lapic->flags);
	} else
		return 1;

	callback (apic_id, acpi_uid, (flags & ACPI_MADT_LAPIC_ENABLED) ? 1 : 0);
	return 1;
}

__INIT_TEXT
void
x86_madt_parse_apics (void (*callback)(uint32_t, uint32_t, int))
{
	if (madt)
		acpi_parse_table_madt (madt, parse_apics_callback, callback);
}

__INIT_TEXT
static int
parse_ioapics_callback (struct acpi_subtable_header *header, void *p)
{
	void (*callback) (uint32_t, unsigned long, uint32_t) = p;

	uint32_t apic_id, gsi_base;
	unsigned long addr;
	if (header->type == ACPI_MADT_IO_APIC) {
		struct acpi_madt_io_apic *ioapic = (struct acpi_madt_io_apic *) header;
		apic_id = le32toh (ioapic->ioapic_id);
		addr = le32toh (ioapic->ioapic_addr);
		gsi_base = le32toh (ioapic->gsi_base);
	} else
		return 1;

	callback (apic_id, addr, gsi_base);
	return 1;
}

__INIT_TEXT
void
x86_madt_parse_ioapics (void (*callback)(uint32_t, unsigned long, uint32_t))
{
	if (madt)
		acpi_parse_table_madt (madt, parse_ioapics_callback, callback);
}

__INIT_TEXT
static int
parse_interrupt_overrides_callback (struct acpi_subtable_header *header, void *p)
{
	/**
	 * (map_source, map_dest, active_hi, active_lo, tgm_edge, tgm_level);
	 */
	void (*callback) (uint32_t, uint32_t, bool, bool, bool, bool) = p;

	uint8_t bus;
	uint32_t map_source, map_dest;
	uint16_t mps_flags;
	if (header->type == ACPI_MADT_INTERRUPT_OVERRIDE) {
		struct acpi_madt_interrupt_override *o =
				(struct acpi_madt_interrupt_override *) header;

		bus = o->bus;
		map_source = o->source_irq;
		map_dest = le32toh (o->gsi_irq);
		mps_flags = le16toh (o->flags);
	} else
		return 1;

	if (bus != 0) {
		printk (PR_WARN "x86_madt_parse_interrupt_overrides: invalid IRQ override bus=%u source=%u dest=%u flags=0x%x\n",
				bus, map_source, map_dest, mps_flags);
		return 1;
	}

	bool force_active_high = false;
	bool force_active_low = false;
	bool force_tgm_edge = false;
	bool force_tgm_level = false;
	bool warn = false;

	if (mps_flags & (1 << 0)) {
		if (mps_flags & (1 << 1))
			force_active_low = true;
		else
			force_active_high = true;
	} else if (mps_flags & (1 << 1))
		warn = true;

	if (mps_flags & (1 << 2)) {
		if (mps_flags & (1 << 3))
			force_tgm_level = true;
		else
			force_tgm_edge = true;
	} else if (mps_flags & (1 << 3))
		warn = true;

	if (mps_flags & ~0xfU)
		warn = true;

	if (warn)
		printk (PR_WARN "x86_madt_parse_interrupt_overrides: unknown MPS flags 0x%x\n",
				mps_flags);

	if (map_source >= 16) {
		printk (PR_WARN "x86_madt_parse_interrupt_overrides: source=%u is outside of ISA range\n",
				map_source);
		return true;
	}

	callback (map_source, map_dest, force_active_high, force_active_low,
			force_tgm_edge, force_tgm_level);
	return true;
}

__INIT_TEXT
void
x86_madt_parse_interrupt_overrides (void (*callback) (uint32_t, uint32_t,
		bool, bool, bool, bool))
{
	if (madt)
		acpi_parse_table_madt (madt, parse_interrupt_overrides_callback,
				callback);
}

static int
parse_apic_nmi_callback (struct acpi_subtable_header *header, void *p)
{
	void (*callback) (uint32_t, int, uint16_t) = p;

	uint32_t acpi_uid;
	int lint;
	uint16_t flags;
	if (header->type == ACPI_MADT_LOCAL_APIC_NMI) {
		struct acpi_madt_local_apic_nmi *nmi = (void *) header;
		acpi_uid = nmi->acpi_processor_uid;
		lint = nmi->lint_num;
		flags = le16toh (nmi->flags);
		if (acpi_uid == 0xffU)
			acpi_uid = 0xffffffffU;
	} else if (header->type == ACPI_MADT_LOCAL_X2APIC_NMI) {
		struct acpi_madt_local_x2apic_nmi *nmi = (void *) header;
		acpi_uid = le32toh (nmi->acpi_processor_uid);
		lint = nmi->lint_num;
		flags = le16toh (nmi->flags);
	} else
		return 1;

	callback (acpi_uid, lint, flags);
	return 1;
}

void
x86_madt_parse_apic_nmis (void (*callback) (uint32_t, int, uint16_t))
{
	if (madt)
		acpi_parse_table_madt (madt, parse_apic_nmi_callback, callback);
}

__INIT_TEXT
static int
madt_lapic_addr_override (struct acpi_subtable_header *header, void *p)
{
	(void) p;
	if (header->type != ACPI_MADT_LAPIC_ADDR_OVERRIDE)
		return 1;

	struct acpi_madt_lapic_addr_override *ovr =
		(struct acpi_madt_lapic_addr_override *) header;
	set_xAPIC_base (le64toh (ovr->address));
	return 0;
}

/**
 * Map ACPI tables into the HHDM and find the xAPIC base in the MADT.
 */
__INIT_TEXT
void
x86_init_acpi_tables_early (void)
{
	/**
	 * Per spec, ACPI tables are stored in ACPI_RECLAIM memory. In
	 * reality, everyone doesn't follow the spec. Therefore we cannot
	 * rely on x86_paging_init to map all ACPI tables.
	 */
	for (unsigned long i = 0; i < num_acpi_tables; i++) {
		unsigned long start = acpi_tables[i].address;
		unsigned long end = start
			+ le32toh (acpi_tables[i].header.length);

		map_hhdm_range (ALIGN_DOWN (start, PAGE_SIZE),
			ALIGN_UP (end, PAGE_SIZE));
	}

	madt = acpi_find_table (ACPI_SIG_MADT, 0);
	if (!madt) {
		printk (PR_WARN "ACPI: no MADT.\n");
		return;
	}

	set_xAPIC_base (madt->local_apic_addr);
	acpi_parse_table_madt (madt, madt_lapic_addr_override, 0);
}
