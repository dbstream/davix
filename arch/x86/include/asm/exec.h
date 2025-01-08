/**
 * Architecture-specific things for exec().
 * Copyright (C) 2025-present  dbstream
 */
#ifndef _ASM_EXEC_H
#define _ASM_EXEC_H 1

#define ARCH_EXEC_STACK_ALIGNMENT 8

struct exec_state;

extern void
arch_exec_jump_to_userspace (struct exec_state *state);

#endif /* _ASM_EXEC_H */
