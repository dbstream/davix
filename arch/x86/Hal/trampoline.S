// SPDX-License-Identifier: GPL-3.0
/*
 * File: Hal/trampoline.S
 * Trampoline contents.
 *
 * Copyright (C) 2025  dbstream
 */
	.file "trampoline.S"

#include <asm/creg_bits.h>
#include <asm/msr_bits.h>	

	.section ".rodata", "a", @progbits

	.global trampoline_page
trampoline_page:
	.code16
	.byte 0xfa		/* cli */
	.byte 0x90		/* nop */
	.byte 0x66, 0xb8	/* mov imm32, %eax */
	.long 0xdeadbeef	/* imm32 */

	movl %eax, %ebp
	shrl $4, %eax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %gs
	movw %ax, %ss

	leal temporary_gdt - trampoline_page, %edx
	addl %ebp, %edx
	movl %edx, (temporary_gdt_address - trampoline_page)
	lgdt (temporary_gdtr - trampoline_page)

	movl $(__CR0_PE | __CR0_NW | __CR0_ET | __CR0_CD), %eax
	movl %eax, %cr0

	movl $0x1000, %esp
	leal (1f - trampoline_page)(%ebp), %eax
	pushl $0x10
	pushl %eax
	lretl

1:	.code32
	movl $0x8, %eax
	movl %eax, %ds
	movl %eax, %es
	movl %eax, %fs
	movl %eax, %gs
	movl %eax, %ss

	leal 0x1000(%ebp), %esp

	/*
	 * The trampoline setup stores the desired states for %cr0, MSR_EFER,
	 * %cr0 and %cr3 at the following offsets from the start of the
	 * trampoline page:
	 *	+0x800	%cr4
	 *	+0x804	MSR_EFER
	 *	+0x808	%cr0
	 *	+0x80c	%cr3
	 */

	movl 0x800(%ebp), %eax
	movl %eax, %cr4

	movl 0x80c(%ebp), %eax
	movl %eax, %cr3

	movl $MSR_EFER, %ecx
	movl 0x804(%ebp), %eax
	xorl %edx, %edx
	wrmsr

	movl 0x808(%ebp), %eax
	movl %eax, %cr0		/* This instruction enables long mode.  */

	leal (1f - trampoline_page)(%ebp), %eax
	pushl $0x20
	pushl %eax
	lretl

1:	.code64
	movl $0x18, %eax
	movl %eax, %ds
	movl %eax, %es
	movl %eax, %fs
	movl %eax, %gs
	movl %eax, %ss
	movq $__startup_ap, %rax
	jmp *%rax

	.balign 64
temporary_gdt:
	.quad 0
	.quad 0x00cf92000000ffff	/* 32-bit data */
	.quad 0x00cf9a000000ffff	/* 32-bit code */
	.quad 0x00af93000000ffff	/* 64-bit data */
	.quad 0x00af9b000000ffff	/* 64-bit code */
temporary_gdt_end:
	.word 0
temporary_gdtr:
	.word temporary_gdt_end - temporary_gdt - 1
temporary_gdt_address:
	.long 0

	.size trampoline_page, . - trampoline_page

	.globl trampoline_page_end
trampoline_page_end:

