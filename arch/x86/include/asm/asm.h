/**
 * Utilities for writing assembler source, and inline asm for x86 instructions.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#ifdef __ASSEMBLER__
#	define __HEAD .section ".head", "awx", @progbits
#	define __TEXT .section ".text", "ax", @progbits
#	define __RODATA .section ".rodata", "a", @progbits
#	define __DATA .section ".data", "aw", @progbits
#	define SYM_FUNC_BEGIN(name)		\
	.type name, @function;			\
name:;						\
	.cfi_startproc
#	define SYM_FUNC_END(name)		\
	.cfi_endproc;				\
	.size name, . - name
#	define SYM_DATA_BEGIN(name)		\
	.type name, @object;			\
name:
#	define SYM_DATA_END(name)		\
	.size name, . - name
#endif

#ifndef __ASSEMBLER__

#include <stdint.h>

static inline void
__invlpg (uintptr_t x)
{
	asm volatile ("invlpg (%0)" :: "r" (x));
}

static inline uint64_t
read_cr0 (void)
{
	uint64_t ret;
	asm volatile ("movq %%cr0, %0" : "=a" (ret) :: "memory");
	return ret;
}

static inline uint64_t
read_cr2 (void)
{
	uint64_t ret;
	asm volatile ("movq %%cr2, %0" : "=a" (ret) :: "memory");
	return ret;
}

static inline uint64_t
read_cr3 (void)
{
	uint64_t ret;
	asm volatile ("movq %%cr3, %0" : "=a" (ret) :: "memory");
	return ret;
}

static inline uint64_t
read_cr4 (void)
{
	uint64_t ret;
	asm volatile ("movq %%cr4, %0" : "=a" (ret) :: "memory");
	return ret;
}

static inline uint64_t
read_cr8 (void)
{
	uint64_t ret;
	asm volatile ("movq %%cr8, %0" : "=a" (ret) :: "memory");
	return ret;
}

static inline void
write_cr0 (uint64_t value)
{
	asm volatile ("movq %0, %%cr0" :: "a" (value) : "memory");
}

static inline void
write_cr3 (uint64_t value)
{
	asm volatile ("movq %0, %%cr3" :: "a" (value) : "memory");
}

static inline void
write_cr4 (uint64_t value)
{
	asm volatile ("movq %0, %%cr4" :: "a" (value) : "memory");
}

static inline void
write_cr8 (uint64_t value)
{
	asm volatile ("movq %0, %%cr8" :: "a" (value) : "memory");
}

static inline uint64_t
read_msr (uint32_t index)
{
	uint32_t high, low;
	asm volatile ("rdmsr" : "=d" (high), "=a" (low) : "c" (index) : "memory");
	return ((uint64_t) high << 32) | low;
}

static inline void
write_msr (uint32_t index, uint64_t value)
{
	uint32_t high = value >> 32;
	uint32_t low = value;
	asm volatile ("wrmsr" :: "d" (high), "a" (low), "c" (index) : "memory");
}

static inline void
raw_irq_disable (void)
{
	asm volatile ("cli" ::: "memory");
}

static inline void
raw_irq_enable (void)
{
	asm volatile ("sti" ::: "memory");
}

#endif
