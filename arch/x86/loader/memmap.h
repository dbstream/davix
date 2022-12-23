/* SPDX-License-Identifier: MIT */
#ifndef __MEMMAP_H
#define __MEMMAP_H

#include <davix/types.h>
#include <asm/boot.h>

#define MAX_MEMMAP_ENTRIES 128
struct memmap {
	struct list entry_list;
	struct list free_list;
	struct memmap_entry entries[MAX_MEMMAP_ENTRIES];
};

/*
 * The original loader memmap, containing multiboot2 memmap entries *mostly*
 * untampered.
 */
extern struct memmap orig_memmap;

/*
 * The memmap we pass to the main kernel. This memmap is initialized with the
 * entries from the original loader memmap, but is modified by, e.g., memalloc
 * functions.
 */
extern struct memmap memmap;

void memmap_init(struct memmap *map);

void memmap_copy(struct memmap *dst, struct memmap *src);

void memmap_update_region(struct memmap *map,
	unsigned long start, unsigned long end, enum memmap_type type,
	int silent);

void memmap_dump(char loglevel, const char *what, struct memmap *map);

#endif /* __MEMMAP_H */
