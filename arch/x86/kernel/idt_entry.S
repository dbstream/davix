/**
 * Kernel entry via interrupts and exceptions.
 * Copyright (C) 2024  dbstream
 */
	.file "idt_entry.S"
	.code64
	.altmacro

#include <asm/entry.h>
#include <asm/gdt.h>
#include <asm/interrupt.h>
#include <asm/msr.h>
#include <asm/sections.h>
#include <asm/trap.h>

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

	/**
	 * Although %rbp isn't a scratch register, save it anyways. 
	 * (atomic_entry/exit depends on it).
	 */
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
	.type \name, @function
	.balign 16
\name:
	.cfi_startproc
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

	/* error_code is not part of the iret frame */
	addq $8, %rsp
	.cfi_adjust_cfa_offset -8
	iretq

	.cfi_endproc
	.size \name, . - \name
.endm

.macro idtentry_explicit name cfunc
	idtentry_exception \name 0 0 \cfunc \cfunc
.endm

.macro idtentry_irq nr
	.type asm_irq_\nr, @function
	.balign 16
asm_irq_\nr:
	.cfi_startproc
	CFI_IRET_FRAME 0

	PUSHQ($\nr)

	jmp asm_irq_common

	.cfi_endproc
	.size asm_irq_\nr, . - asm_irq_\nr
.endm

.macro idtentry_irq_array_entry nr
	.quad asm_irq_\nr
.endm

.macro idtentry_atomic name has_err skip_userspace_check cfunc
	.globl \name
	.type \name, @function
	.balign 16
\name:
	.cfi_startproc
	CFI_IRET_FRAME 8*\has_err

.if \has_err == 0
	PUSHQ($0)
.endif

	PUSH_ENTRY_REGS 0

.if \skip_userspace_check == 0
	movq IRET_CS(%rdi), %rax
	cmpq $__KERNEL_CS, %rax
	je 1f

	/**
	 * We came from userspace, hence we are on user GSBASE.
	 * --> swapgs unconditionally
	 */
	swapgs
	call \cfunc
	swapgs
	jmp 2f
1:
.endif
	call atomic_entry
	call \cfunc
	call atomic_exit

.if \skip_userspace_check == 0
2:
.endif
	POP_ENTRY_REGS 0

	/* error_code is not part of the iret frame */
	addq $8, %rsp
	.cfi_adjust_cfa_offset -8
	iretq

	.cfi_endproc
	.size \name, . - \name
.endm

	__TEXT

	/**
	 * Common routine for IRQ entry and exit.
	 *
	 * IRQ entries originate from an external interrupt source such as the
	 * local APIC or PCI devices via MSI/MSI-X. The CPU jumps to a handler
	 * pointed to by the IDT, which jumps to us.
	 */
	.type asm_irq_common, @function
	.balign 16
asm_irq_common:
	.cfi_startproc
	/* The IDT stub pushed an error code for us. */
	CFI_IRET_FRAME 8

	PUSH_ENTRY_REGS 0

	movq IRET_CS(%rdi), %rax
	cmpq $__KERNEL_CS, %rax
	je 1f

	swapgs
	call handle_IRQ
	swapgs
	jmp 2f
1:
	call handle_IRQ
2:
	POP_ENTRY_REGS 0

	/* error_code is not part of the iret frame */
	addq $8, %rsp
	.cfi_adjust_cfa_offset -8
	iretq

	.cfi_endproc
	.size asm_irq_common, . - asm_irq_common

	/**
	 * Helper functions for atomic entry and exit.
	 *
	 * An atomic entry is a kernel entry that doesn't adhere to the normal
	 * rules. These can occur in IRQ-disabled regions (NMIs, MCEs), and may
	 * occur in kernelspace while the userspace GSBASE is still active. As
	 * such, they must be treated very carefully to ensure correct behavior.
	 */

	/**
	 * Rules for atomic_entry and atomic_exit:
	 * - atomic_entry and atomic_exit must be called after registers are
	 *   pushed and before registers are popped.
	 * - The value of %rbp must be preserved for corresponding atomic_entry
	 *   and atomic_exit pairs.
	 */

	/**
	 * Look at (rsp & ~(TRAP_STK_SIZE - 1)) to determine whether we should
	 * swap GSBASE or not. See comment in idt.c for further details.
	 */
	.type atomic_entry, @function
	.balign 16
atomic_entry:
	.cfi_startproc

	/* read GSBASE into %rax */
	movl $MSR_GSBASE, %ecx
	rdmsr
	shlq $32, %rdx
	orq %rdx, %rax

	/* compare with kernel GSBASE */
	movq %rsp, %rsi
	andq $~(TRAP_STK_SIZE - 1), %rsi
	cmpq %rax, (%rsi)
	je 1f

	/* we are on user GSBASE, swapgs here and in atomic_exit */
	swapgs
	movl $1, %ebp
	ret

1:	/* we are on kernel GSBASE, don't swapgs */
	xorq %rbp, %rbp
	ret
	.cfi_endproc
	.size atomic_entry, . - atomic_entry

	/**
	 * Look at %rbp to determine if GSBASE was swapped by atomic_entry, and
	 * swap back if necessary.
	 */
	.type atomic_exit, @function
	.balign 16
atomic_exit:
	.cfi_startproc

	testq %rbp, %rbp
	jnz 1f
	ret
1:
	swapgs
	ret

	.cfi_endproc
	.size atomic_exit, . - atomic_exit

idtentry_exception asm_handle_BP 0 1 handle_BP_interrupt handle_BP_interrupt_k

idtentry_atomic asm_handle_NMI 0 0 handle_NMI
idtentry_atomic asm_handle_MCE 0 0 handle_MCE

idtentry_explicit asm_handle_APIC_timer handle_APIC_timer
idtentry_explicit asm_handle_IRQ_spurious handle_IRQ_spurious

idtentry_explicit asm_handle_PIC_IRQ handle_PIC_IRQ

.set i, 0
.rept IRQ_VECTORS_PER_CPU
	idtentry_irq %i
	.set i, i + 1
.endr

	__RODATA

	.globl asm_idtentry_irq_array
	.type asm_idtentry_irq_array, @object
	.balign 16
asm_idtentry_irq_array:
	.set i, 0
.rept IRQ_VECTORS_PER_CPU
	idtentry_irq_array_entry %i
	.set i, i + 1
.endr
	.size asm_idtentry_irq_array, . - asm_idtentry_irq_array
