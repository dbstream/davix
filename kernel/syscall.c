/**
 * Tracing points for syscall.
 * Copyright (C) 2025-present  dbstream
 *
 * This is mainly meant as a debugging aid when developing the kernel.
 */
#include <davix/all_syscalls.h>
#include <davix/errno.h>
#include <davix/printk.h>
#include <davix/syscall.h>
#include <asm/syscall_nr.h>

static inline int
syscall_tracing_enabled (void)
{
	return 0;
}

static inline const char *
sysnr_to_string (unsigned long nr)
{
	switch (nr) {
#define def_sysnr_to_string(name) case __SYS_##name: return "SYS_" #name;
		ALL_SYSCALLS(def_sysnr_to_string)
#undef def_sysnr_to_string
	default:
		return "<unknown syscall>";
	}
}

static inline const char *
errno_to_string (errno_t e)
{
	switch (e) {
#define def_errno_to_string(name) case name: return #name;
		__ALL_ERRNOS(def_errno_to_string)
#undef def_errno_to_string
	default:
		return "<unknown errno>";
	}
}

void
trace_syscall_enter (syscall_regs_t regs)
{
	if (!syscall_tracing_enabled ())
		return;

	printk ("trace_syscall_enter:  syscall %3lu  %s();\n",
			(unsigned long) SYSCALL_NR(regs),
			sysnr_to_string ((unsigned long) SYSCALL_NR(regs)));
}

void
trace_syscall_exit (syscall_regs_t regs, int is_error)
{
	if (!syscall_tracing_enabled ())
		return;

	if (is_error) {
		printk ("trace_syscall_exit:   syscall %3lu  %s()=%2ld  %s\n",
				(unsigned long) SYSCALL_NR(regs),
				sysnr_to_string ((unsigned long) SYSCALL_NR(regs)),
				(long) SYSCALL_RET(regs),
				errno_to_string ((errno_t) SYSCALL_RET(regs)));
	} else {
		printk ("trace_syscall_exit:   syscall %3lu  %s()=%ld\n",
				(unsigned long) SYSCALL_NR(regs),
				sysnr_to_string ((unsigned long) SYSCALL_NR(regs)),
				(long) SYSCALL_RET(regs));
	}
}
