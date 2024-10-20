/**
 * Task switching on x86.
 * See include/asm/task.h for details.
 */
	.file "task_switch.S"
	.code64

#include <asm/sections.h>

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

	__TEXT

	.globl arch_switch_to
	.type arch_switch_to, @function
arch_switch_to:
	.cfi_startproc

	XPUSHQ(rdi)
	XPUSHQ(r15)
	XPUSHQ(r14)
	XPUSHQ(r13)
	XPUSHQ(r12)
	XPUSHQ(rbx)
	XPUSHQ(rbp)

	movq %rsp, (%rsi)
	movq (%rdi), %rsp

	XPOPQ(rbp)
	XPOPQ(rbx)
	XPOPQ(r12)
	XPOPQ(r13)
	XPOPQ(r14)
	XPOPQ(r15)
	XPOPQ(rdi)
	ret

	.cfi_endproc
	.size arch_switch_to, . - arch_switch_to
