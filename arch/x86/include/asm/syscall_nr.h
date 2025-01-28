/**
 * Syscall numbers on x86.
 * Copyright (C) 2025-present  dbstream
 *
 * This header is intended to be included by userspace.
 */
#ifndef __DAVIX_ASM_SYSCALL_NR_H
#define __DAVIX_ASM_SYSCALL_NR_H 1

#define __SYS_STABLE_MIN		0
#define __SYS_STABLE_MAX		1023

#define __SYS_UNSTABLE_MIN		1048576
#define __SYS_UNSTABLE_MAX		1049599

#define __SYS_enosys			0
#define __SYS_write_dmesg		1
#define __SYS_exit			2
#define __SYS_stat			3
#define __SYS_mknod			4
#define __SYS_reboot			5

#endif /* __DAVIX_ASM_SYSCALL_NR_H  */
