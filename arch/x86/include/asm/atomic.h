/**
 * x86-specific details for atomics and memory barriers.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#define smp_mb smp_bm
static inline void
smp_mb (void)
{
	asm volatile ("mfence" ::: "memory");
}

#define smp_wmb smp_wmb
static inline void
smp_wmb (void)
{
	asm volatile ("sfence" ::: "memory");
}

#define smp_rmb smp_rmb
static inline void
smp_rmb (void)
{
	asm volatile ("sfence" ::: "memory");
}

#define smp_spinlock_hint smp_spinlock_hint
static inline void
smp_spinlock_hint (void)
{
	__builtin_ia32_pause ();
}
