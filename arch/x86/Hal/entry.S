// SPDX-License-Identifier: GPL-3.0
/*
 * File: Hal/entry.S
 * Low-level entry points into the kernel.
 *
 * Copyright (C) 2025  dbstream
 */
	.file "entry.S"
	.altmacro
	.code64

#include <Hal/irq_vectors.h>
#include <asm/entry.h>
#include <asm/gdt.h>

#define PUSHQ(value)			\
	pushq value;			\
	.cfi_adjust_cfa_offset 8

#define POPQ(reg)			\
	popq reg;			\
	.cfi_adjust_cfa_offset -8

#define XPUSHQ(reg)			\
	PUSHQ(%reg);			\
	.cfi_rel_offset reg, 0

#define XPOPQ(reg)			\
	POPQ(%reg);			\
	.cfi_restore reg

.macro CFI_IRET_FRAME offset
	.cfi_signal_frame
	.cfi_def_cfa rsp, 40 + \offset
	.cfi_offset rsp, -16
	.cfi_offset rip, -40
.endm

.macro PUSH_ENTRY_REGS
	XPUSHQ(rax)
	XPUSHQ(rdi)
	XPUSHQ(rsi)
	XPUSHQ(rdx)
	XPUSHQ(rcx)
	XPUSHQ(r8)
	XPUSHQ(r9)
	XPUSHQ(r10)
	XPUSHQ(r11)
	XPUSHQ(rbp)
	XPUSHQ(rbx)
	XPUSHQ(r12)
	XPUSHQ(r13)
	XPUSHQ(r14)
	XPUSHQ(r15)
	PUSHQ($0)
	leaq 8(%rsp), %rdi
.endm

.macro POP_ENTRY_REGS
	addq $8, %rsp
	.cfi_adjust_cfa_offset -8
	XPOPQ(r15)
	XPOPQ(r14)
	XPOPQ(r13)
	XPOPQ(r12)
	XPOPQ(rbx)
	XPOPQ(rbp)
	XPOPQ(r11)
	XPOPQ(r10)
	XPOPQ(r9)
	XPOPQ(r8)
	XPOPQ(rcx)
	XPOPQ(rdx)
	XPOPQ(rsi)
	XPOPQ(rdi)
	XPOPQ(rax)
.endm

.macro idtentry_exception name has_err user_func kernel_func
	.globl \name
	.type \name, @function
	.balign 16
\name:
	.cfi_startproc
	CFI_IRET_FRAME 8*\has_err

.if \has_err == 0
	PUSHQ($0)
.endif

	PUSH_ENTRY_REGS
	movq IRET_CS(%rdi), %rax
	cmpq $__KERNEL_CS, %rax
	je 1f

	swapgs
	call \user_func
	swapgs
	jmp 2f
1:
	call \kernel_func
2:
	POP_ENTRY_REGS

	addq $8, %rsp
	.cfi_adjust_cfa_offset -8
	iretq

	.cfi_endproc
	.size \name, . - \name
.endm

.macro idtentry_vector nr
	.type asm_irq_\nr, @function
asm_irq_\nr:
	.cfi_startproc
	CFI_IRET_FRAME 0
	PUSHQ($\nr)
	jmp asm_irq_common
	.cfi_endproc
	.size asm_irq_\nr, . - asm_irq_\nr
.endm

	.text

idtentry_exception asm_handle_GP 1 handle_GP_exception handle_GP_exception_k
idtentry_exception asm_handle_PF 1 handle_PF_exception handle_PF_exception_k

	.type asm_handle_int80h, @function
asm_handle_int80h:
	.cfi_startproc
	CFI_IRET_FRAME 0
	swapgs
	PUSHQ($0)
	PUSH_ENTRY_REGS
	call HalHandleInt80h
	POP_ENTRY_REGS
	addq $8, %rsp
	.cfi_adjust_cfa_offset -8
	swapgs
	iretq
	.cfi_endproc
	.size asm_handle_int80h, . - asm_handle_int80h

