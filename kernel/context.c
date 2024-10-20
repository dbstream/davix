/**
 * Interruption/preemption context tracking.
 * Copyright (C) 2024  dbstream
 */
#include <davix/context.h>
#include <davix/printk.h>
#include <davix/sched.h>
#include <davix/stdbool.h>
#include <asm/cpulocal.h>

void
preempt_resched (void)
{
	do {
		preempt_off ();

		/**
		 * We do not need to be atomic here. The only bit that can
		 * change if we are interrupted is the PREEMPT_NO_RESCHED bit,
		 * but we will reschedule anyways, so it does not matter.
		 */
		set_preempt_state (get_preempt_state () | PREEMPT_NO_RESCHED);

		schedule ();
	} while (preempt_on_atomic ());
}

static preempt_state_t
preempt_enter_generic (preempt_state_t type)
{
	/* interrupts are disabled and preempt_state cannot be changed by NMI */
	preempt_state_t state = get_preempt_state ();
	set_preempt_state (state | type);
	return state;
}

static void
preempt_leave_generic (preempt_state_t state, bool is_nmi)
{
	preempt_state_t new_state = get_preempt_state ();
	preempt_state_t mask = PREEMPT_IRQ_LEVEL_MASK;
	if (!is_nmi)
		mask |= PREEMPT_NO_RESCHED;

	if ((state & ~mask) != (new_state & ~mask)) {
		/**
		 * We detected unbalanced preempt_on/preempt_off, or someone
		 * tried to modify PREEMPT_NO_RESCHED in NMI context.
		 */
		printk (PR_ERR "preempt_leave_generic(%d): illegal preempt_state transition 0x%08x --> 0x%08x\n",
			(int) is_nmi,
			state, new_state);
	}

	if (!is_nmi)
		state = (state & ~PREEMPT_NO_RESCHED) | (new_state & PREEMPT_NO_RESCHED);

	set_preempt_state (state);
	if (!is_nmi && !state)
		preempt_resched ();
}

preempt_state_t
preempt_enter_IRQ (void)
{
	return preempt_enter_generic (PREEMPT_IRQ_LEVEL_IRQ);
}

preempt_state_t
preempt_enter_NMI (void)
{
	return preempt_enter_generic (PREEMPT_IRQ_LEVEL_NMI);
}

void
preempt_leave_IRQ (preempt_state_t state)
{
	preempt_leave_generic (state, false);
}

void
preempt_leave_NMI (preempt_state_t state)
{
	preempt_leave_generic (state, true);
}
