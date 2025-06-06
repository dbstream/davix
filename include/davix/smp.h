/**
 * SMP (Simultaneous Multiprocessing) system bringup.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

bool
arch_smp_boot_cpu (unsigned int cpu);

void
smp_boot_all_cpus (void);
