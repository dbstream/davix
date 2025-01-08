/**
 * exec() -  architecture-specific handling
 * Copyright (C) 2025-present  dbstream
 */
#include <davix/exec.h>
#include <asm/exec.h>

/**
 * The first three arguments, %rdi, %rsi, and %rdx, are passed directly to
 * userspace. This is intended to be argc, argv, and envp.
 */
extern void
asm_exec_jump_to_userspace (unsigned long rdi, unsigned long rsi,
		unsigned long rdx, unsigned long rip, void *rsp);

void
arch_exec_jump_to_userspace (struct exec_state *state)
{
	asm_exec_jump_to_userspace (0, 0, 0, state->entry_point_addr, state->user_stack_addr);
}
