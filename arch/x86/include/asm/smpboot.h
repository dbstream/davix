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

#endif /* __ASM_SMPBOOT_H */
