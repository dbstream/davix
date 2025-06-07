/**
 * SMP (Simultaneous Multiprocessing) system bringup.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

bool
arch_smp_boot_cpu (unsigned int cpu);

void
smp_boot_all_cpus (void);

void
smp_handle_call_on_one_ipi (void);

void
smp_call_on_cpu (unsigned int cpu, void (*fn)(void *), void *arg);
