/* SPDX-License-Identifier: MIT */
#include <davix/types.h>
#include <davix/list.h>
#include "memmap.h"
#include "printk.h"

static const char *memmap_type(enum memmap_type type)
{
	return type == MEMMAP_USABLE_RAM ? "System RAM" :
		type == MEMMAP_LOADER ? "Loader" :
		type == MEMMAP_ACPI_DATA ? "ACPI Tables" :
		type == MEMMAP_ACPI_NVS ? "ACPI NVS memory" :
		type == MEMMAP_KERNEL ? "Kernel & Modules" :
		"Reserved";
}

static struct memmap_entry *alloc_entry(struct memmap *map)
{
	if(list_empty(&map->free_list))
		return NULL;

	struct memmap_entry *entry =
		container_of(map->free_list.next, struct memmap_entry, list);

	list_del(&entry->list);
	return entry;
}

static void free_entry(struct memmap *map, struct memmap_entry *entry)
{
	list_add(&map->free_list, &entry->list);
}

void memmap_init(struct memmap *map)
{
	list_init(&map->entry_list);
	list_init(&map->free_list);
	for(unsigned long i = 0; i < MAX_MEMMAP_ENTRIES; i++) {
		struct memmap_entry *entry = &map->entries[i];
		list_add(&map->free_list, &entry->list);
	}
}

void memmap_copy(struct memmap *dst, struct memmap *src)
{
	memmap_init(dst);
	struct memmap_entry *entry;
	list_for_each(entry, &src->entry_list, list) {
		struct memmap_entry *copied_entry = alloc_entry(dst);
		if(!copied_entry) {
			error("memmap: cannot alloc entry for [mem %p - %p] %s",
				entry->start, entry->end - 1,
				memmap_type(entry->type));
			continue;
		}

		copied_entry->start = entry->start;
		copied_entry->end = entry->end;
		copied_entry->type = entry->type;
		list_radd(&dst->entry_list, &copied_entry->list);
	}
}

static void memmap_remove_if_less(struct memmap *map,
	unsigned long start, unsigned long end, enum memmap_type type)
{
	struct memmap_entry *entry, *tmp;
	list_for_each_safe(entry, &map->entry_list, list, tmp) {
		if(entry->start >= end)
			break;
		if(entry->end <= start)
			continue;
		if(entry->type >= type)
			continue;

		if(entry->start >= start) {
			if(entry->end <= end) {
				list_del(&entry->list);
				free_entry(map, entry);
				continue;
			}

			entry->start = end;
			break;
		}

		if(entry->end > end) {
			tmp = alloc_entry(map);
			if(!tmp) {
				error("memmap: cannot alloc entry for [mem %p - %p] %s\n",
					end, entry->end - 1, memmap_type(entry->type));
				break;
			}
			tmp->start = end;
			tmp->end = entry->end;
			tmp->type =  entry->type;
			list_add(&entry->list, &tmp->list);
			entry->end = start;
			break;
		}

		entry->end = start;
	}
}

static struct memmap_entry *get_enext(struct memmap *map, struct list *pos)
{
	if(pos->next == &map->entry_list)
		return NULL;
	return container_of(pos->next, struct memmap_entry, list);
}

static void memmap_fill_regions(struct memmap *map,
	unsigned long start, unsigned long end, enum memmap_type type)
{
	struct list *pos;
	struct memmap_entry *eprev, *enext;

	pos = &map->entry_list;
	eprev = NULL;
	enext = list_empty(pos)
		? NULL : container_of(pos->next, struct memmap_entry, list);

	for(;;) {
loop_start:;
		unsigned long hole_start = 0;
		unsigned long hole_end = 0xfffffffffffff000UL;
		int coalesce_left = 0;
		int coalesce_right = 0;
		if(eprev) {
			hole_start = eprev->end;
			if(eprev->type == type && hole_start >= start)
				coalesce_left = 1;
		}
		if(enext) {
			hole_end = enext->start;
			if(enext->type == type && hole_end <= end)
				coalesce_right = 1;
		}

		if(hole_start >= end)
			break;
		if(hole_end <= start)
			goto loop_next;
		if(hole_start == hole_end)
			goto loop_next;

		if(coalesce_left) {
			if(coalesce_right) {
				struct memmap_entry *old = enext;
				enext = get_enext(map, &enext->list);
				list_del(&old->list);
				eprev->end = old->end;
				free_entry(map, old);
				goto loop_start;
			}
			if(hole_end >= end) {
				eprev->end = end;
				break;
			}
			eprev->end = hole_end;
		} else if(coalesce_right) {
			enext->start = (hole_end > start)
				? hole_end : start;
		} else {
			struct memmap_entry *new_entry = alloc_entry(map);
			if(!new_entry) {
				error("memmap: cannot alloc entry for [mem %p - %p] %s\n",
					hole_start > start
						? hole_start : start,
					(hole_end < end
						? hole_end : end) - 1,
					memmap_type(type));
				break;
			}
			new_entry->start = hole_start > start
				? hole_start : start;
			new_entry->end = hole_end < end
				? hole_end : end;
			new_entry->type = type;
			list_add(pos, &new_entry->list);
		}

loop_next:
		if(!enext)
			break;
		eprev = enext;
		pos = &eprev->list;
		enext = get_enext(map, pos);
	}
}

static void __memmap_update_region(struct memmap *map,
	unsigned long start, unsigned long end, enum memmap_type type)
{
	if(end > 0xfffffffffffff000UL)
		end = 0xfffffffffffff000UL;
	if(end < start)
		return;
	memmap_remove_if_less(map, start, end, type);
	memmap_fill_regions(map, start, end, type);
}

void memmap_update_region(struct memmap *map,
	unsigned long start, unsigned long end, enum memmap_type type,
	int silent)
{
	if(!silent)
		debug("memmap: update region [mem %p - %p] => %s\n",
			start, end, memmap_type(type));

	__memmap_update_region(map, start, end, type);
}

void memmap_dump(char loglevel, const char *what, struct memmap *map)
{
	struct memmap_entry *entry;

	printk(loglevel, "%s:\n", what);
	unsigned long i = 0;
	list_for_each(entry, &map->entry_list, list) {
		printk(loglevel, "  [%2lu]: [mem %p - %p]: %s\n",
			i++,
			entry->start,
			entry->end - 1,
			memmap_type(entry->type));
	}
}
