/**
 * Physical memory tracking before core mm is up and running.
 * Copyright (C) 2024  dbstream
 *
 * We use two lists: initmem_usable_ram and initmem_reserved, representing all
 * available system RAM (potentially including reserved/allocated regions) and
 * reserved/allocated regions. Memory allocation inserts into initmem_reserved,
 * which serves as an avoid-list. Each list is sorted by address.
 *
 * The name initmem_usable_ram is misleading, as it can represent other memory
 * types than conventional RAM (see initmem_flags_t below).
 *
 * Notably, initmem_usable_ram is not __INIT_DATA, because it is used as a
 * reference to all of physical memory later on.
 */
#ifndef _DAVIX_INITMEM_H
#define _DAVIX_INITMEM_H 1

#include <davix/stdbool.h>

extern bool mm_is_early;

typedef unsigned int initmem_flags_t;

#define INITMEM_FLAG(n) (1U << n)

/* "Special purpose" - memory which is ear-marked for a particular driver. */
#define INITMEM_SPECIAL_PURPOSE		INITMEM_FLAG(0)

/* Persistent RAM, sometimes referred to as non-volatile RAM. */
#define INITMEM_PERSISTENT		INITMEM_FLAG(1)

/* Turn the initmem_register call into an initmem_reserve call. */
#define INITMEM_RESERVE			INITMEM_FLAG(2)

/* ACPI NVS memory that must be saved and restored across a sleep. */
#define INITMEM_ACPI_NVS		INITMEM_FLAG(3)

/* Memory which is reserved for kernel data. */
#define INITMEM_KERN_RESERVED		INITMEM_FLAG(4)

/* Memory which should not be used during kernel boot. */
#define INITMEM_AVOID_DURING_INIT	INITMEM_FLAG(5)

/* This shouldn't be set by external users of the API. */
#define INITMEM_INTERNAL		INITMEM_FLAG(31)

/* The following flags are valid for external callers of the API to set. */
#define INITMEM_ALLOWED_FLAGS_MASK					\
	(INITMEM_SPECIAL_PURPOSE | INITMEM_PERSISTENT		\
	| INITMEM_RESERVE | INITMEM_ACPI_NVS			\
	| INITMEM_KERN_RESERVED | INITMEM_AVOID_DURING_INIT)

/* Memory with these flags are skipped while allocating */
#define INITMEM_DONT_ALLOCATE_MASK	\
	(INITMEM_SPECIAL_PURPOSE | INITMEM_PERSISTENT		\
	| INITMEM_ACPI_NVS | INITMEM_AVOID_DURING_INIT)

/* Regions with these flags cannot be merged */
#define INITMEM_DONT_MERGE_MASK		\
	(INITMEM_SPECIAL_PURPOSE | INITMEM_PERSISTENT | INITMEM_INTERNAL)

/* These flags cause initmem_register to append to initmem_reserved instead. */
#define INITMEM_RESERVE_MASK		\
	(INITMEM_RESERVE | INITMEM_KERN_RESERVED)

/**
 * initmem_range - Represents a range of physical memory.
 */
struct initmem_range {
	unsigned long start;
	unsigned long end;
	initmem_flags_t flags;
};

struct initmem_vector {
	unsigned long num_entries;
	unsigned long max_entries;
	struct initmem_range *entries;
	char name[];
};

extern struct initmem_vector initmem_usable_ram;
extern struct initmem_vector initmem_reserved;

/* Print the contents of an initmem_vector. */
extern void
initmem_dump (struct initmem_vector *vector);

/* Register a region of usable RAM with initmem. */
extern void
initmem_register (unsigned long start, unsigned long size,
	initmem_flags_t flags);

/* Reserve a region of memory. */
extern void
initmem_reserve (unsigned long start, unsigned long size,
	initmem_flags_t flags);

/**
 * Iterate over memory ranges. *it_a and *it_b should be initialized to zero.
 * Returns false if there are no more entries.
 */
extern int
initmem_next_free (struct initmem_range *out,
	unsigned long *it_a, unsigned long *it_b);

/**
 * The same as initmem_next_free, but aligns the region start and end to
 * 'align' bytes.
 */
extern int
initmem_next_free_aligned (struct initmem_range *out,
	unsigned long *it_a, unsigned long *it_b, unsigned long align);

/**
 * Allocate physical memory that lies completely within [start, end).
 */
extern unsigned long
initmem_alloc_phys_range (unsigned long size, unsigned long align,
	unsigned long start, unsigned long end);

/**
 * Allocate virtual memory whose backing physical memory is contained
 * completely within [start, end).
 */
extern void *
initmem_alloc_virt_range (unsigned long size, unsigned long align,
	unsigned long start, unsigned long end);

extern unsigned long
initmem_alloc_phys (unsigned long size, unsigned long align);

extern void *
initmem_alloc_virt (unsigned long size, unsigned long align);

/* Free memory using physical address. */
extern void
initmem_free_phys (unsigned long start, unsigned long size);

/* Free memory using virtual address. */
extern void
initmem_free_virt (void *start, unsigned long size);

#endif /* _DAVIX_INITMEM_H */