.macro emit_idtentry_vector_unless_int80h i
	.if \i != IRQ_VECTOR_INT80h
		idtentry_vector \i
	.endif
.endm

.set i, IRQ_VECTOR_DYNAMIC_FIRST
.rept 0x100 - IRQ_VECTOR_DYNAMIC_FIRST
	emit_idtentry_vector_unless_int80h %i
	.set i, i + 1
.endr

	.balign 64
	.type asm_irq_common, @function
asm_irq_common:
	.cfi_startproc
	CFI_IRET_FRAME 8
	PUSH_ENTRY_REGS
	movq IRET_CS(%rdi), %rax
	cmpq $__KERNEL_CS, %rax
	je .Lirq_entry_from_kernel

	swapgs
	call HalHandleIRQVectorFromUserspace
	swapgs
	jmp .Lirq_exit_common

.Lirq_entry_from_kernel:
	call HalHandleIRQVectorFromKernel

.Lirq_exit_common:
	POP_ENTRY_REGS
	addq $8, %rsp
	.cfi_adjust_cfa_offset -8
	iretq
	.cfi_endproc
	.size asm_irq_common, . - asm_irq_common

	/**
	 * asm_switch_thread - switch to another KTHREAD.
	 * @%rdi: previous KTHREAD
	 * @%rsi: next KTHREAD
	 * Returns the previous KTHREAD in %rax.
	 */
	.globl asm_switch_thread
	.type asm_switch_thread, @function
	.balign 64
asm_switch_thread:
	.cfi_startproc
	/* Push registers. */
	XPUSHQ(r15)
	XPUSHQ(r14)
	XPUSHQ(r13)
	XPUSHQ(r12)
	XPUSHQ(rbx)
	XPUSHQ(rbp)

	/* Store the old stack pointer in the previous KTHREAD. */
	movq %rsp, (%rdi)

	/* Load the new stack pointer from the next KTHREAD. */
	movq (%rsi), %rsp

	/* Store the old task in %rax. */
	movq %rdi, %rax

	/* Pop registers. */
	XPOPQ(rbp)
	XPOPQ(rbx)
	XPOPQ(r12)
	XPOPQ(r13)
	XPOPQ(r14)
	XPOPQ(r15)

	/* Return with the old task in %rax. */
	ret
	.cfi_endproc
	.size asm_switch_thread, . - asm_switch_thread

	/**
	 * asm_ret_from_new_thread - return from a new KTHREAD.
	 * @%r12: 'entrypoint' argument to HalInitializeThread
	 * @%r13: 'arg' argument to HalInitializeThread
	 * @%rax: previous KTHREAD
	 */
	.globl asm_ret_from_new_thread
	.type asm_ret_from_new_thread, @function
	.balign 8
asm_ret_from_new_thread:
	.cfi_startproc
	.cfi_undefined rip
	movq %rax, %rdi
	movq %r12, %rsi
	movq %r13, %rdx

	call HalReturnFromNewThread

	POP_ENTRY_REGS

	addq $8, %rsp
	iretq
	.cfi_endproc
	.size asm_ret_from_new_thread, . - asm_ret_from_new_thread

	.section ".rodata", "a", @progbits

.macro emit_idtentry_vector_array_entry i
	.if  \i == IRQ_VECTOR_INT80h
		.quad asm_handle_int80h
	.else
		.quad asm_irq_\i
	.endif
.endm

	.balign 8
	.globl asm_idtentry_vector_array
	.type asm_idtentry_vector_array, @object
asm_idtentry_vector_array:
.set i, IRQ_VECTOR_DYNAMIC_FIRST
.rept 0x100 - IRQ_VECTOR_DYNAMIC_FIRST
	emit_idtentry_vector_array_entry %i
	.set i, i + 1
.endr
	.size asm_idtentry_vector_array, . - asm_idtentry_vector_array

