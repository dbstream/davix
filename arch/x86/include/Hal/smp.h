// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/Hal/smp.h
 * Simultaneous Multi-Processing (SMP) support routines.
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

/**
 * HalCurrentProcessor - get the index of the CPU this task currently runs on.
 */
static inline unsigned int HalCurrentProcessor(void)
{
	unsigned int cpu;
	asm volatile("movl %%gs:8, %0" : "=r"(cpu));
	return cpu;
}

