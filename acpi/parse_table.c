#include <davix/acpi.h>

void __acpi_parse_table(struct acpi_table_header *table, unsigned long hdrlen,
	void (*callback)(struct acpi_subtable_header *, void *), void *arg)
{
	struct acpi_subtable_header *entry, *end;

	end = (struct acpi_subtable_header *)
		((unsigned long) table + table->length);

	entry = (struct acpi_subtable_header *)
		((unsigned long) table + hdrlen);

	while(entry < end) {
		callback(entry, arg);
		entry = (struct acpi_subtable_header *)
			((unsigned long) entry + entry->length);
	}
}
