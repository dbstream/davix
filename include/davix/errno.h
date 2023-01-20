/* SPDX-License-Identifier: MIT */
#ifndef __DAVIX_ERRNO_H
#define __DAVIX_ERRNO_H

#define MAX_ERRNO 1023

#define IS_ERROR(x) ({ \
	long __x = (long) (x); \
	__x < 0 && __x > -MAX_ERRNO; \
})

#define ERROR_OF(x) ({ \
	long __x = (long) (x); \
	(int) -__x; \
})

#define ENOMEM 1

#endif /* __DAVIX_ERRNO_H */
