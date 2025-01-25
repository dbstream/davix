/**
 * Initialization and usage of ACPI tables.
 * Copyright (C) 2024  dbstream
 */
#include <acpi/acpi.h>
#include <acpi/tables.h>
#include <davix/endian.h>
#include <davix/panic.h>
#include <davix/printk.h>
#include <davix/stddef.h>
#include <davix/string.h>
#include <asm/page_defs.h>
#include <asm/sections.h>

unsigned long acpi_rsdp_phys_for_uacpi = 0;

#define MAX_ACPI_TABLES 128
struct acpi_table_list_entry acpi_tables[MAX_ACPI_TABLES];
unsigned long num_acpi_tables = 0;

struct acpi_table_fadt acpi_fadt;

unsigned int acpi_version_major;
unsigned int acpi_version_minor;

void *
acpi_find_table (const char *sig, unsigned long idx)
{
	for (unsigned long i = 0; i < num_acpi_tables; i++) {
		if (memcmp (acpi_tables[i].header.signature, sig, 4) != 0)
			continue;

		if (idx) {
			idx--;
			continue;
		}

		return (void *) phys_to_virt (acpi_tables[i].address);
	}

	return NULL;
}

void
acpi_parse_table (struct acpi_table_header *header, unsigned long offset,
	int (*callback)(struct acpi_subtable_header *, void *), void *p)
{
	unsigned long end = (unsigned long) header + le32toh(header->length);
	unsigned long addr = (unsigned long) header + offset;

	while (addr < end) {
		struct acpi_subtable_header *header =
			(struct acpi_subtable_header *) addr;

		if (!callback (header, p))
			return;

		addr += header->length;
	}
}

void
acpi_parse_table_madt (struct acpi_table_madt *madt,
	int (*callback)(struct acpi_subtable_header *, void *), void *p)
{
	acpi_parse_table ((struct acpi_table_header *) madt,
		sizeof (*madt), callback, p);
}

static inline void
print_table (struct acpi_table_header *header, unsigned long phys_addr)
{
	printk (PR_INFO "ACPI: %4.4s %p %06x (v%u %-6.6s %-8.8s)\n",
		header->signature, (void *) phys_addr, le32toh(header->length),
		header->revision, header->oemid, header->oem_table_id);
}

__INIT_TEXT
static void
emit_table (struct acpi_table_header *header, unsigned long addr)
{
	print_table (header, addr);
	if (num_acpi_tables >= MAX_ACPI_TABLES)
		panic ("Too many ACPI tables.");

	acpi_tables[num_acpi_tables].header = *header;
	acpi_tables[num_acpi_tables++].address = addr;
}

__INIT_TEXT
static void
parse_table_entry (unsigned long addr)
{
	for (;;) {
		if (!addr)
			return;

		struct acpi_table_header *table = arch_early_map_acpi_table (addr, 36);
		if (!table)
			panic ("ACPI subsystem coultn't map a table for early access.");

		emit_table (table, addr);

		unsigned long len = le32toh (table->length);
		arch_early_unmap_acpi_table (table);
		table = arch_early_map_acpi_table (addr, len);
		if (!table)
			panic ("ACPI subsystem coultn't map a table for early access.");

		if (acpi_compute_checksum (table, len) != 0)
			printk (PR_WARN "ACPI: incorrect checksum for \"%.4s\"\n",
				table->signature);

		addr = 0;
		if (!memcmp (table->signature, ACPI_SIG_FADT, 4)) {
			if (sizeof (acpi_fadt) < len)
				len = sizeof (acpi_fadt);
			memcpy (&acpi_fadt, table, len);

			addr = le32toh (acpi_fadt.dsdt);
			if (acpi_fadt.x_dsdt)
				addr = le64toh (acpi_fadt.x_dsdt);
		}

		arch_early_unmap_acpi_table (table);
	}
}

__INIT_TEXT
static void
acpi_parse_rsdt (struct acpi_table_rsdt *rsdt)
{
	unsigned long n = (rsdt->header.length - sizeof(rsdt->header)) / 4;
	for (unsigned long i = 0; i < n; i++)
		parse_table_entry (le32toh (rsdt->entries[i]));
}

__INIT_TEXT
static void
acpi_parse_xsdt (struct acpi_table_xsdt *rsdt)
{
	unsigned long n = (rsdt->header.length - sizeof(rsdt->header)) / 8;
	for (unsigned long i = 0; i < n; i++)
		parse_table_entry (le64toh (rsdt->entries[i]));
}

__INIT_TEXT
static void
acpi_parse_rsdp (struct acpi_table_rsdp *table)
{
	unsigned long addr;

	if (acpi_compute_checksum (table, 20) != 0)
		panic ("Incorrect checksum for RSD PTR.");

	addr = le32toh (table->rsdt_address);
	if (table->revision >= 2) {
		if (acpi_compute_checksum (table, table->length))
			panic ("Incorrect ext_checksum for RSD PTR.");
		if (table->xsdt_address)
			addr = le64toh (table->xsdt_address);
	}

	struct acpi_table_header *rsdt = arch_early_map_acpi_table(addr, 36);
	if (!rsdt)
		panic ("ACPI subsystem couldn't map RSDT for early access.");

	emit_table (rsdt, addr);

	unsigned long len = le32toh (rsdt->length);
	arch_early_unmap_acpi_table (rsdt);
	rsdt = arch_early_map_acpi_table (addr, len);
	if (!rsdt)
		panic ("ACPI subsystem couldn't map RSDT for early access.");

	if (acpi_compute_checksum (rsdt, len) != 0)
		panic ("Incorrect checksum for %.4s.", rsdt->signature);

	if (!memcmp (rsdt->signature, ACPI_SIG_RSDT, 4))
		acpi_parse_rsdt ((struct acpi_table_rsdt *) rsdt);
	else if (!memcmp (rsdt->signature, ACPI_SIG_XSDT, 4))
		acpi_parse_xsdt ((struct acpi_table_xsdt *) rsdt);
	else
		panic ("ACPI subsystem expectes \"RSDT\" or \"XSDT\", got \"%.4s\"",
			rsdt->signature);

	arch_early_unmap_acpi_table (rsdt);
}

/**
 * Read ACPI table headers into acpi_tables. 'root_table' must point to the
 * RSDP, RSDT, or XSDT, and the entirety of it is expected to be mapped.
 */
__INIT_TEXT
void
acpi_init_tables (void *root_table, unsigned long rsdp_phys)
{
	/**
	 * TODO:  fake a RSDP if we are passed the RSDT or XSDT.
	 */

	if (!memcmp (root_table, ACPI_SIG_RSDP, 8)) {
		acpi_rsdp_phys_for_uacpi = rsdp_phys;
		acpi_parse_rsdp (root_table);
	} else if (!memcmp (root_table, ACPI_SIG_RSDT, 4))
		acpi_parse_rsdt (root_table);
	else if (!memcmp (root_table, ACPI_SIG_XSDT, 4))
		acpi_parse_xsdt (root_table);
	else
		panic ("ACPI subsystem expects \"RSD PTR \", \"RSDT\", or \"XSDT\", got \"%.4s\"",
			(const char *) root_table);

	acpi_version_major = acpi_fadt.header.revision;
	acpi_version_minor = acpi_fadt.fadt_minor_version;
	printk (PR_NOTICE "ACPI: version %u.%u\n",
		acpi_version_major, acpi_version_minor);
}
