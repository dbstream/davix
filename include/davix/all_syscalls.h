/**
 * List of all kernel syscalls.
 * Copyright (C) 2025-present  dbstream
 */
#ifndef _DAVIX_ALL_SYSCALLS_H
#define _DAVIX_ALL_SYSCALLS_H

#include <asm/all_syscalls.h>

#define ALL_SYSCALLS(macro)		\
	ARCH_ALL_SYSCALLS(macro)	\
	macro(write_dmesg)		\
	macro(exit)			\
	macro(stat)			\
	macro(mknod)			\
	macro(reboot)			\
	macro(unlink)			\


#endif /* _DAVIX_ALL_SYSCALLS_H  */
