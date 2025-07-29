// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/asm/atomic.h
 * Architecture-specific overrides for atomic operations and memory barriers.
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

#define smp_rmb() do { asm volatile("lfence" ::: "memory"); } while (0)
#define smp_wmb() do { asm volatile("sfence" ::: "memory"); } while (0)
#define smp_mb() do { asm volatile("mfence" ::: "memory"); } while (0)

#define smp_spinwait_hint() do { asm volatile("pause" ::: "memory"); } while (0)

