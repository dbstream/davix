/**
 * Interprocessor interrupts on x86.
 * Copyright (C) 2025-present  dbstream
 */
#include <asm/apic_def.h>
#include <asm/apic.h>
#include <asm/interrupt.h>
#include <asm/smp.h>

void
arch_send_smp_call_on_one_IPI (unsigned int cpu)
{
	uint32_t apicid = cpu_to_apic_id (cpu);

	apic_send_IPI (APIC_DM_FIXED | VECTOR_SMP_CALL_ON_ONE, apicid);
}
