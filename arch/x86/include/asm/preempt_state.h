/**
 * x86 implementation of preempt_state
 * Copyright (C) 2024  dbstream
 *
 * We store preempt_state in the fixed CPU-local section, accessible as %gs:12.
 */
#ifndef _ASM_PREEMPT_STATE_H
#define _ASM_PREEMPT_STATE_H 1

#include <davix/stdbool.h>

typedef unsigned int preempt_state_t;

static inline preempt_state_t
get_preempt_state (void)
{
	preempt_state_t state;
	asm volatile ("movl %%gs:12, %0" : "=r" (state));
	return state;
}

static inline void
set_preempt_state (preempt_state_t state)
{
	asm volatile ("movl %0, %%gs:12" :: "r" (state));
}

static inline void
preempt_off (void)
{
	asm volatile ("incl %%gs:12" ::: "cc");
}

static inline bool
preempt_on_atomic (void)
{
	bool zero;
	asm volatile ("decl %%gs:12" : "=@cce" (zero) :: "cc");
	return zero;
}

#endif /* _ASM_PREEMPT_STATE_H */
