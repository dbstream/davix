/**
 * Kernel image ELF sections and build constants.
 * Copyright (C) 2024  dbstream
 */
#ifndef _ASM_SECTIONS_H
#define _ASM_SECTIONS_H 1

#define __KERNEL_START	0xffffffff80000000
#define __KERNEL_ALIGN	0x1000

#ifdef __ASSEMBLER__
#define __section(sname, flags, stype) .section sname, flags, @stype
#else /* __ASSEMBLER__ */
#define __section(sname, flags, stype) __attribute__((__section__(sname)))
#endif /* !__ASSEMBLER__ */

#define __HEAD __section(".header", "awx", progbits)

#define __TEXT __section(".text", "ax", progbits)
#define __RODATA __section(".rodata", "a", progbits)
#define __DATA __section(".data", "aw", progbits)

#define __DATA_PAGE_ALIGNED __section(".data.page_aligned", "aw", progbits)

#define __INIT_TEXT __section(".init.text", "ax", progbits)
#define __INIT_RODATA __section(".init.rodata", "a", progbits)
#define __INIT_DATA __section(".init.data", "awx", progbits)

#define __CPULOCAL_FIXED __section(".cpulocal.fixed", "aw", progbits)
#define __CPULOCAL __section(".cpulocal", "aw", progbits)

#ifndef __ASSEMBLER__

extern char __header_start[];
extern char __header_end[];

extern char __kernel_start[];

extern char __text_start[];
extern char __text_end[];

extern char __rodata_start[];
extern char __rodata_end[];

extern char __data_start[];
extern char __data_end[];

extern char __cpulocal_virt_start[];
extern char __cpulocal_virt_end[];

extern char __init_start[];
extern char __init_end[];

extern char __kernel_end[];

#endif /* !__ASSEMBLER__ */
#endif /* _ASM_SECTIONS_H */
