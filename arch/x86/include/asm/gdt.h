/**
 * Layout of the Global Descriptor Table.
 * Copyright (C) 2024  dbstream
 */
#ifndef _ASM_GDT_H
#define _ASM_GDT_H 1

#define __USER32_CS 0x08
#define __USER32_DS 0x10
#define __USER64_DS 0x10
#define __USER64_CS 0x18
#define __KERNEL_DS 0x20
#define __KERNEL_CS 0x28
#define __GDT_TSS 0x30

#endif /* _ASM_GDT_H */
