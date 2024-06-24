/**
 * x86 support for Simultaneous Multiprocessing.
 * Copyright (C) 2024  dbstream
 */
#include <davix/cpuset.h>
#include <davix/initmem.h>
#include <davix/panic.h>
#include <davix/printk.h>
#include <asm/acpi.h>
#include <asm/apic.h>
#include <asm/cpulocal.h>
#include <asm/page_defs.h>
#include <asm/sections.h>
#include <asm/smp.h>

static uint32_t bsp_apic_id;

unsigned long __cpulocal_offsets[CONFIG_MAX_NR_CPUS];

__INIT_TEXT
static void
local_apic_callback (uint32_t apic_id, uint32_t acpi_uid, int present)
{
	if (apic_id == bsp_apic_id) {
		cpu_to_acpi_uid[0] = acpi_uid;
		return;
	}

	if (nr_cpus >= CONFIG_MAX_NR_CPUS)
		return;

	cpu_to_apicid[nr_cpus] = apic_id;
	cpu_to_acpi_uid[nr_cpus] = acpi_uid;
	if (present)
		set_cpu_present (nr_cpus);

	nr_cpus++;
}

__INIT_TEXT
void
x86_smp_init (void)
{
	bsp_apic_id = apic_read_id ();
	printk (PR_INFO "x86/smp: CPU0 has apic_id=%u\n", bsp_apic_id);

	cpu_to_apicid[0] = bsp_apic_id;

	x86_madt_parse_apics (local_apic_callback);

	/* setup CPU-local regions */
	/* BSP is always CPU0, hence start iterating at 1 */
	unsigned long size = __cpulocal_virt_end - __cpulocal_virt_start;
	for (unsigned int i = 1; i < nr_cpus; i++) {
		__cpulocal_offsets[i] =
			(unsigned long) initmem_alloc_virt (size, PAGE_SIZE);
		if (!__cpulocal_offsets[i])
			panic ("Out of memory when allocating CPU-local regions.");

		that_cpu_write (&__cpulocal_offset, i, __cpulocal_offsets[i]);
		that_cpu_write (&__this_cpu_id, i, i);
		that_cpu_write (&__stack_canary, i, this_cpu_read (&__stack_canary));
	}
}
