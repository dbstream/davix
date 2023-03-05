/* SPDX-License-Identifier: MIT */
#ifndef __ASM_ENTRY_H
#define __ASM_ENTRY_H

/*
 * Data structures used by the kernel for register state on kernel entry.
 */

#include <davix/types.h>

struct irq_frame {
	/* bottom of stack */

	/*
	 * SysV ABI says that these are callee-preserved registers. We might
	 * need them in some point in the future.
	 */
	u64 r15;
	u64 r14;
	u64 r13;
	u64 r12;
	u64 rbx;
	u64 rbp;

	/*
	 * SysV ABI says that these are scratch registers, so we must push them
	 * on the stack.
	 */
	u64 r11;
	u64 r10;
	u64 r9;
	u64 r8;
	u64 rcx;
	u64 rdx;
	u64 rsi;
	u64 rdi;
	u64 rax;

	/*
	 * Exception number.
	 */
	u64 excep_nr;

	/*
	 * Error code, pushed by some exceptions. For other producers of
	 * irq_regs, this will probably be zero.
	 */
	u32 alignas(u64) error;

	/*
	 * %cs:%rip when the interrupt happened.
	 */
	u64 rip;
	u16 alignas(u64) cs;

	/*
	 *  %rflags when the interrupt happened.
	 */
	u64 rflags;

	/*
	 * %ss:%rsp is always pushed by interrupts in long mode.
	 */
	u64 rsp;
	u16 alignas(u64) ss;

	/* top of stack */
};

void x86_setup_early_idt(void);

void x86_load_idt_ap(void);

void x86_idle_task(void);

/*
 * Vector numbers for architectural exceptions.
 */
#define X86_TRAP_DIV_BY_ZERO 0
#define X86_TRAP_DEBUG 1
#define X86_TRAP_NMI 2
#define X86_TRAP_BREAKPOINT 3
#define X86_TRAP_OVERFLOW 4
#define X86_TRAP_BOUND_RANGE 5
#define X86_TRAP_UD 6
#define X86_TRAP_DEVICE_NOT_AVAIL 7
#define X86_TRAP_DOUBLE_FAULT 8
#define X86_TRAP_BAD_TSS 10
#define X86_TRAP_BAD_SEGMENT 11
#define X86_TRAP_BAD_STACK 12
#define X86_TRAP_GP 13
#define X86_TRAP_PF 14
#define X86_TRAP_FP_EXCEPTION 16
#define X86_TRAP_ALIGNMENT_CHECK 17
#define X86_TRAP_MCE 18
#define X86_TRAP_SIMD_FP 19
#define X86_TRAP_CONTROL_PROT 21

#endif /* __ASM_ENTRY_H */
