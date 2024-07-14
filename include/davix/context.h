/**
 * Interruption/preemption context tracking.
 * Copyright (C) 2024  dbstream
 */
#ifndef _DAVIX_CONTEXT_H
#define _DAVIX_CONTEXT_H 1

#include <davix/atomic.h>
#include <davix/stdbool.h>
#include <davix/stdint.h>
#include <asm/cpulocal.h>
#include <asm/irq.h>

/**
 * Architectures need to provide get_preempt_state(), preempt_off(),
 * preempt_on_atomic(), and set_preempt_state(). These must be atomic
 * wrt the current CPU.
 */
#include <asm/preempt_state.h>

/**
 * This bit is cleared when rescheduling is needed. Users do the
 * equivalent of:
 *	if (!preempt_state)
 *		preempt_resched ();
 * to reschedule when necessary.
 */
#define PREEMPT_NO_RESCHED (1U << 29)

#define PREEMPT_IRQ_LEVEL_MASK (3U << 30)
#define PREEMPT_IRQ_LEVEL_IRQ (2U << 30)
#define PREEMPT_IRQ_LEVEL_NMI (3U << 30)

#define in_irq() (get_preempt_state () >= PREEMPT_IRQ_LEVEL_IRQ)
#define in_nmi() (get_preempt_state () >= PREEMPT_IRQ_LEVEL_NMI)

extern void
preempt_resched (void);

/**
 * Critical regions are defined by preempt_off/preempt_on pairs. These increment
 * and decrement the preempt_state.
 */

static inline void
preempt_on (void)
{
	if (preempt_on_atomic ())
		preempt_resched ();
}

/**
 * These functions are called to mark IRQ and NMI entry/exit.
 *
 * The return value from preempt_enter_foo must be passed to preempt_leave_foo.
 */

extern preempt_state_t
preempt_enter_IRQ (void);

extern preempt_state_t
preempt_enter_NMI (void);

extern void
preempt_leave_IRQ (preempt_state_t state);

extern void
preempt_leave_NMI (preempt_state_t state);

#endif /* DAVIX_CONTEXT_H */
