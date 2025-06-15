/**
 * Kernel entry points.
 * Copyright (C) 2025-present  dbstream
 */
	.file "entry.S"
	.altmacro
	.code64

#include <asm/asm.h>
#include <asm/entry.h>
#include <asm/gdt.h>
#include <asm/interrupt.h>

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
	.balign 16
SYM_FUNC_BEGIN(\name)
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
SYM_FUNC_END(\name)
.endm

.macro idtentry_vector nr
SYM_FUNC_BEGIN(asm_irq_\nr)
	CFI_IRET_FRAME 0
	PUSHQ($\nr)
	jmp asm_handle_irq_common
SYM_FUNC_END(asm_irq_\nr)
.endm

.macro idtentry_vector_array_entry nr
	.quad asm_irq_\nr
.endm

	__TEXT

	.balign 64
SYM_FUNC_BEGIN(asm_handle_irq_common)
	CFI_IRET_FRAME 8

	PUSH_ENTRY_REGS
	movq IRET_CS(%rdi), %rax
	cmpq $__KERNEL_CS, %rax
	je .Lirq_entry_from_kernelspace

	swapgs
	call __entry_from_irq_vector
	swapgs
	jmp .Lirq_entry_common

.Lirq_entry_from_kernelspace:
	call __entry_from_irq_vector_k

.Lirq_entry_common:
	POP_ENTRY_REGS

	addq $8, %rsp
	.cfi_adjust_cfa_offset -8

	iretq
SYM_FUNC_END(asm_handle_irq_common)

idtentry_exception asm_handle_GP 1 handle_GP_exception handle_GP_exception_k
idtentry_exception asm_handle_PF 1 handle_PF_exception handle_PF_exception_k

	.set i, IRQ_VECTOR_OFFSET
.rept IRQ_VECTOR_NUM
	idtentry_vector %i
	.set i, i + 1
.endr

	/**
	 * asm_switch_to - task switching assembly
	 */
	.globl asm_switch_to
SYM_FUNC_BEGIN(asm_switch_to)
	/* push registers...  */
	XPUSHQ(r15)
	XPUSHQ(r14)
	XPUSHQ(r13)
	XPUSHQ(r12)
	XPUSHQ(rbx)
	XPUSHQ(rbp)

	/* Store old stack pointer in old task...  */
	movq %rsp, (%rdi)

	/* Load new stack pointer from new task...  */
	movq (%rsi), %rsp

	/* Store the old task in %rax...  */
	movq %rdi, %rax

	/* pop registers...  */
	XPOPQ(rbp)
	XPOPQ(rbx)
	XPOPQ(r12)
	XPOPQ(r13)
	XPOPQ(r14)
	XPOPQ(r15)

	/* Return with the old task in %rax  */
	ret
SYM_FUNC_END(asm_switch_to)

	/**
	 * asm_ret_from_new_task - return to userspace from a new task
	 *
	 * asm_switch_to returns here when a new task begins execution.
	 * Initially, the stack pointer is 16-byte aligned and points to just
	 * below a struct entry_regs (such that POP_ENTRY_REGS does the correct
	 * thing).
	 */
	.globl asm_ret_from_new_task
SYM_FUNC_BEGIN(asm_ret_from_new_task)
	.cfi_undefined rip
	/* asm_switch_to stores the previous task in %rax  */
	movq %rax, %rsi

	/* arch_create_task stashes the function and argument in %r12 and %r13  */
	movq %r12, %rdx
	movq %r13, %rdi

	/* Setup get_user_entry_regs */
	movq %rsp, %gs:16

	/* Call arch_ret_from_new_task on an aligned stack pointer.  */
	subq $8, %rsp
	call arch_ret_from_new_task
	addq $8, %rsp

	/* Return to userspace by loading register state from the iret frame.  */
	POP_ENTRY_REGS

	/* Now, return to userspace.  */
	addq $8, %rsp
	iretq

SYM_FUNC_END(asm_ret_from_new_task)

	__RODATA

	.balign 8
	.globl asm_idtentry_vector_array
SYM_DATA_BEGIN(asm_idtentry_vector_array)
	.set i, IRQ_VECTOR_OFFSET
.rept IRQ_VECTOR_NUM
	idtentry_vector_array_entry %i
	.set i, i + 1
.endr
SYM_DATA_END(asm_idtentry_vector_array)
