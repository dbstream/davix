/**
 * acpi_parse_subtable()
 * Copyright (C) 2025-present  dbstream
 */
#include <davix/acpi_table.h>

uacpi_iteration_decision
acpi_parse_subtable (acpi_sdt_hdr *header, size_t header_len,
		uacpi_iteration_decision (*callback) (acpi_entry_hdr *, void *),
		void *arg)
{
	uintptr_t addr = (uintptr_t) header + header_len;
	uintptr_t end = (uintptr_t) header + header->length;

	while (addr < end) {
		acpi_entry_hdr *entry = (acpi_entry_hdr *) addr;

		uacpi_iteration_decision decision = callback (entry, arg);
		if (decision == UACPI_ITERATION_DECISION_BREAK)
			return decision;

		addr += entry->length;
	}

	return UACPI_ITERATION_DECISION_CONTINUE;
}
