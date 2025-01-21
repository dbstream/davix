/**
 * Assembly routines for copying data to and from userspace.
 * Copyright (C) 2025-present  dbstream
 */
	.file "usercopy.S"
	.code64

#include <davix/errno.h>
#include <asm/sections.h>

	__TEXT

	.balign 64

	.globl __memcpy_from_userspace
	.type __memcpy_from_userspace, @function
__memcpy_from_userspace:
	.cfi_startproc
	jmp usercopy_common
	.cfi_endproc
	.size __memcpy_from_userspace, . - __memcpy_from_userspace

	.globl __memcpy_to_userspace
	.type __memcpy_to_userspace, @function
__memcpy_to_userspace:
	.cfi_startproc
	jmp usercopy_common
	.cfi_endproc
	.size __memcpy_to_userspace, . - __memcpy_to_userspace

	.globl __usercopy_pagefault_address

	.type usercopy_common, @function
usercopy_common:
	.cfi_startproc
	xorq %rax, %rax
	movq %rdx, %rcx
__usercopy_pagefault_address:
	rep movsb
	ret
	.cfi_endproc
	.size usercopy_common, . - usercopy_common

	.globl __usercopy_fixup_address
	.type __usercopy_fixup_address, @function
__usercopy_fixup_address:
	.cfi_startproc
	movl $EFAULT, %eax
	ret
	.cfi_endproc
	.size __usercopy_fixup_address, . - __usercopy_fixup_address

	.globl __userstrcpy_pagefault_address

	.globl __strncpy_from_userspace
	.type __strncpy_from_userspace, @function
__strncpy_from_userspace:
	.cfi_startproc

1:
__userstrcpy_pagefault_address:
	lodsb (%rsi), %al
	stosb %al, (%rdi)

	testb %al, %al
	jz 1f

	decq %rdx
	jnz 1b

1:	xorq %rax, %rax
	ret

	.cfi_endproc
	.size __strncpy_from_userspace, . - __strncpy_from_userspace
