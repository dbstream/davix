/**
 * Kernel entry points.
 * Copyright (C) 2025-present  dbstream
 */
	.file "entry.S"
	.code64

#include <asm/asm.h>
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

.macro PUSH_ENTRY_REGS saveall
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
.if \saveall == 1
	XPUSHQ(rbx)
	XPUSHQ(r12)
	XPUSHQ(r13)
	XPUSHQ(r14)
	XPUSHQ(r15)
	PUSHQ($0)
.endif
	leaq (-40 + 48*\saveall)(%rsp), %rdi
.endm

.macro POP_ENTRY_REGS saveall
.if \saveall == 1
	addq $8, %rsp
	.cfi_adjust_cfa_offset -8
	XPOPQ(r15)
	XPOPQ(r14)
	XPOPQ(r13)
	XPOPQ(r12)
	XPOPQ(rbx)
.endif
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

.macro idtentry_exception name has_err saveall user_func kernel_func
	.globl \name
	.balign 16
SYM_FUNC_BEGIN(\name)
	CFI_IRET_FRAME 8*\has_err

.if \has_err == 0
	PUSHQ($0)
.endif

	PUSH_ENTRY_REGS \saveall
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
	POP_ENTRY_REGS \saveall

	addq $8, %rsp
	.cfi_adjust_cfa_offset -8

	iretq
SYM_FUNC_END(\name)
.endm

	__TEXT

idtentry_exception asm_handle_GP 1 0 handle_GP_exception handle_GP_exception_k
idtentry_exception asm_handle_PF 1 0 handle_PF_exception handle_PF_exception_k
