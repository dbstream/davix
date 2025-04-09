/**
 * Kernel linker script for x86.
 * Copyright (C) 2025-present  dbstream
 */
#define KERNEL_START 0xffffffff80000000
#define KERNEL_ALIGN 0x1000

OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start32_multiboot2)

PHDRS
{
	head		PT_LOAD		FLAGS(7);
	text		PT_LOAD		FLAGS(5);
	rodata		PT_LOAD		FLAGS(4);
	data		PT_LOAD		FLAGS(6);
	percpu		PT_LOAD		FLAGS(6);
}

SECTIONS
{
	. = 0;

	__kernel_start = . + KERNEL_START;
	__head_start = . + KERNEL_START;
	.head		: AT(ADDR(.head))			{
		KEEP(*(.head))
	}							:head
	. = KERNEL_START + SIZEOF(.head);
	. = ALIGN(KERNEL_ALIGN);
	__head_end = .;
	__text_start = .;
	.text		: AT(ADDR(.text) - KERNEL_START)	{
		*(.text .text.*)
	}							:text
	. = ALIGN(KERNEL_ALIGN);
	__text_end = .;
	__rodata_start = .;
	.rodata		: AT(ADDR(.rodata) - KERNEL_START)	{
		*(.rodata .rodata.*)
		. = ALIGN(8);
		__pcpu_constructors_start = .;
		KEEP(*(.pcpu_constructors))
		__pcpu_constructors_end = .;
	}							:rodata
	. = ALIGN(KERNEL_ALIGN);
	__rodata_end = .;
	__data_start = .;
	.data		: AT(ADDR(.data) - KERNEL_START)	{
		*(.data .data.*)
	}							:data
	.bss		: AT(ADDR(.bss) - KERNEL_START)		{
		*(COMMON)
		*(.bss .bss.*)
	}							:data
	. = ALIGN(KERNEL_ALIGN);
	__data_end = .;
	__percpu_start = .;
	.percpu 0	: AT(__percpu_start - KERNEL_START)	{
		KEEP(*(.percpu.fixed))
		*(.percpu)
	}							:percpu
	. = __percpu_start + SIZEOF(.percpu);
	. = ALIGN(KERNEL_ALIGN);
	__percpu_end = .;
	__kernel_end = .;

	__multiboot_bss_end = __kernel_end - KERNEL_START;

	. = ASSERT(__pcpu_fixed == 0, "__pcpu_fixed is not at address zero.");

	/DISCARD/ : {
		*(.comment)
		*(.eh_frame)
		*(.note .note.*)
	}
}
