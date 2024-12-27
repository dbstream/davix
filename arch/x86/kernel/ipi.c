/**
 * Interprocessor interrupts.
 * Copyright (C) 2024  dbstream
 */
#include <davix/cpuset.h>
#include <davix/context.h>
#include <davix/panic.h>
#include <davix/smp.h>
#include <asm/apic.h>
#include <asm/entry.h>
#include <asm/fence.h>
#include <asm/interrupt.h>
#include <asm/ipi.h>
#include <asm/irq.h>
#include <asm/smp.h>

void
arch_send_call_on_cpu_IPI (unsigned int cpu)
{
	apic_send_IPI (APIC_DM_FIXED | VECTOR_CALL_ON_CPU, cpu_to_apicid[cpu]);
}

void
arch_panic_stop_others (void)
{
	unsigned int this_cpu = this_cpu_id ();

	/** send an IPI on VECTOR_PANIC to each CPU */
	for_each_online_cpu (cpu)
		if (cpu != this_cpu)
			apic_send_IPI (APIC_DM_FIXED | VECTOR_PANIC, cpu_to_apicid[cpu]);

	/** give all CPUs some time to respond to the IPI */
	for (int i = 0; i < 1000000; i++)
		arch_relax ();

	/** ok, they are not responding.  NMI them to death.  */
	for_each_online_cpu (cpu)
		if (cpu != this_cpu)
		apic_send_IPI (APIC_DM_NMI, cpu_to_apicid[cpu]);

	/** wait for CPUs to go offline  */
	for_each_online_cpu (cpu)
		if (cpu != this_cpu)
			do
				arch_relax ();
			while (cpu_online (cpu));

	/** final memory barrier  */
	mb ();
}

void
handle_call_on_cpu_IPI (struct entry_regs *regs)
{
	preempt_state_t state = preempt_enter_IRQ ();

	(void) regs;
	smp_do_call_on_cpu_work ();

	preempt_leave_IRQ (state);
}

void
handle_panic_IPI (struct entry_regs *regs)
{
	preempt_state_t state = preempt_enter_IRQ ();

	(void) regs;
	panic_stop_self ();

	preempt_leave_IRQ (state);
}
