/* SPDX-License-Identifier: MIT */

.section .text
.globl __switch_to
.type __switch_to, @function
__switch_to:
	/*
	 * Save registers.
	 */
	pushq %rbp
	pushq %rbx
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15
	pushq %rsi
	pushq %rdi

	/*
	 * Switch stacks.
	 */
	movq %rsp, (%rdi)
	movq (%rsi), %rsp

	/*
	 * Call after_switch_from().
	 */
	call x86_after_switch_from

	/*
	 * Restore registers.
	 */
	popq %rdi
	popq %rsi
	popq %r15
	popq %r14
	popq %r13
	popq %r12
	popq %rbx
	popq %rbp
	ret

.size __switch_to, . - __switch_to
