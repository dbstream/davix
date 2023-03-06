/* SPDX-License-Identifier: MIT */
/*
 * Non-early entry points into the kernel.
 * These are:
 *   - Syscalls
 *   - Exceptions
 *   - Interrupts
 *   - Inter-processor interrupts
 */

#include <asm/msr.h>
#include <asm/segment.h>

.altmacro /* enable more advanced macro features */

.code64
.section .text

.macro repeat what from to
\what \from
.if \to-\from
repeat \what %(\from+1) \to
.endif
.endm

.macro isr nr cfunc
.align 16
isr_\nr:
	/*
	 * Align %rsp to 16 bytes to skip the error code field in irq_frame if
	 * none was pushed by the CPU.
	 */
	andq $0xfffffffffffffff0, %rsp

	/*
	 * Push the ISR number onto the stack.
	 */
	pushq $\nr

	/*
	 * Later on, we do a 'call *%rax' to jump to cfunc.
	 */
	pushq %rax
	movq $\cfunc, %rax

	jmp isr_common
.endm

.align 64
isr_common:
	cld
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

	cmpw $GDT_KERNEL_CS, 160(%rsp)
	jne 1f /* userspace - always swapgs */

	movq %rax, %r8 /* stash cfunc in r8 as it gets overwritten by rdmsr */

	movl $MSR_GSBASE, %ecx
	rdmsr /* now, eax contains low 32 bits, edx contains high 32 bits */

	testl $(1 << 31), %edx
	js 2f /* swapgs already done */

	movq %r8, %rax /* restore stashed cfunc */
1:
	swapgs
	call *%rax
	swapgs
	jmp 1f
2:
	call *%r8
1:

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

.macro unhandled nr
isr \nr x86_ex_unhandled
.endm

.macro hwirq nr
isr \nr x86_irq
.endm

repeat unhandled 0 1
isr 2 x86_nmi
repeat unhandled 3 7
isr 8 x86_doublefault
repeat unhandled 9 12
isr 13 x86_gp_fault
isr 14 x86_page_fault
repeat unhandled 15 17
isr 18 x86_mce
repeat unhandled 19 31

repeat hwirq 32 63
repeat hwirq 64 91
repeat hwirq 92 127
repeat hwirq 128 159
repeat hwirq 160 191
repeat hwirq 192 223
repeat hwirq 224 252

isr 253 x86_smp_notified
isr 254 x86_timer
isr 255 x86_spurious

.macro ivt_entry nr
.quad isr_\nr
.endm

.section .rodata
.globl x86_isr_stubs
.type x86_isr_stubs, @object
.align 16
x86_isr_stubs:
repeat ivt_entry 0 31
repeat ivt_entry 32 63
repeat ivt_entry 64 91
repeat ivt_entry 92 127
repeat ivt_entry 128 159
repeat ivt_entry 160 191
repeat ivt_entry 192 223
repeat ivt_entry 224 255
