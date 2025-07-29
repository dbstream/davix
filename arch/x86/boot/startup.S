// SPDX-License-Identifier: GPL-3.0
/*
 * File: boot/startup.S
 * Hal entry point.
 *
 * Copyright (C) 2025  dbstream
 */
	.file "stateup.S"
	.code64

#include <asm/creg_bits.h>
#include <asm/gdt.h>
#include <asm/msr_bits.h>

	.text

	/**
	 * __startup_bsp - cold boot entry point.
	 * @%rdi: pointer to multiboot information struct
	 * @%rsi: kernel load offset in physical address space
	 */
	.globl __startup_bsp
	.type __startup_bsp, @function
__startup_bsp:
	.cfi_startproc
	.cfi_undefined rip

	/*
	 * Setup the BSP's pointer to the .percpu section.
	 */
	movl $MSR_GSBASE, %ecx
	leaq __percpu_start(%rip), %rax
	movq %rax, %rdx
	shrq $32, %rdx
	wrmsr
	movq %rax, %gs:0

	/*
	 * Load the permanent GDT.
	 */
	leaq __percpu_GDT(%rip), %rax
	addq %gs:0, %rax
	subq $16, %rsp
	movw $0x3f, (%rsp)
	movq %rax, 2(%rsp)
	lgdt (%rsp)
	addq $16, %rsp

	/*
	 * Reload the segment registers, preserving MSR_GSBASE.
	 */
	swapgs
	movl $__KERNEL_DS, %eax
	movl %eax, %ds
	movl %eax, %es
	movl %eax, %fs
	movl %eax, %gs
	movl %eax, %ss
	swapgs

	/*
	 * Enable the global bit in PTEs.
	 */
	movq %cr4, %rax
	orq $__CR4_PGE, %rax
	movq %rax, %cr4

	/*
	 * Return to _start on __KERNEL_CS.
	 */
	pushq $__KERNEL_CS
	pushq $_start
	lretq

	.cfi_endproc
	.size __startup_bsp, . - __startup_bsp

	.globl _start
	.type _start, @function
_start:
	.cfi_startproc
	.cfi_undefined rip

	cld
	call HalStartKernel
	nop

	.cfi_endproc
	.size _start, . - _start

	.section ".percpu.fixed", "aw", @progbits

	.globl __pcpu_fixed
	.type __pcpu_fixed, @object
__pcpu_fixed:
	.fill 64, 1, 0
	.size __pcpu_fixed, . - __pcpu_fixed

	.section ".percpu", "aw", @progbits
	.balign 64
	.globl __percpu_GDT
	.type __percpu_GDT, @object
__percpu_GDT:
	.quad 0				/** NULL entry  */
	.quad 0x00cffb000000ffff	/** __USER32_CS  */
	.quad 0x00cff3000000ffff	/** __USERXX_DS  */
	.quad 0x00affb000000ffff	/** __USER64_CS  */
	.quad 0x00af9b000000ffff	/** __KERNEL_CS  */
	.quad 0x00af93000000ffff	/** __KERNEL_DS  */
	.quad 0
	.quad 0
	.size __percpu_GDT, . - __percpu_GDT

