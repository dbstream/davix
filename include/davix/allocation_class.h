/**
 * allocation_class, a way of specifying accounting information for allocations
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

/**
 * Allocation classes.
 *
 * NOTE: users of memory allocation APIs that take these as an argument should
 * avoid the use of underscored (__ALLOC-prefixed) allocation classes.
 *
 * __ALLOC_ZERO			Zero the allocated memory.
 * __ALLOC_WASRESERVED		Allocate a page which has previously been
 *				reserved.
 * __ALLOC_HIGHPRIO		Perform a high-priority allocation.  These
 *				bypass the low memory watermark.
 */
enum allocation_class : unsigned int {
	__ALLOC_ZERO		= 1U << 0,
	__ALLOC_WASRESERVED	= 1U << 1,
	__ALLOC_HIGHPRIO	= 1U << 2,

	ALLOC_KERNEL		= 0,
	ALLOC_USER		= __ALLOC_ZERO
};
