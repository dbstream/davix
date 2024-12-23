/**
 * Kernel startup in 64-bit mode.
 * Copyright (C) 2024  dbstream
 */
	.file "startup64.S"
	.code64

#include <asm/gdt.h>
#include <asm/msr.h>
#include <asm/sections.h>

	__TEXT

	.globl _start
	.type _start, @function
_start:
	.cfi_startproc
	.cfi_undefined rip

	cli
	cld

	movq __startup_rsp(%rip), %rsp
	movq __startup_rip(%rip), %rax
	call *%rax

	.cfi_endproc
	.size _start, . - _start

	__DATA

	/**
	 * The "startup block", which points to the kernel entry point.
	 * Overwritten for APs on SMP kernels.
	 */
	.globl __startup_rsp
	.globl __startup_rip
	.balign 16
__startup_rsp:	.quad initial_stack_top
__startup_rip:	.quad x86_start_kernel

	__DATA_PAGE_ALIGNED

	.balign 0x1000
initial_stack:
	.fill 0x4000, 1, 0
initial_stack_top:

	__CPULOCAL_FIXED

	/**
	 * This is the fixed CPU-local data. It needs to be at constant address
	 * zero, because GCC hardcodes the stack canary as %gs:40, and we also
	 * hardcode the CPU-local offset itself as %gs:0.
	 */
	.globl __cpulocal_offset
	.type __cpulocal_offset, @object
	.size __cpulocal_offset, 8
__cpulocal_offset:	.quad 0

	.globl __this_cpu_id
	.type __this_cpu_id, @object
	.size __this_cpu_id, 4
__this_cpu_id:		.long 0

	.globl __preempt_state
	.type __preempt_state, @object
	.size __preempt_state, 4
__preempt_state:
			.long 0

	.globl __current_task
	.type __current_task, @object
	.size __current_task, 8
__current_task:
			.quad 0

			.fill 16, 1, 0

	.globl __stack_canary
	.type __stack_canary, @object
	.size __stack_canary, 8
__stack_canary:		.quad 0

			.fill 16, 1, 0	/* Pad to 64 bytes. */

	__CPULOCAL

	/**
	 * This is the per-CPU GDT. It gets copied to the other CPU-local
	 * sections and the TSS gets filled in by C code.
	 */
	.globl percpu_GDT
	.type percpu_GDT, @object
	.balign 64
percpu_GDT:
	.quad 0
	.quad 0x00cffb000000ffff	/* __USER32_CS */
	.quad 0x00cff3000000ffff	/* __USERXX_DS */
	.quad 0x00affb000000ffff	/* __USER64_CS */
	.quad 0x00af93000000ffff	/* __KERNEL_DS */
	.quad 0x00af9b000000ffff	/* __KERNEL_CS */
	.quad 0				/* TSS: low qword */
	.quad 0				/* TSS: high qword */

	__INIT_TEXT

	.globl __startup64_bsp
	.type __startup64_bsp, @function
	.balign 16
__startup64_bsp:
	.cfi_startproc
	.cfi_undefined rip

	/* Setup CPU-local variables access for BSP. */
	movl $MSR_GSBASE, %ecx
	leaq __bsp_cpulocal_offset(%rip), %rax
	movq %rax, %rdx
	shrq $32, %rdx
	wrmsr
	movq %rax, %gs:0	/* cpulocal_offset */

	jmp lgdt_and_goto__start

	.cfi_endproc
	.size __startup64_bsp, . - __startup64_bsp

	__TEXT

	.globl __startup64_ap
	.type __startup64_ap, @function
	.balign 16
__startup64_ap:
	.cfi_startproc
	.cfi_undefined rip

	/** smpboot.c sets up CPU-local variables for this AP.  */
	call startup64_ap_initialize_cpu

lgdt_and_goto__start:
	leaq percpu_GDT(%rip), %rbx
	addq %gs:0, %rbx
	subq $16, %rsp
	movw $0x3f, (%rsp)
	movq %rbx, 2(%rsp)
	lgdt (%rsp)
	addq $16, %rsp

	swapgs		/* Changing %gs clobbers GSBASE, so save it. */
	movl $__KERNEL_DS, %eax
	movl %eax, %ds
	movl %eax, %es
	movl %eax, %fs
	movl %eax, %gs
	movl %eax, %ss
	swapgs		/* Restore saved GSBASE. */

	pushq $__KERNEL_CS
	pushq $_start
	lretq

	.cfi_endproc
	.size __startup64_ap, . - __startup64_ap
