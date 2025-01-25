/**
 * External API for the ACPI subsystem.
 * Copyright (C) 2024  dbstream
 */
#ifndef _ACPI_ACPI_H
#define _ACPI_ACPI_H

#include <davix/stdbool.h>

struct acpi_table_rsdp;
struct acpi_table_header;

extern unsigned long acpi_rsdp_phys_for_uacpi;

extern void *
arch_early_map_acpi_table (unsigned long addr, unsigned long size);

extern void
arch_early_unmap_acpi_table (void *virt);

extern void
acpi_init_tables (void *root_table, unsigned long rsdp_phys_for_uacpi);

extern unsigned int acpi_version_major;
extern unsigned int acpi_version_minor;

static inline bool
acpi_newer_than (unsigned int major, unsigned int minor)
{
	return acpi_version_major > major ||
		(acpi_version_major == major && acpi_version_minor >= minor);
}

#endif /* _ACPI_ACPI_H */
