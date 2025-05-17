/**
 * acpi_parse_subtable()
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#include <uacpi/acpi.h>

uacpi_iteration_decision
acpi_parse_subtable (acpi_sdt_hdr *header, size_t header_len,
		uacpi_iteration_decision (*callback) (acpi_entry_hdr *, void *),
		void *arg);

static inline uacpi_iteration_decision
acpi_parse_madt (acpi_madt *madt,
		uacpi_iteration_decision (*callback) (acpi_entry_hdr *, void *),
		void *arg)
{
	return acpi_parse_subtable (&madt->hdr, sizeof (*madt), callback, arg);
}
