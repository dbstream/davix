/**
 * Implementations of memcpy, memmove and memset for x86.
 * Copyright (C) 2024  dbstream
 */
	.file "memcpy.S"
	.code64

#include <asm/sections.h>

	__TEXT

	.globl memset
	.type memset, @function
	.balign 16
memset:
	.cfi_startproc
	movl %esi, %eax
	movq %rdx, %rcx
	movq %rdi, %rdx
	rep stosb
	movq %rdx, %rax
	ret
	.cfi_endproc
	.size memset, . - memset

	.globl memcpy
	.type memcpy, @function
	.balign 16
memcpy:
	.cfi_startproc
	movq %rdx, %rcx
	movq %rdi, %rax
	rep movsb
	ret
	.cfi_endproc
	.size memcpy, . - memcpy

	.globl memmove
	.type memmove, @function
	.balign 16
memmove:
	.cfi_startproc
	movq %rdi, %rax
	movq %rdx, %rcx
	cmpq %rdi, %rsi
	jbe .Lbackward
	rep movsb
	ret
.Lbackward:
	addq %rcx, %rdi
	addq %rcx, %rsi
	decq %rdi
	decq %rsi
	std
	rep movsb
	cld
	ret
	.cfi_endproc
	.size memmove, . - memmove
