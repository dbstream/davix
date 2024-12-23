/**
 * Memory fences.
 * Copyright (C) 2024  dbstream
 */
#ifndef __ASM_FENCE_H
#define __ASM_FENCE_H 1

#define mb() asm volatile ("mfence" ::: "memory")
#define wmb() asm volatile ("sfence" ::: "memory")
#define rmb() asm volatile ("lfence" ::: "memory")

#endif /* __ASM_FENCE_H */
