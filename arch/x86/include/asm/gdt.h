// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/asm/gdt.h
 * Global Descriptor Table constants.
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

#ifndef __ASSEMBLER__
extern "C" unsigned long __percpu_GDT[];
#endif

/*
 * This layout is mandated by the syscall and sysret mechanism.  In particular,
 * sysret requires __USER32_CS + 16 == __USERXX_DS + 8 == __USER64_CS, and
 * syscall requires __KERNEL_CS + 8 == __KERNEL_DS.
 */
#define __USER32_CS	(0x08 | 3)
#define __USER32_DS	(0x10 | 3)
#define __USER64_DS	(0x10 | 3)
#define __USER64_CS	(0x18 | 3)
#define __KERNEL_CS	0x20
#define __KERNEL_DS	0x28
#define __GDT_TSS	0x30
#define GDT_NUM_ENTRIES 8

