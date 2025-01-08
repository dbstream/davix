/**
 * Common handling for exec().
 * Copyright (C) 2025-present  dbstream
 */
#ifndef _DAVIX_EXEC_H
#define _DAVIX_EXEC_H 1

#include <davix/errno.h>
#include <davix/stddef.h>
#include <davix/sys_types.h>

struct file;

extern errno_t
kernel_fexecve (struct file *file, char **argv, char **envp);

struct exec_state {
	struct file *file;
	char **argv;
	char **envp;

	unsigned int has_reached_point_of_no_return : 1;
	unsigned int stack_grows_down : 1;

	unsigned long entry_point_addr;
	void *user_stack_addr;
	void *user_stack_end;
	unsigned long user_stack_reserved;
};

struct exec_driver {
	errno_t (*do_execve) (struct exec_state *state);
};

extern const struct exec_driver exec_driver_elf;

extern errno_t
exec_point_of_no_return (struct exec_state *state);

extern errno_t
exec_mmap_file (unsigned long addr, size_t size, int prot,
		struct file *file, off_t offset);

extern errno_t
exec_mprotect_range (unsigned long addr, size_t size);

extern errno_t
exec_mmap_stack (struct exec_state *state, size_t size, int prot);

extern errno_t
exec_push_to_stack (struct exec_state *state, const void *buf, size_t size,
		void **out_addr);

#endif /* _DAVIX_EXEC_H  */
