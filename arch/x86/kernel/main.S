/* SPDX-License-Identifier: MIT */
.code64

#include <asm/segment.h>

.section .text
.globl _start
.type _start, @function
_start:
	/*
	 * We are currently in long mode with a GDT that has undefined state.
	 * Setup the stack, load a new GDT and jump to the real kernel entry
	 * point.
	 */

	xorq %rbp, %rbp
	movq $(early_kernel_stack + 0x1000), %rsp

	subq $16, %rsp	/* allocate space on the stack */

	/*
	 * Load the new GDT.
	 */
	movw $(GDT_SIZE - 1), (%rsp)
	movq $early_kernel_gdt, 2(%rsp)
	lgdt (%rsp)

	/*
	 * Setup a JMP FAR frame.
	 */
	movq $1f, (%rsp)
	movw $GDT_KERNEL_CS, 8(%rsp)

	/*
	 * Do a far jump to load %cs.
	 * Buggy GAS omits the rex64 prefix for ljmp, so include it manually.
	 */
	rex64 ljmp *(%rsp)
1:
	/*
	 * Load the data segments and the stack segment.
	 */
	movl $GDT_KERNEL_DS, %eax
	movl %eax, %ds
	movl %eax, %es
	movl %eax, %fs
	movl %eax, %gs
	movl %eax, %ss

	addq $16, %rsp

	/*
	 * ... and finally, enter the main kernel.
	 */
	pushq $0
	jmp x86_start_kernel

.section .data
.balign 16

/*
 * This should have the same layout as the "normal" kernel GDT.
 * It simplifies things.
 */
early_kernel_gdt:
	/* NULL segment */
	.quad 0

	/* KERNEL_CS */
	.long 0xffff
	.byte 0
	.byte 0b10011010
	.byte 0b10101111
	.byte 0

	/* KERNEL_DS */
	.long 0xffff
	.byte 0
	.byte 0b10010010
	.byte 0b11001111
	.byte 0

	/* USER32_CS */
	.long 0xffff
	.byte 0
	.byte 0b11111010
	.byte 0b11001111
	.byte 0

	/* USER_DS */
	.long 0xffff
	.byte 0
	.byte 0b11110010
	.byte 0b11001111
	.byte 0

	/* USER_CS */
	.long 0xffff
	.byte 0
	.byte 0b11111010
	.byte 0b10101111
	.byte 0

	/* TSS - not used by early kernel */
	.quad 0
	.quad 0

early_kernel_stack:
	.fill 0x1000, 1, 0
