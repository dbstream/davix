/**
 * Local APIC functions.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#include <stdint.h>

extern uint32_t cpu_to_apic_array[];
extern uint32_t cpu_to_acpi_uid_array[];

static inline uint32_t
cpu_to_apic_id (int cpu)
{
	return cpu_to_apic_array[cpu];
}

static inline uint32_t
cpu_to_acpi_uid (int cpu)
{
	return cpu_to_acpi_uid_array[cpu];
}

uint32_t
apic_read (int reg);

void
apic_write (int reg, uint32_t value);

uint32_t
apic_read_id (void);

void
apic_write_icr (uint32_t value, uint32_t target);

void
apic_wait_icr (void);

void
apic_send_IPI (uint32_t value, uint32_t target);

void
set_xAPIC_base (uintptr_t addr);

void
apic_init (void);

void
apic_eoi (void);

void
apic_start_timer (void);
