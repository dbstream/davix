/**
 * Interprocessor interrupts on x86.
 * Copyright (C) 2025-present  dbstream
 */
#include <asm/apic_def.h>
#include <asm/apic.h>
#include <asm/interrupt.h>
#include <asm/smp.h>
#include <asm/switch_to.h>
#include <davix/cpuset.h>

void
arch_send_smp_call_on_one_IPI (unsigned int cpu)
{
	uint32_t apicid = cpu_to_apic_id (cpu);

	apic_send_IPI (APIC_DM_FIXED | VECTOR_SMP_CALL_ON_ONE, apicid);
}

void
arch_send_panic_IPI_to_others (void)
{
	for (unsigned int cpu : cpu_online) {
		if (cpu == this_cpu_id ())
			continue;

		uint32_t apicid = cpu_to_apic_id (cpu);
		apic_send_IPI (APIC_DM_FIXED | VECTOR_SMP_PANIC, apicid);
	}
}

void
arch_send_panic_NMI_to_others (void)
{
	for (unsigned int cpu : cpu_online) {
		if (cpu == this_cpu_id ())
			continue;

		/*
		 * Currently there is nothing on the NMI vector.  That is fine
		 * since it will just General-Protection fault instead.
		 */
		uint32_t apicid = cpu_to_apic_id (cpu);
		apic_send_IPI (APIC_DM_NMI, apicid);
	}
}

void
arch_send_reschedule_IPI (unsigned int target)
{
	uint32_t apicid = cpu_to_apic_id (target);
	apic_send_IPI (APIC_DM_FIXED | VECTOR_SMP_RESCHEDULE, apicid);
}
