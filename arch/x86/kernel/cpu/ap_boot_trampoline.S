/**
 * AP boot trampoline for SMP system bringup.
 * Copyright (C) 2024  dbstream
 *
 * This code gets copied to the smpboot_page by arch_smpboot_init.
 */
	.file "ap_boot_trampoline.S"

#include <asm/cregs.h>
#include <asm/msr.h>
#include <asm/sections.h>

.macro HCF
	movb $'X', %al
	outb %al, $0xe9
10069:	hlt
	jmp 10069b
.endm

	__RODATA


	.balign 64
trampoline_start:

	.globl x86_ap_boot_trampoline
	.type x86_ap_boot_trampoline, @object
x86_ap_boot_trampoline:
	.code16
	.byte 0xfa		/* cli */
	.byte 0x90		/* nop */
	.byte 0x66, 0xb8	/* movl $constant, %eax */
	.long 0xdeadbeef	/* constant placeholder */

	movl %eax, %ebp
	shr $4, %eax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %gs
	movw %ax, %ss

	leal (temporary_gdt - trampoline_start)(%ebp), %edx
	movl %edx, (temporary_gdt_address - trampoline_start)
	lgdt (temporary_gdtr - trampoline_start)

	movl $(__CR0_PE | __CR0_NW | __CR0_ET | __CR0_CD), %eax
	movl %eax, %cr0

	movl 0x1000, %esp
	leal (1f - trampoline_start)(%ebp), %eax
	pushl $0x10
	pushl %eax
	lretl

1:	/** <-- (!) Start of 32-bit execution.  */
	.code32
	movl $0x8, %eax
	movl %eax, %ds
	movl %eax, %es
	movl %eax, %fs
	movl %eax, %gs
	movl %eax, %ss

	leal 0x1000(%ebp), %esp

	/**
	 * smpboot_init stores the following values for us:
	 *   +0x800  __cr4_state   The desired value in %cr4.
	 *   +0x804  __efer_state  The desired value in MSR_EFER.
	 *   +0x808  __cr0_state   The desired value in %cr0.  (This enables paging.)
	 *   +0x80c                The desired value in %cr3.
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
	movl %eax, %cr0		/** <-- (!) This instruction enters long-mode. */

	leal (1f - trampoline_start)(%ebp), %eax
	pushl $0x20
	pushl %eax
	lretl			/** <-- (!) This instruction enters 64-bit mode. */

1:	/** <-- (!) Start of 64-bit execution.  */
	.code64
	movl $0x18, %eax
	movl %eax, %ds
	movl %eax, %es
	movl %eax, %fs
	movl %eax, %gs
	movl %eax, %ss

	movb $'X', %al
	outb %al, $0xe9
1:	hlt
	jmp 1b

	.balign 64
temporary_gdt:
	.quad 0				/* 0x00: null entry */
	.quad 0x00cf92000000ffff	/* 0x08: 32-bit data */
	.quad 0x00cf9a000000ffff	/* 0x10: 32-bit code */
	.quad 0x00af93000000ffff	/* 0x18: 64-bit data */
	.quad 0x00af9b000000ffff	/* 0x20: 64-bit code */
temporary_gdt_end:
	.word 0
temporary_gdtr:
	.word temporary_gdt_end - temporary_gdt - 1
temporary_gdt_address:
	.long 0

x86_ap_boot_trampoline_end:
	.size x86_ap_boot_trampoline, . - x86_ap_boot_trampoline
	.globl x86_ap_boot_trampoline_end
