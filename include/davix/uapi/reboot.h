/**
 * Defintions for SYS_reboot.
 * Copyright (C) 2025-present  dbstream
 *
 * The reboot() syscall takes the following form:
 *
 *	void
 *	SYS_reboot (int cmd, int arg, unsigned int mag0, unsigned int mag1);
 *
 * SYS_reboot takes the following arguments:
 *
 *	'cmd' is the reboot command (REBOOT_CMD_*).
 *	'arg' is the argument, with different meanings depending on the value
 *	of 'cmd'.
 *	'mag0' must be DAVIX_REBOOT_MAG0.
 *	'mag1' must be DAVIX_REBOOT_MAG1.
 *
 * If either of 'mag0' or 'mag1' doesn't hold the correct value, the syscall
 * fails with EINVAL.
 *
 * Only the superuser can call SYS_reboot.  For other users, the syscall will
 * fail.
 */
#ifndef _DAVIX_UAPI_REBOOT_H
#define _DAVIX_UAPI_REBOOT_H 1

#define DAVIX_REBOOT_MAG0	0x44585057U	/* "DXPW" */
#define DAVIX_REBOOT_MAG1	0x524d414eU	/* "RMAN" */

/**
 * Query support for other SYS_reboot commands.  'arg' is a REBOOT_CMD_* to
 * query support for.  Returns ESUCCESS if 'arg' is supported and ENOTSUP if
 * it is not.
 */
#define REBOOT_CMD_QUERY_SUPPORT 0

/**
 * Shutdown the system.  'arg' is reserved and must be zero.  Doesn't return
 * on success.
 */
#define REBOOT_CMD_POWEROFF 12345678

#endif /* _DAVIX_UAPI_REBOOT_H  */
