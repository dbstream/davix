/**
 * Definitions for openat() and friends.
 * Copyright (C) 2025-present  dbstream
 *
 * This interface takes inspiration from the Linux openat2 interface.
 */
#ifndef _DAVIX_UAPI_OPENAT_H
#define _DAVIX_UAPI_OPENAT_H 1

#include <davix/uapi/types.h>

#define AT_FDCWD	-1	/* Resolve from the process working directory.  */
#define AT_FDROOT	-2	/* Resolve from the filesystem root.  */


/**
 *	O_READ		Open the file in read mode.
 *	O_WRITE		Open the file in write mode.
 *	O_RDWR		Shorthand for O_READ|O_WRITE.
 *	O_APPEND	Open the file in append mode.  Before each write(), the
 *			file offset is positioned at the end of the file, as if
 *			with lseek().
 *	O_CREAT		Allow creating the file if it did not exist.
 *	O_CREAT_USE_UMASK	Specify that the file mode creation mask set by
 *			umask() should be applied instead of the permission bits
 *			in 'mode'.
 *	O_EXCL		If the file already existed, fail the syscall.
 *	O_FILETYPE	If the file existed, fail unless its file type bits
 *			match those in 'mode'.
 *	O_NOFOLLOW	If the trailing component of the pathname is a symlink,
 *			fail with ELOOP.
 */
#define O_READ			0x00000001
#define O_WRITE			0x00000002
#define O_RDWR			(O_READ | O_WRITE)
#define O_APPEND		0x00000004
#define O_CREAT			0x00000008
#define O_CREAT_USE_UMASK	0x00000010
#define O_EXCL			0x00000020
#define O_FILETYPE		0x00000040
#define O_NOFOLLOW		0x00000080

/**
 *	RESOLVE_BENEATH		Require all the components of the path to be a
 *				descendant of the starting directory.
 *	RESOLVE_IN_ROOT		Treat the starting directory as the filesystem
 *				root.
 *	RESOLVE_NOSYMLINK	Do not follow symbolic links.  NOSPECIALSYMLINK
 *				is implied.
 *	RESOLVE_NOSPECIALSYMLINK	Disallow certain "special" symbolic
 *				links from being followed, such as /proc/self.
 *	RESOLVE_NOXDEV		Do not cross filesystem boundaries.
 *	RESOLVE_NOIO		Do not allow IO operations to be initiated by
 *				the lookup.
 */
#define RESOLVE_BENEATH			0x01
#define RESOLVE_IN_ROOT			0x02
#define RESOLVE_NOSYMLINK		0x04
#define RESOLVE_NOSPECIALSYMLINK	0x08
#define RESOLVE_NOXDEV			0x10
#define RESOLVE_NOIO			0x20

/**
 * Pathwalk information from userspace.
 */
struct pathwalk_info {
	int	flags;		/** O_* flags.  */
	mode_t	mode;		/** File mode.  */
	int	resolve;	/** RESOLVE_* flags.  */
};

#endif /** _DAVIX_UAPI_OPENAT_H  */
