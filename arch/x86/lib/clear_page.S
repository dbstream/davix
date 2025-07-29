// SPDX-License-Identifier: GPL-3.0
/*
 * File: lib/clear_page.S
 * Fast full page zeroing.
 *
 * Copyright (C) 2025  dbstream
 */
	.file "clear_page.S"
	.code64

	.text

	/**
	 * clear_page - zero out a full page of memory
	 * @rdi: pointer to virtual address of page to be written to
	 */
	.globl clear_page
	.type clear_page, @function
clear_page:
	.cfi_startproc
	xorq %rax, %rax
	movl $512, %ecx
	/*
	 * NB: Calling convention specifies DF=0 on function entry.
	 */
	rep stosq
	ret
	.cfi_endproc
	.size clear_page, . - clear_page

