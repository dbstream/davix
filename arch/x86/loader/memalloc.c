/* SPDX-License-Identifier: MIT */
#include <davix/list.h>
#include "memalloc.h"
#include "memmap.h"

void *__memalloc(unsigned long size, unsigned long align,
	unsigned long start, unsigned long end, enum memmap_type type)
{
	struct memmap_entry *entry;
	list_for_each(entry, &memmap.entry_list, list) {
		if(entry->type != MEMMAP_USABLE_RAM)
			continue;

		unsigned long area_start = \
			align_up(max(start, entry->start), align);

		unsigned long area_end = min(end, entry->end);

		if(area_end < area_start)
			continue;
		if(area_end - area_start < size)
			continue;

		memmap_update_region(&memmap,
			area_start, area_start + size,
			type, 1);
		return (void *) area_start;
	}
	return NULL;
}
