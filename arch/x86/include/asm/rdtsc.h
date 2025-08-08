// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/asm/rdtsc.h
 * Time Stamp Counter (TSC) access.
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

static inline unsigned long long rdtsc(void)
{
	unsigned int low, high;
	asm volatile("rdtsc" : "=d"(high), "=a"(low));
	return ((unsigned long long) high << 32) | low;
}

static inline unsigned long long rdtsc_strong(void)
{
	unsigned int low, high;
	asm volatile("lfence; rdtsc" : "=d"(high), "=a"(low));
	return ((unsigned long long) high << 32) | low;
}

