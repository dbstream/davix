/**
 * Generic memory allocation flags
 * Copyright (C) 2024  dbstream
 */
#ifndef DAVIX_ALLOC_FLAGS_H
#define DAVIX_ALLOC_FLAGS_H 1

typedef unsigned int alloc_flags_t;

/** 
 * Overview of allocation flags:
 *   __ALLOC_ZERO		Zero the allocated memory.
 *   __ALLOC_ACCT		Prevent allocation if the total number of
 *				reserved pages is too high.
 *   __ALLOC_RESERVED		Decrement total_reserved_pages if the allocation
 *				succeeds. This flag overrides __ALLOC_ACCT.
 *
 * Effects of allocation flags on free:
 *   __ALLOC_ACCT		No effect.
 *   __ALLOC_RESERVED		Increment total_reserved_pages by one.
 */

#define _ALLOC_FLAG(n) (1U << n)
#define __ALLOC_ZERO			_ALLOC_FLAG(0)
#define __ALLOC_ACCT			_ALLOC_FLAG(1)
#define __ALLOC_RESERVED		_ALLOC_FLAG(2)

#define ALLOC_KERNEL		0
#define ALLOC_USER		(__ALLOC_ZERO | __ALLOC_ACCT)
#define ALLOC_USER_RESERVED	(ALLOC_USER | __ALLOC_RESERVED)

#endif /* DAVIX_ALLOC_FLAGS_H */
