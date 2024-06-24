/**
 * Low-level kernel entry
 * Copyright (C) 2024  dbstream
 */
#ifndef _ASM_ENTRY_H
#define _ASM_ENTRY_H 1

#ifdef __ASSEMBLER__
/* define offsets from the iret_frame */
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
#else /* __ASSEMBLER__ */

struct entry_regs {
	/**
	 * These registers are not  scratch registers. Most interrupt handlers
	 * do not push them, instead offsetting the stack pointer for a
	 * consistent layout.
	 *
	 * %rsp is implicitly saved by hardware as part of the iret frame.
	 */
	unsigned long	saved_r15,
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
	unsigned long	saved_r11,
			saved_r10,
			saved_r9,
			saved_r8,
			saved_rcx,
			saved_rdx,
			saved_rsi,
			saved_rdi,
			saved_rax;

	unsigned long error_code;

	unsigned long rip;
	unsigned long cs;
	unsigned long rflags;
	unsigned long rsp;
	unsigned long ss;
};


#endif /* !__ASSEMBLER__ */
#endif /* _ASM_ENTRY_H */
