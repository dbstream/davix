/**
 * Kernel syscalls on x86.
 * Copyright (C) 2025-present  dbstream
 *
 * SYSCALL ABI:
 *	Register	Register usage on entry		Register usage on exit
 *	%rax		Syscall number			Result or errno_t
 *	%rdi		First argument			clobbered
 *	%rsi		Second argument			clobbered
 *	%rdx		Third argument			clobbered
 *	%r10		Fourth argument			clobbered
 *	%r8		Fifth argument			clobbered
 *	%r9		Sixth argument			clobbered
 *	%rflags						cleared, .CF indicates error
 *	%rcx						clobbered
 *	%r11						clobbered
 */
#include <asm/cpulocal.h>
#include <davix/all_syscalls.h>
#include <davix/errno.h>
#include <davix/printk.h>
#include <davix/stdbool.h>
#include <davix/syscall.h>
#include <asm/entry.h>
#include <asm/irq.h>
#include <asm/page_defs.h>
#include <asm/rflags.h>
#include <asm/syscall_nr.h>

SYSCALL0(void, enosys)
{
	syscall_return_error (ENOSYS);
}

static int
handle_syscall (struct entry_regs *regs);

__CPULOCAL unsigned long __syscall_saved_rsp;

/**
 *  Handle a syscall.
 *
 * Return true if we need to return via iretq.
 **/
bool
x86_handle_syscall (struct entry_regs *regs)
{
	regs->rsp = this_cpu_read (&__syscall_saved_rsp);
	irq_enable ();

	int is_error = handle_syscall (regs);

	irq_disable ();
	this_cpu_write (&__syscall_saved_rsp, regs->rsp);
	regs->saved_r11 = (is_error ? __RFL_CF : 0) | __RFL_INITIAL;

	/**
	 * Apparently noncanonical %rip can cause kernel-mode #GP.  And if the
	 * return %rip has changed because of the syscall, we need to return via
	 * iretq.
	 */
	if (regs->rip != regs->saved_rcx || regs->rip > USER_MMAP_HIGH)
		return true;

	return false;
}

#define declare_syscalls(name) extern int __wrap_sys_##name (struct entry_regs *);
	ALL_SYSCALLS(declare_syscalls)
#undef declare_syscalls

static int
handle_syscall (struct entry_regs *regs)
{
	switch (regs->saved_rax) {
#define dispatch_syscalls(name) case __SYS_##name:	return __wrap_sys_##name (regs);
	ALL_SYSCALLS(dispatch_syscalls)
#undef dispatch_syscalls
	default:
		return __wrap_sys_enosys (regs);
	};
}
