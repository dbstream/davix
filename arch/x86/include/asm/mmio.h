/**
 * MMIO helpers.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#include <stdint.h>

static inline uint8_t
mmio_read8 (volatile void *address)
{
	uint8_t value;
	asm volatile ("movb %1, %0" : "=r" (value) : "m" (*(volatile uint8_t *) address) : "memory");
	return value;
}

static inline uint16_t
mmio_read16 (volatile void *address)
{
	uint16_t value;
	asm volatile ("movw %1, %0" : "=r" (value) : "m" (*(volatile uint16_t *) address) : "memory");
	return value;
}

static inline uint32_t
mmio_read32 (volatile void *address)
{
	uint32_t value;
	asm volatile ("movl %1, %0" : "=r" (value) : "m" (*(volatile uint32_t *) address) : "memory");
	return value;
}

static inline uint64_t
mmio_read64 (volatile void *address)
{
	uint64_t value;
	asm volatile ("movq %1, %0" : "=r" (value) : "m" (*(volatile uint64_t *) address) : "memory");
	return value;
}

static inline void
mmio_write8 (volatile void *address, uint8_t value)
{
	asm volatile ("movb %0, %1" :: "r" (value), "m" (*(volatile uint8_t *) address) : "memory");
}

static inline void
mmio_write16 (volatile void *address, uint16_t value)
{
	asm volatile ("movw %0, %1" :: "r" (value), "m" (*(volatile uint16_t *) address) : "memory");
}

static inline void
mmio_write32 (volatile void *address, uint32_t value)
{
	asm volatile ("movl %0, %1" :: "r" (value), "m" (*(volatile uint32_t *) address) : "memory");
}

static inline void
mmio_write64 (volatile void *address, uint64_t value)
{
	asm volatile ("movq %0, %1" :: "r" (value), "m" (*(volatile uint64_t *) address) : "memory");
}

static inline volatile void *
mmio_ptr_offset (volatile void *ptr, uintptr_t offset)
{
	return (volatile void *) ((uintptr_t) ptr + offset);
}
