/**
 * Syscall handlers.
 * Copyright (C) 2025-present  dbstream
 */
#ifndef _ASM_SYSCALL_H
#define _ASM_SYSCALL_H 1

#include <asm/entry.h>

typedef struct entry_regs *syscall_regs_t;

#define SYSCALL_ARG1(regs) ((regs)->saved_rdi)
#define SYSCALL_ARG2(regs) ((regs)->saved_rsi)
#define SYSCALL_ARG3(regs) ((regs)->saved_rdx)
#define SYSCALL_ARG4(regs) ((regs)->saved_r10)
#define SYSCALL_ARG5(regs) ((regs)->saved_r8)
#define SYSCALL_ARG6(regs) ((regs)->saved_r9)
#define SYSCALL_NR(regs) ((regs)->error_code)
#define SYSCALL_RET(regs) ((regs)->saved_rax)

#define __SYSCALL_RETURN_VOID(regs) do {		\
	(regs)->saved_rax = 0;				\
	return 0;					\
} while (0)

#define __SYSCALL_RETURN_SUCCESS(regs, value) do {	\
	(regs)->saved_rax = (unsigned long) (value);	\
	return 0;					\
} while (0)

#define __SYSCALL_RETURN_ERROR(regs, errno) do {	\
	(regs)->saved_rax = (errno);			\
	return 1;					\
} while (0)

#endif /* _ASM_SYSCALL_H */
