/**
 * x86 support for Simultaneous Multiprocessing (SMP).
 * Copyright (C) 2024  dbstream
 */
#ifndef _ASM_SMP_H
#define _ASM_SMP_H 1

/* Get the currently running processor. */
static inline unsigned int
this_cpu_id (void)
{
	unsigned int id;
	asm ("movl %%gs:8, %0" : "=r" (id));
	return id;
}

extern void
x86_smp_init (void);

#endif /* _ASM_SMP_H */
