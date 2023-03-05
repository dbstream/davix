/* SPDX-License-Identifier: MIT */
/*
 * x86 smpboot trampoline for when application processors (APs) start up.
 * The processor jumps here in 16-bit mode. Stabilise the environment and
 * somehow reach 64-bit mode, before jumping to the actual kernel.
 */
#include <asm/msr.h>
#include <asm/segment.h>

.section .rodata
.globl x86_smp_trampoline
.type x86_smp_trampoline, @object
.balign 4096
x86_smp_trampoline:
.code16
	/*
	 * Do a 'movl $imm32,%eax'.
	 * The '$imm32' is little-endian at offset 2 from x86_smp_trampoline.
	 */
	.byte 0x66, 0xb8, 0x00, 0x00, 0x00, 0x00

	/*
	 * Now, eax contains the address of x86_smp_trampoline.
	 */
	cli
	wbinvd
	addl %eax, (reset_cs_frame - x86_smp_trampoline)(%eax)
	ljmpl *(reset_cs_frame - x86_smp_trampoline)(%eax)
reset_cs_frame:
	.long reset_cs - x86_smp_trampoline
	.word 0
reset_cs:
	xorw %bx, %bx
	movw %bx, %ds
	movw %bx, %es
	movw %bx, %ss

	addl %eax, (early_gdt_ptr_addr - x86_smp_trampoline)(%eax)
	lgdtl (early_gdt_ptr - x86_smp_trampoline)(%eax)
	movl %cr0, %ebx
	orl $1, %ebx
	movl %ebx, %cr0
	addl %eax, (jmp_32bit_frame - x86_smp_trampoline)(%eax)
	ljmpl *(jmp_32bit_frame - x86_smp_trampoline)(%eax)
jmp_32bit_frame:
	.long jmp_32bit - x86_smp_trampoline
	.word GDT_KERNEL_CS
jmp_32bit:
.code32
	movw $GDT_KERNEL_DS, %bx
	movw %bx, %ds
	movw %bx, %es
	movw %bx, %ss

	/*
	 * Perform the switch to 64-bit mode with the help of some variables
	 * on the stack.
	 */
	leal (stack_top - x86_smp_trampoline - 24)(%eax), %esp

	popl %ebx
	movl %ebx, %cr4

	popl %ebx
	movl %ebx, %cr3

	movl $MSR_EFER, %ecx
	xorl %edx, %edx
	movl %eax, %ebx
	popl %eax
	wrmsr

	popl %eax
	movl %eax, %cr0

	addl %ebx, (late_gdt_ptr_addr - x86_smp_trampoline)(%ebx)
	lgdt (late_gdt_ptr - x86_smp_trampoline)(%ebx)

	addl %ebx, (jmp_64bit_frame - x86_smp_trampoline)(%ebx)
	ljmp *(jmp_64bit_frame - x86_smp_trampoline)(%ebx)
jmp_64bit_frame:
	.long jmp_64bit - x86_smp_trampoline
	.word GDT_KERNEL_CS
jmp_64bit:
.code64
	/* force a jmp to an immediate */
	movq $enter_kernel, %rax
	jmp *%rax
early_gdt:
	.long 0, 0
	.long 0x0000ffff, 0x00cf9a00 /* KERNEL32 */
	.long 0x0000ffff, 0x008f9200 /* DATA32 */
early_gdt_ptr:
	.word early_gdt_ptr - early_gdt - 1
early_gdt_ptr_addr:
	.long early_gdt - x86_smp_trampoline
late_gdt:
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
late_gdt_ptr:
	.word late_gdt_ptr - late_gdt - 1
late_gdt_ptr_addr:
	.long late_gdt - x86_smp_trampoline
stack:
	.fill 0x1000 - (. - x86_smp_trampoline), 1, 0
stack_top:

.section .text
.balign 16
enter_kernel:
	popq %rax
	movq %rax, %rsp
	pushq $0
	xorq %rbp, %rbp
	jmp x86_smp_cpu_setupcode

.globl x86_smp_fixup_gdt
.type x86_smp_fixup_gdt, @function
.balign 16
x86_smp_fixup_gdt:
	subq $16, %rsp
	movw $(GDT_SIZE - 1), (%rsp)
	movq %rdi, 2(%rsp)
	lgdt (%rsp)
	movq $1f, (%rsp)
	movw $GDT_KERNEL_CS, 8(%rsp)
	rex64 ljmp *(%rsp)
1:
	movl $GDT_KERNEL_DS, %eax
	movl %eax, %ds
	movl %eax, %es
	movl %eax, %fs
	movl %eax, %gs
	movl %eax, %ss
	addq $16, %rsp
	ret
