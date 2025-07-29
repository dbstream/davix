// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/asm/creg_access.h
 * Functions for reading and writing Control Registers (CRn).
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

static inline unsigned long
read_cr0 (void)
{
	unsigned long ret;
	asm volatile ("movq %%cr0, %0" : "=a" (ret) :: "memory");
	return ret;
}

static inline unsigned long
read_cr2 (void)
{
	unsigned long ret;
	asm volatile ("movq %%cr2, %0" : "=a" (ret) :: "memory");
	return ret;
}

static inline unsigned long
read_cr3 (void)
{
	unsigned long ret;
	asm volatile ("movq %%cr3, %0" : "=a" (ret) :: "memory");
	return ret;
}

static inline unsigned long
read_cr4 (void)
{
	unsigned long ret;
	asm volatile ("movq %%cr4, %0" : "=a" (ret) :: "memory");
	return ret;
}

static inline unsigned long
read_cr8 (void)
{
	unsigned long ret;
	asm volatile ("movq %%cr8, %0" : "=a" (ret) :: "memory");
	return ret;
}

static inline void
write_cr0 (unsigned long value)
{
	asm volatile ("movq %0, %%cr0" :: "a" (value) : "memory");
}

static inline void
write_cr3 (unsigned long value)
{
	asm volatile ("movq %0, %%cr3" :: "a" (value) : "memory");
}

static inline void
write_cr4 (unsigned long value)
{
	asm volatile ("movq %0, %%cr4" :: "a" (value) : "memory");
}

static inline void
write_cr8 (unsigned long value)
{
	asm volatile ("movq %0, %%cr8" :: "a" (value) : "memory");
}

