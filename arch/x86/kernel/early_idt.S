/* SPDX-License-Identifier: MIT */
.altmacro

.code64
.section .text

.macro repeat what from to
\what \from
.if \to-\from
repeat \what %(\from+1) \to
.endif
.endm

.macro isr excep_nr cfunc
isr_\excep_nr:

	/*
	 * If the CPU pushed an error code, %rsp will be
	 * 16-byte aligned, so this does nothing. If the
	 * CPU didn't push an error code, we skip past the
	 * error code field to keep the ``struct irq_frame``
	 * consistent.
	 */
	andq $0xfffffffffffffff0, %rsp

	pushq $\excep_nr

	pushq %rax
	movq $\cfunc, %rax /* later, we do a ``jmp *%rax`` to the C function */

	jmp isr_stub_common

.endm

isr_stub_common:

	/*
	 * Save register state to the irq_frame.
	 */
	pushq %rdi
	pushq %rsi
	pushq %rdx
	pushq %rcx
	pushq %r8
	pushq %r9
	pushq %r10
	pushq %r11
	pushq %rbp
	pushq %rbx
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15

	movq %rsp, %rdi /* Store the location of ``struct irq_frame`` in %rdi */

	/*
	 * Setup a stack frame.
	 */
	pushq $0
	pushq $0
	movq %rsp, %rbp

	/*
	 * Call ``void (*rax)(struct irq_frame *);``
	 */
	call *%rax

	/*
	 * Skip past the stack frame.
	 */
	addq $16, %rsp

	/*
	 * Restore register state.
	 */
	popq %r15
	popq %r14
	popq %r13
	popq %r12
	popq %rbx
	popq %rbp
	popq %r11
	popq %r10
	popq %r9
	popq %r8
	popq %rcx
	popq %rdx
	popq %rsi
	popq %rdi
	popq %rax

	/*
	 * Skip the excep_nr and error code and return.
	 */
	addq $16, %rsp
	iretq

isr 0 __handle_early_exception	/* #DE - Divide-By-Zero-Error */
isr 1 __handle_early_exception	/* #DB - Debug Exception */
isr 2 __handle_early_nmi	/* #NMI - Non-Maskable-Interrupt */
isr 3 __handle_early_exception	/* #BP - Breakpoint Exception */
isr 4 __handle_early_exception	/* #OF - Overflow Exception */
isr 5 __handle_early_exception	/* #BR - Bound-Range Exception */
isr 6 __handle_early_exception	/* #UD - Invalid Opcode */
isr 7 __handle_early_exception	/* #NM - Device-Not-Available */
isr 8 __handle_early_exception	/* #DF - Double-Fault */
isr 9 __handle_early_exception	/* Formerly Coprocessor-Segment-Overrun,
				 * but reserved in modern CPUs. */
isr 10 __handle_early_exception	/* #TS - Invalid-TSS Exception */
isr 11 __handle_early_exception	/* #NP - Segment-Not-Present */
isr 12 __handle_early_exception	/* #SS - Stack Exception */
isr 13 __handle_early_exception	/* #GP - General-Protection */
isr 14 __handle_early_pagefault	/* #PF - Page-Fault Exception */
isr 15 __handle_early_exception	/* Reserved vector */
isr 16 __handle_early_exception	/* #MF - Floating-Point Exception */
isr 17 __handle_early_exception	/* #AC - Alignment0 Check Excpetion */
isr 18 __handle_early_nmi		/* #MC - Machine-Check Exception */
isr 19 __handle_early_exception	/* #XF - SIMD Exception */
isr 20 __handle_early_exception	/* Reserved vector */
isr 21 __handle_early_exception	/* #CP - Control-Protection Exception */
isr 22 __handle_early_exception	/* Reserved vector */
isr 23 __handle_early_exception	/* Reserved vector */
isr 24 __handle_early_exception	/* Reserved vector */
isr 25 __handle_early_exception	/* Reserved vector */
isr 26 __handle_early_exception	/* Reserved vector */
isr 27 __handle_early_exception	/* Reserved vector */
isr 28 __handle_early_exception	/* #HV - Hypervisor Injection */
isr 29 __handle_early_exception	/* #VC - VMM Communication Exception */
isr 30 __handle_early_exception	/* #SX - Security Exception */
isr 31 __handle_early_exception	/* Reserved vector */

.section .rodata

.macro ivt_entry excep_nr
.quad isr_\excep_nr
.endm

.global early_isr_stubs
.type early_isr_stubs, @object

.balign 16
early_isr_stubs:
	repeat ivt_entry 0 31
.size early_isr_stubs, . - early_isr_stubs
