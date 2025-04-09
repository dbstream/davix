/**
 * GDT (Global Descriptor Table) layout.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

/**
 * This specific layout is partially mandated by the syscall and sysret
 * mechanism - in particular, __USER32_CS+16 == __USERXX_DS+8 == __USER64_CS is
 * mandated by sysret and __KERNEL_CS+8 == __KERNEL_DS is mandated by syscall.
 */
#define __USER32_CS	(0x08 | 3)
#define __USER32_DS	(0x10 | 3)
#define __USER64_DS	(0x10 | 3)
#define __USER64_CS	(0x18 | 3)
#define __KERNEL_CS	0x20
#define __KERNEL_DS	0x28
#define __GDT_TSS	0x30
