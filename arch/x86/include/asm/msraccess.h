// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/asm/msraccess.h
 * Functions for writing and reading Model Specific Registers (MSRs).
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

static inline unsigned long
read_msr (unsigned int index)
{
	unsigned int high, low;
	asm volatile ("rdmsr" : "=d" (high), "=a" (low) : "c" (index) : "memory");
	return ((unsigned long) high << 32) | low;
}

static inline void
write_msr (unsigned int index, unsigned long value)
{
	unsigned int high = value >> 32;
	unsigned int low = value;
	asm volatile ("wrmsr" :: "d" (high), "a" (low), "c" (index) : "memory");
}

