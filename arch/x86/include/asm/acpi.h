/**
 * ACPI initialization on x86.
 * Copyright (C) 2024  dbstream
 */
#ifndef _ASM_ACPI_H
#define _ASM_ACPI_H 1

#include <davix/stdbool.h>
#include <davix/stdint.h>

extern void
x86_init_acpi_tables_early (void);

extern void
x86_madt_parse_apics (void (*callback)
	(uint32_t apic_id, uint32_t acpi_uid, int present));

extern void
x86_madt_parse_apic_nmis (void (*callback)
	(uint32_t acpi_uid, int lint, uint16_t flags));

static inline bool
mps_inti_polarity_low (uint16_t inti_flags, bool dfl)
{
	bool ret[4] = { dfl, false, dfl, true };
	return ret[inti_flags & 3];
}

static inline bool
mps_inti_trigger_mode_level (uint16_t inti_flags, bool dfl)
{
	bool ret[4] = { dfl, false, dfl, true };
	return ret[(inti_flags >> 2) & 3];
}

#endif /* _ASM_ACPI_H */
