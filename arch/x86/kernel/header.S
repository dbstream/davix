/**
 * Image header and protocol entry points.
 * Copyright (C) 2024  dbstream
 *
 * High-level overview of the boot process:
 *	- POST, bootloader entry, etc..
 *	- Bootloader loads our kernel image into any 2MiB-aligned contiguous
 *	physical memory region.
 *	- Identity-map all of lower 4GiB, then switch to 64-bit mode.
 *	- Given load offset X, map -2GiB...-1GiB to X.
 *	- Pass boot information to __startup64_bsp.
 */
	.file "header.S"
	.altmacro

#include <asm/cregs.h>
#include <asm/gdt.h>
#include <asm/msr.h>
#include <asm/sections.h>
#include "multiboot.h"

	__HEAD

header:
	.long MB2_MAGIC
	.long MB2_ARCH_X86
	.long header_end - header
	.long MB2_CHKSUM(header_end - header)
	.word MB2_TAG_INFO_REQ
	.word 0
	.long 8
	.word MB2_TAG_LOAD_ADDR
	.word 0
	.long 24
	.long 0
	.long 0xffffffff
	.long 0
	.long 0
	.word MB2_TAG_ENTRY_ADDR
	.word 0
	.long 12
	.long _start32_multiboot2 - header
	.long 0
	.word MB2_TAG_RELOCATABLE
	.word 0
	.long 24
	.long 0x100000
	.long 0xbfffffff
	.long 0x200000
	.long 0
	.word MB2_TAG_FRAMEBUFFER
	.word 0
	.long 20
	.long 0
	.long 0
	.long 32
	.long 0
	.word MB2_TAG_END
	.word 0
	.long 8
header_end:

	/**
	 * 32-bit protocol entrypoint into the kernel.
	 * We are running at a relocated address. TODO:
	 *	- Install 64-bit page tables.
	 *	- Switch to 64-bit mode.
	 *	- Jump to __startup64_bsp.
	 */
	.globl _start32_multiboot2
_start32_multiboot2:
	.code32

	/* Do a very evil hack to calculate the load offset. */
	leal 8(%ebx), %esp
	movl (%esp), %eax
	call 1f
1:	popl %ebp
	movl %eax, (%esp)
	subl $(1b-header), %ebp		/* %ebp now contains the load offset. */

	leal (header_end-header)(%ebp), %esp	/* The header can be used as a stack. */

	/* Give us a GDT we can use for later. */
	subl $8, %esp
	leal (boot_gdt-header)(%ebp), %eax
	movw $0x2f, (%esp)	/* make the GDT big enough for __KERNEL_CS */
	movl %eax, 2(%esp)
	lgdt (%esp)
	addl $8, %esp

	pushl %ebx	/* Save boot information for later use. */

	/**
	 * These are the page tables to relocate:
	 *	- PML5
	 *	- Bottom-half PML4
	 *	- Top-half PML4
	 *	- Bottom-half PML3
	 *	- Top-half PML3
	 * Relocate the pointers to the next-level page tables. For the PML3s,
	 * relocate four pointers to PML2s which reside at the bottom and top,
	 * respectively, of the page tables.
	 *
	 * Later, (when LA57 is checked for,) we must properly install the
	 * top-half PML3 into the bottom-half PML4 if LA57 is not available.
	 */
	addl %ebp, (0x0000 + boot_pagetables - header)(%ebp)	/* PML5[0] */
	addl %ebp, (0x0ff8 + boot_pagetables - header)(%ebp)	/* PML5[511] */
	addl %ebp, (0x1000 + boot_pagetables - header)(%ebp)	/* Bottom PML4[0] */
	addl %ebp, (0x2ff8 + boot_pagetables - header)(%ebp)	/* Top PML4[511] */
	addl %ebp, (0x3000 + boot_pagetables - header)(%ebp)	/* Bottom PML3[0] */
	addl %ebp, (0x3008 + boot_pagetables - header)(%ebp)	/* Bottom PML3[1] */
	addl %ebp, (0x3010 + boot_pagetables - header)(%ebp)	/* Bottom PML3[2] */
	addl %ebp, (0x3018 + boot_pagetables - header)(%ebp)	/* Bottom PML3[3] */
	addl %ebp, (0x4ff0 + boot_pagetables - header)(%ebp)	/* Top PML3[510] */
	addl %ebp, (0x4ff8 + boot_pagetables - header)(%ebp)	/* Top PML3[511] */

	/**
	 * Now we check for LA57 support.
	 */
	xorl %eax, %eax
	cpuid
	cmpl $7, %eax
	jb .Lno_la57
	movl $7, %eax
	xorl %ecx, %ecx
	cpuid
	testl $(1 << 16), %ecx
	jz .Lno_la57

	/* Setup %eax and %edx with the desired values of %cr4 and %cr3. */

	movl %cr4, %eax
	leal (boot_pagetables - header)(%ebp), %edx
	orl $(__CR4_PAE | __CR4_LA57), %eax
	jmp 1f
