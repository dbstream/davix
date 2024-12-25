/**
 * Support for Simultaneous Multiprocessing (SMP).
 * Copyright (C) 2024  dbstream
 */
#ifndef _DAVIX_SMP_H
#define _DAVIX_SMP_H 1

extern void
smp_init (void);

extern void
smpboot_init (void);

/**
 * Bring other CPUs online.
 */
extern void
smp_boot_cpus (void);

/**
 * Architectures call this function on freshly-booted CPUs after initializing
 * architecture-specific state and marking the CPU as online.
 */
extern void
smp_start_additional_cpu (void);

#endif /* _DAVIX_SMP_H */
