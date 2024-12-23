/**
 * Bitmap utilities.
 * Copyright (C) 2024  dbstream
 */
#ifndef _DAVIX_BITMAP_H
#define _DAVIX_BITMAP_H

#include <davix/atomic.h>
#include <davix/stdint.h>

typedef unsigned long bitmap_t;

#define BITMAP_SIZE(num_bits)			\
	(((num_bits) + MACHINE_BITS - 1) / MACHINE_BITS)

#define BITMAP_DEFINE(name, num_bits)	bitmap_t name[BITMAP_SIZE(num_bits)]

static inline unsigned long
bitmap_idx (unsigned long bit_idx)
{
	return bit_idx / MACHINE_BITS;
}

static inline unsigned long
bitmap_bit_idx (unsigned long bit_idx)
{
	return bit_idx % MACHINE_BITS;
}

static inline unsigned long
bitmap_bit (unsigned long bit_idx)
{
	return 1UL << bitmap_bit_idx (bit_idx);
}

static inline unsigned long
bitmap_get (bitmap_t *bitmap, unsigned long bit)
{
	return atomic_load_relaxed (&bitmap[bitmap_idx(bit)]) & bitmap_bit(bit);
}

static inline void
bitmap_set (bitmap_t *bitmap, unsigned long bit)
{
	atomic_or_fetch_relaxed (&bitmap[bitmap_idx(bit)], bitmap_bit(bit));
}

static inline void
bitmap_clear (bitmap_t *bitmap, unsigned long bit)
{
	atomic_and_fetch_relaxed (&bitmap[bitmap_idx(bit)], ~bitmap_bit(bit));
}

/**
 * Get the next set bit in a bitmap.
 * Arguments:
 *	bitmap: pointer to bitmap.
 *	size: size of the bitmap.
 *	curr: pointer to iterator variable, should be initialized to zero.
 * Returns zero if all set bits are exhausted; non-zero otherwise.
 */
extern int
bitmap_next_set_bit (bitmap_t *bitmap, unsigned long size, unsigned long *curr);

#define bitmap_for_each(bitmap, size, it)	\
	for (unsigned long it = -1UL;		\
		bitmap_next_set_bit ((bitmap), (size), &it); )

#endif /* _DAVIX_BITMAP_H */
