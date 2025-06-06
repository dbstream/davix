/**
 * _start, __startup_bsp, __startup_ap
 * Copyright (C) 2025-present  dbstream
 */
	.file "startup.S"
	.code64

#include <asm/asm.h>
#include <asm/creg_bits.h>
#include <asm/gdt.h>
#include <asm/msr_bits.h>

	__TEXT

	.globl __startup_bsp
SYM_FUNC_BEGIN(__startup_bsp)
	.cfi_undefined rip

	/** Setup the pointer to .percpu on the BSP.  */
	movl $MSR_GSBASE, %ecx
	leaq __percpu_start(%rip), %rax
	movq %rax, %rdx
	shrq $32, %rdx
	wrmsr
	movq %rax, %gs:0

	/** Load the permanent GDT.  */
	leaq percpu_GDT(%rip), %rax
	addq %gs:0, %rax 
	subq $16, %rsp
	movw $0x3f, (%rsp)
	movq %rax, 2(%rsp)
	lgdt (%rsp)
	addq $16, %rsp

	/** Reload the segment registers.  */
	swapgs		/** Changing %gs clobbers GSBASE, so save it.  */
	movl $__KERNEL_DS, %eax
	movl %eax, %ds
	movl %eax, %es
	movl %eax, %fs
	movl %eax, %gs
	movl %eax, %ss
	swapgs		/** Restore the saved GSBASE.  */

	/** Enable the global bit in PTEs.  */
	movq %cr4, %rax
	orq $__CR4_PGE, %rax
	movq %rax, %cr4

	/** Return to _start on __KERNEL_CS.  */
	pushq $__KERNEL_CS
	pushq $_start
	lretq
SYM_FUNC_END(__startup_bsp)

	.globl __startup_ap
SYM_FUNC_BEGIN(__startup_ap)
	.cfi_undefined rip

	/** Setup the stack pointer.  */
	movq __ap_startup_rsp(%rip), %rsp

	/** Call C code to announce ourselves and setup percpu.  */
	call x86_smpboot_setup_percpu

	/** Load the permanent GDT.  */
	leaq percpu_GDT(%rip), %rax
	addq %gs:0, %rax
	subq $16, %rsp
	movw $0x3f, (%rsp)
	movq %rax, 2(%rsp)
	lgdt (%rsp)
	addq $16, %rsp

	/** Reload the segment registers.  */
	swapgs		/** Changing %gs clobbers GSBASE, so save it.  */
	movl $__KERNEL_DS, %eax
	movl %eax, %ds
	movl %eax, %es
	movl %eax, %fs
	movl %eax, %gs
	movl %eax, %ss
	swapgs		/** Restore the saved GSBASE.  */

	/** Return to start on __KERNEL_CS.  */
	pushq $__KERNEL_CS
	pushq $_start
	lretq
SYM_FUNC_END(__startup_ap)

	.balign 16
	.globl _start
SYM_FUNC_BEGIN(_start)
	.cfi_undefined rip

	cld

	movq __startup_rip(%rip), %rax
	call *%rax
	nop
SYM_FUNC_END(_start)

	__DATA

	.balign 16
	.globl __startup_rip
SYM_DATA_BEGIN(__startup_rip)
	.quad x86_start_kernel
SYM_DATA_END(__startup_rip)
	.globl __ap_startup_rsp
SYM_DATA_BEGIN(__ap_startup_rsp)
	.quad 0
SYM_DATA_END(__ap_startup_rsp)

	.section ".percpu.fixed", "aw", @progbits

	.globl __pcpu_fixed
SYM_DATA_BEGIN(__pcpu_fixed)
	.fill 64, 1, 0
SYM_DATA_END(__pcpu_fixed)

	.section ".percpu", "aw", @progbits

	.balign 64
	.globl percpu_GDT
SYM_DATA_BEGIN(percpu_GDT)
	.quad 0				/** NULL entry  */
	.quad 0x00cffb000000ffff	/** __USER32_CS  */
	.quad 0x00cff3000000ffff	/** __USERXX_DS  */
	.quad 0x00affb000000ffff	/** __USER64_CS  */
	.quad 0x00af9b000000ffff	/** __KERNEL_CS  */
	.quad 0x00af93000000ffff	/** __KERNEL_DS  */
	.quad 0
	.quad 0
SYM_DATA_END(percpu_GDT)
