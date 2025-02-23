/**
 * Memory mapped I/O functions.
 * Copyright (C) 2025-present  dbstream
 */
#ifndef _ASM_MMIO_H
#define _ASM_MMIO_H 1

#include <davix/stdint.h>

static inline uint8_t
mmio_read8 (const void *p)
{
	uint8_t x;
	asm volatile ("movb (%1), %0" : "=r" (x) : "r" (p) : "memory");
	return x;
}

static inline uint16_t
mmio_read16 (const void *p)
{
	uint16_t x;
	asm volatile ("movw (%1), %0" : "=r" (x) : "r" (p) : "memory");
	return x;
}

static inline uint32_t
mmio_read32 (const void *p)
{
	uint32_t x;
	asm volatile ("movl (%1), %0" : "=r" (x) : "r" (p) : "memory");
	return x;
}

static inline uint64_t
mmio_read64 (const void *p)
{
	uint64_t x;
	asm volatile ("movq (%1), %0" : "=r" (x) : "r" (p) : "memory");
	return x;
}

static inline void
mmio_write8 (void *p, uint8_t x)
{
	asm volatile ("movb %0, (%1)" :: "r" (x), "r" (p) : "memory");
}

static inline void
mmio_write16 (void *p, uint16_t x)
{
	asm volatile ("movw %0, (%1)" :: "r" (x), "r" (p) : "memory");
}

static inline void
mmio_write32 (void *p, uint32_t x)
{
	asm volatile ("movl %0, (%1)" :: "r" (x), "r" (p) : "memory");
}

static inline void
mmio_write64 (void *p, uint64_t x)
{
	asm volatile ("movq %0, (%1)" :: "r" (x), "r" (p) : "memory");
}

#endif /* _ASM_MMIO_H */
