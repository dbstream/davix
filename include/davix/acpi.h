#ifndef __DAVIX_ACPI_H
#define __DAVIX_ACPI_H

#include <acpi/acpi.h>

void __acpi_parse_table(struct acpi_table_header *table, unsigned long hdrlen,
	void (*callback)(struct acpi_subtable_header *, void *), void *arg);

#define acpi_parse_table(table, callback, arg) \
	__acpi_parse_table((struct acpi_table_header *) (table), \
		(sizeof(*(table))), \
		(callback), (arg));

#endif /* __DAVIX_ACPI_H */
