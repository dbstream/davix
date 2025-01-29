/**
 * Error codes.
 * Copyright (C) 2024  dbstream
 */
#ifndef _DAVIX_ERRNO_H
#define _DAVIX_ERRNO_H 1

/**
 * Conventions for error handling:
 * - All error codes are non-zero positive integers.
 * - Functions which can return an error should return an errno_t. In this
 *   case, zero indicates success.
 * - Return values should be placed in a passed pointer.
 */

#ifndef __ASSEMBLER__
typedef int errno_t;
#endif

#define ESUCCESS 0
#define ENOMEM 1
#define EIO 2
#define ENOTSUP 3
#define EAGAIN 4
#define ETIME 5
#define EINTR 6
#define EINVAL 7
#define EFAULT 8
#define EEXIST 9
#define ENOEXEC 10
#define E2BIG 11
#define ENOSYS 12
#define EPERM 13
#define ENXIO 14
#define ENOENT 15
#define ELOOP 16
#define EXDEV 17
#define ENOTDIR 18
#define EISDIR 19
#define ENAMETOOLONG 20
#define ENOSPC 21

#define __ALL_ERRNOS(macro) \
	macro (ESUCCESS)		\
	macro (ENOMEM)			\
	macro (EIO)			\
	macro (ENOTSUP)			\
	macro (EAGAIN)			\
	macro (ETIME)			\
	macro (EINTR)			\
	macro (EINVAL)			\
	macro (EFAULT)			\
	macro (EEXIST)			\
	macro (ENOEXEC)			\
	macro (E2BIG)			\
	macro (ENOSYS)			\
	macro (EPERM)			\
	macro (ENXIO)			\
	macro (ENOENT)			\
	macro (ELOOP)			\
	macro (EXDEV)			\
	macro (ENOTDIR)			\
	macro (EISDIR)			\
	macro (ENAMETOOLONG)		\
	macro (ENOSPC)			\


#endif /* _DAVIX_ERRNO_H */

