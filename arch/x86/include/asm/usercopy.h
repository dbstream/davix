/**
 * Copying data to/from userspace.
 * Copyright (C) 2025-present  dbstream
 */
#ifndef _ASM_USERCOPY_H
#define _ASM_USERCOPY_H 1

#include <davix/errno.h>
#include <davix/stddef.h>
#include <asm/page_defs.h>

static inline errno_t
usercopy_ok (const void *addr, size_t size)
{
	unsigned long start = (unsigned long) addr;
	unsigned long end = start + size;
	if (end < start)
		return EFAULT;

	if (start < USER_MMAP_LOW)
		return EFAULT;
	if (end && end - 1UL > USER_MMAP_HIGH)
		return EFAULT;

	return ESUCCESS;
}

extern errno_t
__memcpy_from_userspace (void *dst, const void *src, size_t size);

extern errno_t
__memcpy_to_userspace (void *dst, const void *src, size_t size);

extern errno_t
__strncpy_from_userspace (char *dst, const char *src, size_t n);

static inline errno_t
memcpy_from_userspace (void *dst, const void *src, size_t size)
{
	errno_t e = usercopy_ok (src, size);
	if (e == ESUCCESS)
		e = __memcpy_from_userspace (dst, src, size);
	return e;
}

static inline errno_t
memcpy_to_userspace (void *dst, const void *src, size_t size)
{
	errno_t e = usercopy_ok (dst, size);
	if (e == ESUCCESS)
		e = __memcpy_to_userspace (dst, src, size);
	return e;
}

/**
 * BIG FAT WARNING:   This does NOT null-terminate *dst.
 */
static inline errno_t
strncpy_from_userspace (char *dst, const char *src, size_t n)
{
	errno_t e = usercopy_ok (src, n);
	if (e == ESUCCESS)
		e = __strncpy_from_userspace (dst, src, n);
	return e;
}

#endif /*  _ASM_USERCOPY_H  */
