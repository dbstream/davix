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

typedef int errno_t;

#define ESUCCESS 0
#define ENOMEM 1
#define EIO 2
#define ENOTSUP 3

#endif /* _DAVIX_ERRNO_H */

