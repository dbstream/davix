/**
 * struct entry_regs
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#ifdef __ASSEMBLER__
#define IRET_R15	0
#define IRET_R14	8
#define IRET_R13	16
#define IRET_R12	24
#define IRET_RBX	32
#define IRET_RBP	40
#define IRET_R11	48
#define IRET_R10	56
#define IRET_R9		64
#define IRET_R8		72
#define IRET_RCX	80
#define IRET_RDX	88
#define IRET_RSI	96
#define IRET_RDI	104
#define IRET_RAX	112
#define IRET_ERRORCODE	120
#define IRET_RIP	128
#define IRET_CS		136
#define IRET_RFLAGS	144
#define IRET_RSP	152
#define IRET_SS		160
#else
#include <stdint.h>

struct entry_regs {
	/**
	 * These registers are not  scratch registers. Most interrupt handlers
	 * do not push them, instead offsetting the stack pointer for a
	 * consistent layout.
	 *
	 * %rsp is implicitly saved by hardware as part of the iret frame.
	 */
	uint64_t	saved_r15,
			saved_r14,
			saved_r13,
			saved_r12,
			saved_rbx,
			saved_rbp;

	/**
	 * These registers are defined by the System V ABI as scratch
	 * registers. Therefore, they must be saved and restored by
	 * interrupt handlers.
	 */
	uint64_t	saved_r11,
			saved_r10,
			saved_r9,
			saved_r8,
			saved_rcx,
			saved_rdx,
			saved_rsi,
			saved_rdi,
			saved_rax;

	uint64_t error_code;

	uint64_t rip;
	uint64_t cs;
	uint64_t rflags;
	uint64_t rsp;
	uint64_t ss;
};

#endif
