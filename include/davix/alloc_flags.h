/**
 * Generic memory allocation flags
 * Copyright (C) 2024  dbstream
 */
#ifndef DAVIX_ALLOC_FLAGS_H
#define DAVIX_ALLOC_FLAGS_H 1

typedef unsigned int alloc_flags_t;

#define _ALLOC_FLAG(n) (1U << n)
#define ALLOC_ZERO		_ALLOC_FLAG(0)	/* Zero the allocated memory. */
#define ALLOC_KERNEL		0

#endif /* DAVIX_ALLOC_FLAGS_H */
