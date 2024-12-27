/**
 * Bringup of other processors on SMP systems.
 * Copyright (C) 2024  dbstream
 */
#ifndef __ASM_SMPBOOT_H
#define __ASM_SMPBOOT_H 1

#include <davix/errno.h>

/**
 * Prepare the system for SMPBOOT. This only needs to be called once.
 */
extern errno_t
arch_smpboot_init (void);

extern errno_t
arch_smp_boot_cpu (unsigned int cpu);

/**
 * Resynchronize the timestamp counter of all CPUs with the current CPU.
 * This has to be done under the smpboot lock, which is why it is here.
 */
extern void
x86_smp_resynchronize_tsc (void);

#endif /* __ASM_SMPBOOT_H */
