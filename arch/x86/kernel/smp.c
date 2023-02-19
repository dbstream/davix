/* SPDX-License-Identifier: MIT */
#include <davix/acpi.h>
#include <davix/smp.h>
#include <davix/printk.h>
#include <davix/panic.h>
#include <asm/apic.h>
#include <asm/msr.h>

struct logical_cpu *cpu_slots = NULL;
unsigned num_cpu_slots = 0;

static void madt_calc_max_cpu(struct acpi_subtable_header *hdr, void *arg)
{
	(void) arg;

	unsigned no;
	switch(hdr->type) {
	case ACPI_MADT_TYPE_LOCAL_APIC: do {
		struct acpi_madt_local_apic *apic =
			(struct acpi_madt_local_apic *) hdr;
		no = apic->id;
		goto common;
	} while (0);
	case ACPI_MADT_TYPE_LOCAL_X2APIC: do {
		struct acpi_madt_local_x2apic *apic =
			(struct acpi_madt_local_x2apic *) hdr;
		no = apic->local_apic_id;
		goto common;
	} while(0);
	default:
		return;
	}

common:
	if(no + 1 > num_cpu_slots)
		num_cpu_slots = no + 1;
}

static void madt_set_cpu_present(struct acpi_subtable_header *hdr, void *arg)
{
	(void) arg;

	bool present = 0;
	unsigned no;
	switch(hdr->type) {
	case ACPI_MADT_TYPE_LOCAL_APIC: do {
		struct acpi_madt_local_apic *apic =
			(struct acpi_madt_local_apic *) hdr;
		if(apic->lapic_flags & 0b11)
			present = 1;
		no = apic->id;
		goto common;
	} while (0);
	case ACPI_MADT_TYPE_LOCAL_X2APIC: do {
		struct acpi_madt_local_x2apic *apic =
			(struct acpi_madt_local_x2apic *) hdr;
		if(apic->lapic_flags & 0b11)
			present = 1;
		no = apic->local_apic_id;
		goto common;
	} while(0);
	default:
		return;
	}

common:;
	struct logical_cpu *cpu = &cpu_slots[no];
	cpu->possible = 1;
	cpu->present = present;
}

extern char __cpulocal_start[], __cpulocal_end[];

struct logical_cpu *__smp_self cpulocal;

void arch_init_smp(void);
void arch_init_smp(void)
{
	struct acpi_table_madt *madt = NULL;
	acpi_get_table(ACPI_SIG_MADT, 0, (struct acpi_table_header **) &madt);

	/*
	 * As a rule, all systems nowadays have the MADT.
	 * Don't bother dealing with the very few unsupported machines.
	 */
	if(!madt)
		panic("arch_init_smp(): no MADT table found.");

	apic_init(madt);

	/*
	 * The BSP might not always be CPU#0...
	 */
	unsigned bsp = apic_read_id();
	debug("smp: CPU#%u is the BSP.\n", bsp);

	smp_self()->id = bsp;

	/*
	 * We need to know the highest logical CPU number to allocate the
	 * 'struct logical_cpu' array.
	 */
	acpi_parse_table(madt, madt_calc_max_cpu, NULL);
	if(!num_cpu_slots)
		panic("arch_init_smp(): no LAPIC entries in MADT.");

	info("Max CPUs: %u\n", num_cpu_slots);

	if(num_cpu_slots <= smp_self()->id)
		panic("arch_init_smp(): Too few CPU slots! (num_cpu_slots: %u, BSP: %u)\n");

	/* TODO: use something else than kmalloc() */
	cpu_slots = kmalloc(num_cpu_slots * sizeof(struct logical_cpu));
	if(!cpu_slots)
		panic("arch_init_smp(): Couldn't allocate struct logical_cpu array.");

	for_each_logical_cpu(cpu) {
		cpu->id = cpu - cpu_slots;	/* magic pointer arithmetic */
		cpu->online = 0;
		cpu->possible = 0;
		cpu->present = 0;
	}

	/*
	 * For the second time, parse the MADT. We now figure out which CPUs
	 * are present.
	 */
	acpi_parse_table(madt, madt_set_cpu_present, NULL);

	/* Set the BSP to online */
	cpu_slots[smp_self()->id].online = 1;

	/*
	 * We are done with the MADT now.
	 */
	acpi_put_table((struct acpi_table_header *) madt);

	for_each_logical_cpu(cpu) {
		if(!cpu->possible)
			continue;

		if(cpu->online) {
			/* An already onlined CPU must be the BSP. */
			cpu->cpulocal_offset = 0;
		} else {
			/* TODO: use something else than kmalloc() */
			void *mem = kmalloc(__cpulocal_end - __cpulocal_start);
			if(!mem)
				panic("x86/smp: Cannot allocate memory for CPU-local variables!\n");

			cpu->cpulocal_offset = (unsigned long) mem - (unsigned long) __cpulocal_start;
		}

		rdwr_cpulocal_on(cpu, __smp_self) = cpu;
	}

	write_msr(MSR_GSBASE, (unsigned long) &__smp_self);
}
