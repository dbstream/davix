/**
 * Page allocation and deallocation.
 * Copyright (C) 2024  dbstream
 */
#include <davix/page.h>

/**
 * For now, these functions are placeholders.
 * pfn_entry is still needed for slab
 */

struct pfn_entry *
alloc_page (alloc_flags_t flags, unsigned int order)
{
	(void) flags;
	(void) order;
	return NULL;
}

void
free_page (struct pfn_entry *page, unsigned int order)
{
	(void) page;
	(void) order;
}
