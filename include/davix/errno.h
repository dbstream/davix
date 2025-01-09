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

#endif /* _DAVIX_ERRNO_H */

