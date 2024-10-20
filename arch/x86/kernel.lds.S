/**
 * x86 linker script.
 * Copyright (C) 2024  dbstream
 */
#include <asm/sections.h>

OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start32_multiboot2)

PHDRS
{
	header			PT_LOAD		FLAGS(7);
	text			PT_LOAD		FLAGS(5);
	rodata			PT_LOAD		FLAGS(4);
	data			PT_LOAD		FLAGS(6);
	cpulocal		PT_LOAD		FLAGS(6);
	init			PT_LOAD		FLAGS(7);
}

SECTIONS
{
	. = 0;

	__header_start = . + __KERNEL_START;

	.header		: AT(ADDR(.header))			{
		KEEP(*(.header))
	}							:header

	. = __KERNEL_START + SIZEOF(.header);
	. = ALIGN(__KERNEL_ALIGN);

	__header_end = .;
	__kernel_start = .;

	__text_start = .;
	.text		: AT(ADDR(.text) - __KERNEL_START)	{
		*(.text .text.*)
	}							:text
	. = ALIGN(__KERNEL_ALIGN);
	__text_end = .;

	__rodata_start = .;
	.rodata		: AT(ADDR(.rodata) - __KERNEL_START)	{
		*(.rodata .rodata.*)
	}							:rodata
	. = ALIGN(__KERNEL_ALIGN);
	__rodata_end = .;

	__data_start = .;
	.data		: AT(ADDR(.data) - __KERNEL_START)	{
		*(.data .data.*)
	}							:data

	.bss		: AT(ADDR(.bss) - __KERNEL_START)	{
		*(COMMON)
		*(.bss .bss.*)
	}							:data
	. = ALIGN(__KERNEL_ALIGN);
	__data_end = .;
	__cpulocal_virt_start = .;
	__bsp_cpulocal_offset = .;

	__cpulocal_phys_start = . - __KERNEL_START;

	.cpulocal 0	: AT(__cpulocal_phys_start)	{
		__cpulocal_start = .;
		KEEP(*(.cpulocal.fixed))
		*(.cpulocal)
		__cpulocal_end = .;
	}							:cpulocal
	. = __bsp_cpulocal_offset + SIZEOF(.cpulocal);
	. = ALIGN(__KERNEL_ALIGN);
	__cpulocal_virt_end = .;

	__init_start = .;
	.init		: AT(ADDR(.init) - __KERNEL_START)	{
		*(.init.text)
		*(.init.rodata)
		*(.init.data)
	}							:init

	. = ALIGN(__KERNEL_ALIGN);
	__init_end = .;
	__kernel_end = .;

	. = ASSERT(__cpulocal_offset == 0, "__cpulocal_offset must be 0");
	. = ASSERT(__this_cpu_id == 8, "__this_cpu_id must be 8");
	. = ASSERT(__preempt_state == 12, "__preempt_state must be 12");
	. = ASSERT(__current_task == 16, "__current_task must be 16");
	. = ASSERT(__stack_canary == 40, "__stack_canary must be 40");

	/DISCARD/	:					{
		*(.comment)
		*(.note .note.*)
		*(.eh_frame)
		*(.eh_frame_hdr)
	}
}
