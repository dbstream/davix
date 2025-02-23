/**
 * Common handling for exec().
 * Copyright (C) 2025-present  dbstream
 */
#include <davix/align.h>
#include <davix/context.h>
#include <davix/exec.h>
#include <davix/file.h>
#include <davix/mm.h>
#include <davix/panic.h>
#include <davix/sched_types.h>
#include <davix/stddef.h>
#include <asm/exec.h>
#include <asm/mm.h>
#include <asm/usercopy.h>

#include <davix/printk.h>

static const struct exec_driver *exec_drivers[] = {
	&exec_driver_elf,
	NULL
};

errno_t
exec_point_of_no_return (struct exec_state *state)
{
	preempt_off ();
	struct task *self = get_current_task ();
	struct process_mm *mm = mmnew ();
	if (!mm)
		return ENOMEM;

	struct process_mm *oldmm = self->mm;
	self->mm = mm;

	state->has_reached_point_of_no_return = 1;
	switch_to_mm (mm);
	if (oldmm)
		mmput (oldmm);

	preempt_on ();
	return ESUCCESS;
}

errno_t
exec_mmap_file (unsigned long addr, size_t size, int prot,
		struct file *file, off_t offset)
{
	void *mmap_out_addr;
	if (file->ops->mmap)
		return ksys_mmap ((void *) addr, size, prot,
				MAP_PRIVATE | MAP_FIXED_NOREPLACE,
				file, offset, &mmap_out_addr);

	errno_t e = ksys_mmap ((void *) addr, size,
			(prot & PROT_WRITE) ? prot : PROT_WRITE,
			MAP_PRIVATE | MAP_ANON | MAP_FIXED_NOREPLACE,
			NULL, 0, &mmap_out_addr);

	if (e != ESUCCESS)
		return e;

	ssize_t nread;
	e = kernel_read_file (file, mmap_out_addr, size, offset, &nread);
	if (e != ESUCCESS)
		return e;

	if ((size_t) nread != size)
		return EIO;

	if (!(prot & PROT_WRITE))
		return exec_mprotect_range (addr, size, prot);
	return ESUCCESS;
}

errno_t
exec_mprotect_range (unsigned long addr, size_t size, int prot)
{
	return ksys_mprotect ((void *) addr, size, prot);
}

errno_t
exec_mmap_stack (struct exec_state *state, size_t size, int prot)
{
	void *stack_addr;
	errno_t e = ksys_mmap (NULL, size, prot, MAP_ANON | MAP_PRIVATE,
			NULL, 0, &stack_addr);
	if (e != ESUCCESS)
		return e;

	if (state->stack_grows_down) {
		state->user_stack_addr = stack_addr + size;
		state->user_stack_end = stack_addr;
	} else {
		state->user_stack_addr = stack_addr;
		state->user_stack_end = stack_addr + size;
	}

	return ESUCCESS;
}

errno_t
exec_push_to_stack (struct exec_state *state, const void *buf, size_t size,
		void **out_addr)
{
	size_t rem;
	if (state->stack_grows_down)
		rem = state->user_stack_addr - state->user_stack_end;
	else
		rem = state->user_stack_end - state->user_stack_addr;

	if (state->user_stack_reserved > rem)
		state->user_stack_reserved = rem;

	if (!size)
		return EINVAL;

	size = ALIGN_UP (size, ARCH_EXEC_STACK_ALIGNMENT);
	if (!size || size > rem - state->user_stack_reserved)
		return E2BIG;

	if (state->stack_grows_down) {
		state->user_stack_addr -= size;
		errno_t e = memcpy_to_userspace (state->user_stack_addr, buf, size);
		if (e != ESUCCESS)
			return e;
		if (out_addr)
			*out_addr = state->user_stack_addr;
	} else {
		errno_t e = memcpy_to_userspace (state->user_stack_addr, buf, size);
		if (e != ESUCCESS)
			return e;
		if (out_addr)
			*out_addr = state->user_stack_addr;
		state->user_stack_addr += size;
	}

	return ESUCCESS;
}

static void
free_exec_state (struct exec_state *state)
{
	// TODO: free argv and envp vectors
	if (state->file)
		fput (state->file);
}

static void
go_to_userspace (struct exec_state *state)
{
	free_exec_state (state);
	arch_exec_jump_to_userspace (state);
}

errno_t
kernel_fexecve (struct file *file, char **argv, char **envp)
{
	const struct exec_driver **drivers = exec_drivers;
	struct exec_state state = {
		.file = file,
		.argv = argv,
		.envp = envp
	};
	do {
		const struct exec_driver *driver = *drivers;
		errno_t e = driver->do_execve (&state);
		if (e == ESUCCESS)
			go_to_userspace (&state);
		if (state.has_reached_point_of_no_return)
			panic ("exec TODO: gracefully handle point of no return");
		drivers++;
	} while (*drivers);
	free_exec_state (&state);
	return ENOEXEC;
}
