// SPDX-License-Identifier: GPL-3.0
/*
 * File: Hal/irq/ipi.cc
 * Issuing interprocessor interrupts.
 *
 * Copyright (C) 2025  dbstream
 */
#include <Hal/ipi.h>
#include <Hal/irq_vectors.h>
#include <asm/apic-def.h>
#include <asm/apic.h>

void HalSendPanicIPI(unsigned int cpu)
{
	unsigned int apicid = halCpuToApic[cpu];
	apic_send_IPI(APIC_DM_FIXED | IRQ_VECTOR_KERNEL_PANIC, apicid);
}

void HalSendRescheduleIPI(unsigned int cpu)
{
	unsigned int apicid = halCpuToApic[cpu];
	apic_send_IPI(APIC_DM_FIXED | IRQ_VECTOR_RESCHEDULE, apicid);
}

void HalSendSchedTimerRecalcIPI(unsigned int cpu)
{
	unsigned int apicid = halCpuToApic[cpu];
	apic_send_IPI(APIC_DM_FIXED | IRQ_VECTOR_SCHED_TIMER_DIRTY, apicid);
}

