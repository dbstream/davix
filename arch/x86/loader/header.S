/* SPDX-License-Identifier: MIT */
#include <asm/segment.h>
#include <asm/msr.h>
#include "multiboot.h"

.section .boot

boot_header:
	.long MB2_MAGIC
	.long MB2_ARCH_X86
	.long (boot_header_end - boot_header)
	.long MB2_CHECKSUM(boot_header_end - boot_header)
tag_info_req:
	.word MB2_TAG_INFO_REQ
	.word 0
	.long (tag_end -tag_info_req)
tag_end:
	.word MB2_TAG_END
	.word 0
	.long (boot_header_end - tag_end)
boot_header_end:

.globl _start
.type _start, @function
_start:
.code32
	cli
	cld

	/* Setup a small stack for booting */
	movl $boot_stack_end, %esp
	movl %esp, %ebp
	pushl %ebx	/* multiboot2 information is stored in %ebx */

	/* Check for CPUID support */
	pushfl
	popl %eax
	movl %eax, %ebx
	xorl $(1 << 21), %eax
	pushl %eax
	popfl
	pushfl
	popl %eax
	cmpl %eax, %ebx
	je halt32

	/* Check for extmax >= 0x80000001 */
	movl $0x80000000, %eax
	cpuid
	cmpl $0x80000001, %eax
	jb halt32

	/* Check for long mode support */
	movl $0x80000001, %eax
	cpuid
	test $(1 << 29), %eax
	jb halt32

	/* Setup page tables */
	movl $boot_p4d, %eax
	orl $3, %eax
	movl %eax, boot_p5d

	movl $boot_p3d, %eax
	orl $3, %eax
	movl %eax, boot_p4d

	movl $boot_p2d_1, %eax
	orl $3, %eax
	movl %eax, boot_p3d

	movl $boot_p2d_2, %eax
	orl $3, %eax
	movl %eax, (boot_p3d + 8)

	movl $boot_p2d_3, %eax
	orl $3, %eax
	movl %eax, (boot_p3d + 16)

	movl $boot_p2d_4, %eax
	orl $3, %eax
	movl %eax, (boot_p3d + 24)

	movl $0, %ecx
	movl $512, %esi
p2d_loop:
	movl $(1 << 21), %eax
	mull %ecx
	orl $0b10000011, %eax
	movl %eax, boot_p2d_1(,%ecx,8)
	addl $(1 << 30), %eax
	movl %eax, boot_p2d_2(,%ecx,8)
	addl $(1 << 30), %eax
	movl %eax, boot_p2d_3(,%ecx,8)
	addl $(1 << 30), %eax
	movl %eax, boot_p2d_4(,%ecx,8)
	incl %ecx
	cmpl %ecx, %esi
	jne p2d_loop

	/* Disable paging */
	movl %cr0, %eax
	andl $~__CR0_PG, %eax
	movl %eax, %cr0

	/* Check for CPUID max >= 7 */
	movl $0, %eax
	cpuid
	cmpl $7, %eax
	jb no_la57

	/* Check for LA57 support */
	movl $7, %eax
	xorl %ecx, %ecx
	cpuid
	test $(1 << 16), %ecx
	jz no_la57

	/* Set cr4.PAE to 1, cr4.LA57 to 1 and clear cr4.PGE to 0 */
	movl %cr4, %eax
	orl $(__CR4_PAE | __CR4_LA57), %eax
	andl $(~__CR4_PGE), %eax
	movl %eax, %cr4

	movl $boot_p5d, %eax
	movl %eax, %cr3

	jmp after_la57_check
no_la57:

	/* Set cr4.PAE to 1 and clear cr4.PGE to 0 */
	movl %cr4, %eax
	orl $__CR4_PAE, %eax
	andl $(~__CR4_PGE), %eax
	movl %eax, %cr4

	movl $boot_p4d, %eax
	movl %eax, %cr3

after_la57_check:

	/* Set EFER.LME to 1 */
	movl $MSR_EFER, %ecx
	xorl %edx, %edx
	movl $_EFER_LME, %eax
	wrmsr

	/* Enable paging */
	movl %cr0, %eax
	orl $__CR0_PG, %eax
	movl %eax, %cr0

	/* Set cr4.PGE to 1 */
	movl %cr4, %eax
	orl $__CR4_PGE, %eax
	movl %eax, %cr4

	popl %ebx

	subl $8, %esp
	movw $(boot_gdt_end - boot_gdt), (%esp)
	movl $(boot_gdt), 2(%esp)
	lgdt (%esp)
	addl $8, %esp
	jmpl $8,$_start64
halt32:
	hlt
	jmp halt32

_start64:
.code64
	movw $16, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %gs
	movw %ax, %ss

	movq $boot_stack_end, %rsp
	movq %rsp, %rbp
	pushq $0
	pushq $0
	movl %ebx, %edi
	call loader_main

	/*
	 * When we return to here from loader_main(), the kernel entry point is
	 * in ``%rax``.
	 */
	jmp *%rax

.balign 16
boot_gdt:
	.long 0, 0
	.long 0x0000ffff, 0x00af9a00
	.long 0x0000ffff, 0x00cf9200
boot_gdt_end:

.balign 16
boot_stack:
	.fill 0x1000, 1, 0
boot_stack_end:

.section .boot_data
.balign 0x1000
boot_p2d_1:
.fill 0x1000, 1, 0
boot_p2d_2:
.fill 0x1000, 1, 0
boot_p2d_3:
.fill 0x1000, 1, 0
boot_p2d_4:
.fill 0x1000, 1, 0
boot_p3d:
.fill 0x1000, 1, 0
boot_p4d:
.fill 0x1000, 1, 0
boot_p5d:
.fill 0x1000, 1, 0
