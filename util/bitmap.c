/**
 * Bitmap utilities.
 * Copyright (C) 2024  dbstream
 */
#include <davix/bitmap.h>

/* Get the least significant bit of 'word', which is guaranteed non-zero. */
static inline unsigned long
get_lsb (unsigned long word)
{
	return __builtin_ctzl (word);
}

int
bitmap_next_set_bit (bitmap_t *bitmap, unsigned long size, unsigned long *curr)
{
	unsigned long bit_idx = *curr;
	unsigned long word_idx = 0;
	if (bit_idx == -1UL)
		goto loop;

	word_idx = bitmap_idx (bit_idx);
	if (bitmap_bit_idx (bit_idx) < MACHINE_BITS - 1) {
		unsigned long word = bitmap[word_idx];
		word >>= bitmap_bit_idx (bit_idx) + 1;
		if (word) {
			*curr += get_lsb (word) + 1;
			return 1;
		}
	}

	word_idx++;
loop:
	while (word_idx < BITMAP_SIZE(size)) {
		unsigned long word = bitmap[word_idx];
		if (word) {
			*curr = MACHINE_BITS * word_idx + get_lsb (word);
			return 1;
		}

		word_idx++;
	}

	return 0;
}
