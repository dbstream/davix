/**
 * x86/multiboot2 entry point.
 * Copyright (C) 2025-present  dbstream
 *
 * High-level overview of the boot process:
 *	- POST, bootloader entry, etc...
 *	- Bootloader loads our kernel image into 2M-aligned contiguous memory.
 *	- We identity-map all of lower 4G, then switch to 64-bit mode.
 *	- We setup RWX mappings of the kernel at -2G.
 *	- Enter the kernel.
 */
	.file "head.S"
	.altmacro

#include <asm/asm.h>
#include <asm/creg_bits.h>
#include <asm/msr_bits.h>
#include "multiboot.h"

	__HEAD

header:
	.long MB2_MAGIC
	.long MB2_ARCH_X86
	.long .Lheader_end - header
	.long MB2_CHECKSUM(.Lheader_end - header)
	.word MB2_TAG_INFO_REQ
	.word 0
	.long 8
	.word MB2_TAG_LOAD_ADDR
	.word 0
	.long 24
	.long 0
	.long 0xffffffff
	.long 0
	.long __multiboot_bss_end
	.word MB2_TAG_ENTRY_ADDR
	.word 0
	.long 12
	.long _start32_multiboot2 - header
	.long 0
	.word MB2_TAG_RELOCATABLE
	.word 0
	.long 24
	.long 0x200000
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
.Lheader_end:

	.balign 16

	/**
	 * 32-bit protocol entrypoint into the kernel.
	 *
	 * We are running at a relocated address.  %ebx points to the boot
	 * information structure.  All other registers are undefined.
	 */
	.globl _start32_multiboot2
	.type _start32_multiboot2, @function
_start32_multiboot2:
	.code32

	cli		/** Disable interrupts.  */

	/**
	 * Calculate the load offset.  To do this, we repurpose one of the
	 * fields of the boot information structure as a tiny stack.
	 */
	leal 8(%ebx), %esp
	movl -4(%esp), %eax
	call .Lon_relocated_addr
.Lon_relocated_addr:
	popl %ebp
	movl %eax, -4(%esp)
	subl $(.Lon_relocated_addr - header), %ebp

	/**
	 * Repurpose the header as a tiny stack for performing initialization.
	 */
	leal (_start32_multiboot2 - header)(%ebp), %esp

	/** Give us a GDT we can use for later.  */
	subl $8, %esp
	leal (boot_gdt - header)(%ebp), %eax
	movw $0x17, (%esp)
	movl %eax, 2(%esp)
	lgdt (%esp)
	addl $8, %esp

	pushl %ebx	/** Save the boot information for later use.  */

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
	jmp .Lla48_la57_common
.Lno_la57:
	leal (0x4003 + boot_pagetables - header)(%ebp), %eax
	movl %eax, (0x1ff8 + boot_pagetables - header)(%ebp)

	movl %cr4, %eax
	leal (0x1000 + boot_pagetables - header)(%ebp), %edx
	orl $__CR4_PAE, %eax
.Lla48_la57_common:

	movl %eax, %cr4
	movl %edx, %cr3

	movl $MSR_EFER, %ecx
	movl $_EFER_LME, %eax
	xorl %edx, %edx
	wrmsr	/* At this point, enabling paging would enter long mode. */

	leal (.Lon_64bit - header)(%ebp), %eax

	popl %ebx	/* Restore pointer to boot information. */

	pushl $0x08
	pushl %eax

	movl $(__CR0_PE | __CR0_MP | __CR0_ET | \
		__CR0_NE | __CR0_WP | __CR0_AM | __CR0_PG), %eax
	movl %eax, %cr0		/* This instruction enters long mode. */
	lretl			/* This instruction begins 64-bit execution. */
.Lon_64bit:
	.code64
	movl $0x10, %eax
	movl %eax, %ds
	movl %eax, %es
	movl %eax, %fs
	movl %eax, %gs
	movl %eax, %ss

	/** Setup the kernel mapping at -2G. */
	movl $512, %ecx
	leaq top_pml2s(%rip), %rdi
	leaq (0x83 + header)(%rip), %rax
.Lmap_kernel_loop:
	stosq
	addq $0x200000, %rax
	loop .Lmap_kernel_loop

	/**
	 * Install the kmap_fixed page table.
	 * (phys == load_offset + virt - 0xffffffff80000000)
	 */
	leaq (__kmap_fixed_page - 0xffffffff7ffffffd)(%rip), %rax
	movq %rax, (0x1ff8 + top_pml2s)(%rip)

	movq $__startup_bsp, %rax
	movl %ebx, %edi
	leaq header(%rip), %rsi

	/** Set up a 4KiB stack for the kernel.  */
	movq $(0xffffffff80000000 + boot_pagetables - header), %rsp

	/** Jump to __startup_bsp.  */
	jmp *%rax

	.size _start32_multiboot2, . - _start32_multiboot2

	.balign 64
boot_gdt:
	.quad 0
	.quad 0x00af9b000000ffff	/* %cs */
	.quad 0x00af93000000ffff	/* %ds, %es, %fs, %gs, %ss */

	.org 0x4000
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
