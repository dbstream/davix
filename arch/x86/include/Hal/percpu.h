// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/Hal/percpu.h
 * Per-CPU variables support.
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

namespace Hal {

extern unsigned long percpu_offsets[];

template<class T>
static inline T read__seg_gs [[gnu::always_inline]] (T *ptr)
{
	T value;
	if constexpr (sizeof(T) == 1) {
		asm volatile("movb %%gs:%1, %0" : "=r"(value) : "m"(*ptr) : "memory");
	} else if constexpr (sizeof(T) == 2) {
		asm volatile("movw %%gs:%1, %0" : "=r"(value) : "m"(*ptr) : "memory");
	} else if constexpr (sizeof(T) == 4) {
		asm volatile("movl %%gs:%1, %0" : "=r"(value) : "m"(*ptr) : "memory");
	} else if constexpr (sizeof(T) == 8) {
		asm volatile("movq %%gs:%1, %0" : "=r"(value) : "m"(*ptr) : "memory");
	} else {
		asm volatile("addq %%gs:0, %0" : "+r"(ptr) :: "cc");
		value = *ptr;
	}
	return value;
}

template<class T>
static inline void write__seg_gs [[gnu::always_inline]] (T *ptr, T value)
{
	if constexpr(sizeof(T) == 1) {
		asm volatile("movb %0, %%gs:%1" :: "Nr"(value), "m"(*ptr) : "memory");
	} else if constexpr(sizeof(T) == 2) {
		asm volatile("movw %0, %%gs:%1" :: "Nr"(value), "m"(*ptr) : "memory");
	} else if constexpr(sizeof(T) == 4) {
		asm volatile("movl %0, %%gs:%1" :: "Nr"(value), "m"(*ptr) : "memory");
	} else if constexpr(sizeof(T) == 8) {
		asm volatile("movq %0, %%gs:%1" :: "Nr"(value), "m"(*ptr) : "memory");
	} else {
		asm volatile("addq %%gs:0, %0" : "+r"(ptr) :: "cc");
		*ptr = value;
	}
}

template<class T> using PerCPUStorage = T;

#define DEFINE_PERCPU(type, name) \
	::Hal::PerCPUStorage<type> name [[gnu::section (".percpu")]];

}

/**
 * HalReadPerCPU - read the value of a per-CPU variable on the current CPU.
 * @storage: reference to per-CPU variable storage
 * Returns the value at @storage.
 */
template<class T>
static inline T HalReadPerCPU(T &storage)
{
	return Hal::read__seg_gs(&storage);
}

/**
 * HalWritePerCPU - write to a per-CPU variable on the current CPU.
 * @storage: reference to per-CPU variable storage
 * @value: value to store in @storage
 */
template<class T>
static inline void HalWritePerCPU(T &storage, T value)
{
	Hal::write__seg_gs(&storage, value);
}

/**
 * HalPtrThisCPU - convert a per-CPU reference to an absolute pointer.
 * @storage: reference to per-CPU variable storage
 * Returns an absolute pointer, which can be read and written to by other CPUs,
 * to the provided per-CPU variable.
 */
template<class T>
static inline T *HalPtrThisCpu(T &storage)
{
	T *ptr = &storage;
	asm volatile("addq %%gs:0, %0" : "+r"(ptr) :: "cc");
	return ptr;
}

/**
 * HalPtrPerCPU - convert a per-CPU reference to an absolute pointer.
 * @storage: reference to per-CPU variable storage
 * @cpu: identifier of the percpu region to use
 * Returns an absolute pointer, which can be read and written to by other CPUs,
 * to the provided per-CPU variable.
 */
template<class T>
static inline T *HalPtrPerCPU(T &storage, unsigned int cpu)
{
	return (T *) ((unsigned long) &storage + Hal::percpu_offsets[cpu]);
}

/**
 * HalIncrementPerCPU - increment an unsigned int on the current CPU.
 * @storage: reference to per-CPU variable storage
 */
static inline void HalIncrementPerCPU(unsigned int &storage)
{
	asm volatile("incl %%gs:%0" :: "m"(storage) : "cc", "memory");
}

/**
 * HalDecrementPerCPU - decrement an unsigned int on the current CPU.
 * @storage: reference to per-CPU variable storage
 */
static inline void HalDecrementPerCPU(unsigned int &storage)
{
	asm volatile("decl %%gs:%0" :: "m"(storage) : "cc", "memory");
}

/**
 * HalDecrementAndTestPerCPU - decrement an unsigned int on the current CPU.
 * @storage: reference to per-CPU variable storage
 * Returns true if the unsigned int became zero.
 */
static inline bool HalDecrementAndTestPerCPU(unsigned int &storage)
{
	bool is_zero;
	asm volatile("decl %%gs:%1" : "=@cce"(is_zero) : "m"(storage) : "cc", "memory");
	return is_zero;
}

void HalInitializePerCPUVariables(unsigned int cpu);

void HalAllocateAndInitializePerCPUVariables(unsigned int cpu);

#define __HAL_PERCPU_CALLBACK_NAME()					\
	__HAL_PERCPU_CALLBACK_CONCAT(__HalPCPUCallback_, __COUNTER__)
#define __HAL_PERCPU_CALLBACK_CONCAT(a, b) __HAL_PERCPU_CALLBACK_CONCAT2(a, b)
#define __HAL_PERCPU_CALLBACK_CONCAT2(a, b) a ## b

#define __HAL_PERCPU_CALLBACK(name, cpu)				\
static void name(unsigned int);						\
static void __HAL_PERCPU_CALLBACK_CONCAT(__generate_dispatch_, name)	\
	[[gnu::naked, gnu::used]] (void)				\
{									\
	asm volatile(".pushsection .percpu_callbacks; .quad %p0; .popsection" \
			:: "s" (name));					\
}									\
static void name(unsigned int cpu)

/**
 * HAL_PERCPU_CALLBACK - define a callback to run once per CPU.
 * @cpu: variable name to substitute for CPU number
 *
 * The callback bodies are bodies for a function of type void(unsigned int).
 */
#define HAL_PERCPU_CALLBACK(cpu)					\
	__HAL_PERCPU_CALLBACK(__HAL_PERCPU_CALLBACK_NAME(), cpu)

