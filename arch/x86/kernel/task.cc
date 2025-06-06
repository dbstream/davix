/**
 * Task state management.
 * Copyright (C) 2025-present  dbstream
 */
#include <asm/gdt.h>
#include <asm/percpu.h>
#include <string.h>

struct gdt_struct {
	uint64_t entries[GDT_NUM_ENTRIES];
};

extern "C" DEFINE_PERCPU(gdt_struct, percpu_GDT);

PERCPU_CONSTRUCTOR(init_gdt)
{
	gdt_struct *source = percpu_ptr(percpu_GDT);
	gdt_struct *target = percpu_ptr(percpu_GDT).on (cpu);

	memcpy (target, source, sizeof (gdt_struct));
}
