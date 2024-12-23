/**
 * Timestamp Counter (TSC)
 * Copyright (C) 2024  dbstream
 */
#ifndef _ASM_TSC_H
#define _ASM_TSC_H 1

#include <davix/stdbool.h>
#include <davix/time.h>

extern unsigned long tsc_hz;

extern bool
x86_init_tsc (bool use_hpet);

extern nsecs_t
x86_tsc_nsecs (void);

static inline void
__rdtsc (unsigned int *edx, unsigned int *eax)
{
	asm volatile ("rdtsc" : "=d" (*edx), "=a" (*eax));
}

static inline unsigned long
rdtsc (void)
{
	unsigned int edx, eax;
	__rdtsc (&edx, &eax);
	return ((unsigned long) edx << 32) | eax;
}

extern bool
x86_time_is_tsc (void);

/**
 * Synchronize the TSC on a newly-booted CPU. This function should be called on
 * the control CPU.
 */
extern void
x86_synchronize_tsc_control (void);

/**
 * Synchronize the TSC on a newly-booted CPU. This function should be called on
 * the victim CPU.
 */
extern void
x86_synchronize_tsc_victim (void);

#endif /* _ASM_TSC_H */
