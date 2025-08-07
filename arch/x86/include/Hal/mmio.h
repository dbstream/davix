// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/Hal/mmio.h
 * MMIO Helpers.
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

#include <stdint.h>

static inline uint8_t HalMMIORead8(volatile uint8_t *ptr)
{
	uint8_t value;
	asm volatile("movb %1, %0" : "=r"(value) : "m"(*ptr) : "memory");
	return value;
}

static inline uint16_t HalMMIORead16(volatile uint16_t *ptr)
{
	uint16_t value;
	asm volatile("movw %1, %0" : "=r"(value) : "m"(*ptr) : "memory");
	return value;
}

static inline uint32_t HalMMIORead32(volatile uint32_t *ptr)
{
	uint32_t value;
	asm volatile("movl %1, %0" : "=r"(value) : "m"(*ptr) : "memory");
	return value;
}

static inline uint64_t HalMMIORead64(volatile uint64_t *ptr)
{
	uint64_t value;
	asm volatile("movq %1, %0" : "=r"(value) : "m"(*ptr) : "memory");
	return value;
}

static inline void HalMMIOWrite8(volatile uint8_t *ptr, uint8_t value)
{
	asm volatile("movb %0, %1" :: "Nr"(value), "m"(*ptr) : "memory");
}

static inline void HalMMIOWrite16(volatile uint16_t *ptr, uint16_t value)
{
	asm volatile("movw %0, %1" :: "Nr"(value), "m"(*ptr) : "memory");
}

static inline void HalMMIOWrite32(volatile uint32_t *ptr, uint32_t value)
{
	asm volatile("movl %0, %1" :: "Nr"(value), "m"(*ptr) : "memory");
}

static inline void HalMMIOWrite64(volatile uint64_t *ptr, uint64_t value)
{
	asm volatile("movq %0, %1" :: "Nr"(value), "m"(*ptr) : "memory");
}

