/* SPDX-License-Identifier: MIT */
#ifndef __DAVIX_MM_TYPES_H
#define __DAVIX_MM_TYPES_H

#include <davix/types.h>

struct page {
	unsigned long flags;
	union {
		struct {
			struct list list;
			unsigned order;
		} buddy;
		unsigned long _pad[7];
	};
};

static_assert(sizeof(struct page) == 8 * sizeof(unsigned long),
	"sizeof(struct page)");

#define PAGE_IN_FREELIST (1UL << 0)

#endif /* __DAVIX_MM_TYPES_H */