.Lno_la57:
	leal (0x4003 + boot_pagetables - header)(%ebp), %eax
	movl %eax, (0x1ff8 + boot_pagetables - header)(%ebp)

	movl %cr4, %eax
	leal (0x1000 + boot_pagetables - header)(%ebp), %edx
	orl $__CR4_PAE, %eax
1:
	/**
	 * This is a very precise sequence of instructions. Important ones are
	 * marked with a comment.
	 */

	movl %eax, %cr4
	movl %edx, %cr3

	movl $MSR_EFER, %ecx
	movl $_EFER_LME, %eax
	xorl %edx, %edx
	wrmsr	/* <-- (!) */
		/* At this point, enabling paging would enter long mode. */

	leal (1f-header)(%ebp), %eax

	popl %ebx	/* <-- (!) Restore pointer to boot information. */

	pushl $__KERNEL_CS
	pushl %eax

	movl $(__CR0_PE | __CR0_MP | __CR0_ET | \
		__CR0_NE | __CR0_WP | __CR0_AM | __CR0_PG), %eax
	movl %eax, %cr0		/* <-- (!) This instruction enters long mode. */
	lretl			/* <-- (!) This switches to 64-bit execution. */

1:	/* <-- (!) Start of 64-bit execution. */
	.code64
	movl $__KERNEL_DS, %eax
	movl %eax, %ds
	movl %eax, %es
	movl %eax, %fs
	movl %eax, %gs
	movl %eax, %ss

	/* Map the kernel range at -2G. */
	movl $512, %ecx
	leaq top_pml2s(%rip), %rdi
	leaq (0x83+header)(%rip), %rax
1:	stosq
	addq $0x200000, %rax
	loop 1b

	leaq (__early_memmap_page - 0xffffffff7ffffffd)(%rip), %rax
	movq %rax, (0x1ff8 + top_pml2s)(%rip)

	movq $__startup64_bsp, %rax
	movl %ebx, %edi
	leaq header(%rip), %rsi
	jmp *%rax

	.org 0xfd0
boot_gdt:
	.quad 0
	.quad 0
	.quad 0
	.quad 0
	.quad 0x00af93000000ffff	/* __KERNEL_DS */
	.quad 0x00af9b000000ffff	/* __KERNEL_CS */

boot_pagetables:
	/* 0x0000: PML5 */
	.quad (boot_pagetables - header) + 0x1003
	.fill 0xff0, 1, 0
	.quad (boot_pagetables - header) + 0x2003
	/* 0x1000: Bottom PML4 */
	.quad (boot_pagetables - header) + 0x3003
	.fill 0xff8, 1, 0
	/* 0x2000: Top PML4 */
	.fill 0xff8, 1, 0
	.quad (boot_pagetables - header) + 0x4003
	/* 0x3000: Bottom PML3 */
	.quad (boot_pagetables - header) + 0x5003
	.quad (boot_pagetables - header) + 0x6003
	.quad (boot_pagetables - header) + 0x7003
	.quad (boot_pagetables - header) + 0x8003
	.fill 0xfe0, 1, 0
	/* 0x4000: Top PML3 */
	.fill 0xff0, 1, 0
	.quad (boot_pagetables - header) + 0x9003
	.quad (boot_pagetables - header) + 0xa003
	/* 0x5000: Bottom PML2s */
	.macro fill_bottom_pml2 i
	.quad \i + 0x83
	.endm
	.set i, 0
	.rept 2048
	fill_bottom_pml2 %i
	.set i, i + 0x200000
	.endr

	/* 0x9000: Top PML2s */
top_pml2s:
	.fill 0x2000, 1, 0
