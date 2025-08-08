// SPDX-License-Identifier: GPL-3.0
/*
 * File: acpi/tables.cc
 * ACPI Table management and initialization.
 *
 * Copyright (C) 2025  dbstream
 */
#include <Acpi/setup.h>
#include <Acpi/tables.h>
#include <Ke/log.h>
#include <uacpi/uacpi.h>

acpi_fadt *AcpiFADT;

void AcpiInitializeTables(void)
{
	KePrintf("ACPI: initializing table access\n");
	uacpi_status status = uacpi_initialize(UACPI_FLAG_NO_ACPI_MODE);
	if (status != UACPI_STATUS_OK)
		KePanic("ACPI: uacpi_initialize returned %d", (int) status);

	void *table_ptr;
	size_t table_id;
	bool found = AcpiGetTable(ACPI_FADT_SIGNATURE, &table_ptr, &table_id);
	if (!found)
		KePanic("ACPI: no FADT table");

	AcpiFADT = (acpi_fadt *) table_ptr;
}

/**
 * AcpiParseSubtables - parse subtables of an ACPI table.
 * @header: table header
 * @header_len: length of header in bytes
 * @callback: table entry callback
 * @arg: argument passed to @callback
 */
void AcpiParseSubtables(
		acpi_sdt_hdr *header,
		size_t header_len,
		ACPI_SUBTABLE_CALLBACK callback,
		void *arg
)
{
	unsigned long addr = (unsigned long) header + header_len;
	unsigned long end = (unsigned long) header + header->length;

	while (addr < end) {
		acpi_entry_hdr *entry = (acpi_entry_hdr *) addr;

		callback(entry, arg);

		addr += entry->length;
	}
}

