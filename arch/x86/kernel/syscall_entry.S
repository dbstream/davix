/**
 * Routines for entering from and returning to userspace.
 * Copyright (C) 2025-present  dbstream
 *
 * Notably asm_exec_jump_to_userspace is also part of this file.
 */
	.file "syscall_entry.S"
	.code64

#include <asm/gdt.h>
#include <asm/rflags.h>
#include <asm/sections.h>

#define PUSHQ(value)			\
	pushq value;			\
	.cfi_adjust_cfa_offset 8

#define POPQ(value)			\
	popq value;			\
	.cfi_adjust_cfa_offset -8

__TEXT

	.balign 64
	.globl asm_syscall_entry
	.type asm_syscall_entry, @function
asm_syscall_entry:
	.cfi_startproc
	.cfi_undefined rip

	swapgs
	movq %rsp, %gs:__syscall_saved_rsp
	movq %gs:(4+tss_struct), %rsp		/**  Load tss.rsp0   */

	/** construct 'struct entry_regs'  */

	PUSHQ ($__USER64_DS)		/*  ss		*/
	PUSHQ ($0)			/*  rsp		*/
	PUSHQ (%r11)			/*  rflags	*/
	PUSHQ ($__USER64_CS)		/*  cs		*/
	PUSHQ (%rcx)			/*  rip		*/
	PUSHQ (%rax)			/*  error_code	*/
	PUSHQ (%rax)
	PUSHQ (%rdi)
	PUSHQ (%rsi)
	PUSHQ (%rdx)
	PUSHQ (%rcx)
	PUSHQ (%r8)
	PUSHQ (%r9)
	PUSHQ (%r10)
	PUSHQ (%r11)
	PUSHQ (%rbp)
	PUSHQ (%rbx)
	PUSHQ (%r12)
	PUSHQ (%r13)
	PUSHQ (%r14)
	PUSHQ (%r15)
	movq %rsp, %rdi

	PUSHQ ($0)			/* function call alignment  */
	call x86_handle_syscall

	addq $8, %rsp
	.cfi_adjust_cfa_offset -8

	testb %al, %al

	POPQ (%r15)
	POPQ (%r14)
	POPQ (%r13)
	POPQ (%r12)
	POPQ (%rbx)
	POPQ (%rbp)
	POPQ (%r11)
	POPQ (%r10)
	POPQ (%r9)
	POPQ (%r8)
	POPQ (%rcx)
	POPQ (%rdx)
	POPQ (%rsi)
	POPQ (%rdi)
	POPQ (%rax)

	jnz syscall_return_via_iretq

	movq %gs:__syscall_saved_rsp, %rsp
	swapgs
	sysretq

syscall_return_via_iretq:
	addq $8, %rsp
	swapgs
	iretq

	.cfi_endproc
	.size asm_syscall_entry, . - asm_syscall_entry

	.balign 16
	.globl asm_exec_jump_to_userspace
	.type asm_exec_jump_to_userspace, @function
asm_exec_jump_to_userspace:
	.cfi_startproc

	/** ok, we will iretq to userspace
	 *
	 * needed stack layout:
	 * rsp+0x20	ss
	 * rsp+0x18	rsp
	 * rsp+0x10	rflags
	 * rsp+0x08	cs
	 * rsp+0x00	rip
	 */
	PUSHQ($__USER64_DS)
	PUSHQ(%r8)
	PUSHQ($__RFL_INITIAL)
	PUSHQ($__USER64_CS)
	PUSHQ(%rcx)

	/**
	 * Zero out registers to not leak any kernel values into userspace.
	 * FP state (8087, SSE etc..) is zero'd from C code.
	 *
	 * Do not zero %rdi, %rsi, or %rdx.
	 */
	xorq %rax, %rax
	xorq %rbx, %rbx
	xorq %rcx, %rcx
	xorq %rbp, %rbp
	xorq %r8, %r8
	xorq %r9, %r9
	xorq %r10, %r10
	xorq %r11, %r11
	xorq %r12, %r12
	xorq %r13, %r13
	xorq %r14, %r14
	xorq %r15, %r15

	/**
	 * Switch to userspace GSBASE and return to userspace.
	 * swapgs must be done with interrupts disabled.
	 */
	cli
	swapgs
	iretq

	.cfi_endproc
