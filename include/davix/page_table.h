/* SPDX-License-Identifier: MIT */
#ifndef __DAVIX_PAGE_TABLE_H
#define __DAVIX_PAGE_TABLE_H

typedef enum pgcachemode {
	PG_WRITEBACK,		/* Normal, "cacheable" memory. Use this when possible. */
#ifdef CONFIG_HAVE_PGCACHEMODE_WRITETHROUGH
	PG_WRITETHROUGH,	/* Cache reads but not writes. */
#endif
#ifdef CONFIG_HAVE_PGCACHEMODE_WRITECOMBINING
	PG_WRITECOMBINE,	/* Don't cache anything, but do combine consecutive writes. */
#endif
#ifdef CONFIG_HAVE_PGCACHEMODE_UNCACHED_MINUS
	PG_UNCACHED_MINUS,	/* Same as uncached, but can be overridden by platform. */
#endif
	PG_UNCACHED,		/* Don't cache anything. */
	NUM_PG_CACHEMODES
} pgcachemode_t;

/*
 * Fall-back to PG_UNCACHED if arch doesn't provide one of the above.
 */
#ifndef CONFIG_HAVE_PGCACHEMODE_WRITETHROUGH
#define PG_WRITETHROUGH PG_UNCACHED
#endif

#ifndef CONFIG_HAVE_PGCACHEMODE_WRITECOMBINING
#define PG_WRITECOMBINING PG_UNCACHED
#endif

#ifndef CONFIG_HAVE_PGCACHEMODE_UNCACHED_MINUS
#define PG_UNCACHED_MINUS PG_UNCACHED
#endif

#endif /* __DAVIX_PAGE_TABLE_H */
