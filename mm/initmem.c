/**
 * Physical memory services in early stages of the kernel.
 * Copyright (C) 2024  dbstream
 *
 * NOTE on initmem_cond_try_extend: when reserving memory, prefer calling after
 * the initmem_append to initmem_reserved. This is because the memory allocated
 * by initmem_try_extend might overlap with the region being reserved.
 *
 * NOTE on allocation strategy: we prefer top-down allocations over bottom-up
 * ones, in order to:
 *	1) Keep low memory (<16M or <4G) free for devices which might need it.
 *	2) Prevent problems related to allocating addr 0.
 * Case 2 doesn't happen in practice, because architectures like x86 reserve
 * some amount of low memory very early. Top-down allocations come at a cost in
 * fragmentation, which might prevent the use of huge page sizes. Therefore, at
 * a certain threshold (usually 4G), we stop and use bottom-up allocation for
 * addresses above that.
 */
#include <davix/align.h>
#include <davix/initmem.h>
#include <davix/panic.h>
#include <davix/printk.h>
#include <davix/stdbool.h>
#include <davix/stddef.h>
#include <davix/string.h>
#include <asm/page_defs.h>
#include <asm/sections.h>

bool mm_is_early = true;

#if __SIZEOF_LONG__ > 4
#define BOTTOMUP_ALLOCATION_THRESHOLD	0x100000000UL	/* 4GiB */
#endif

#define STATIC_USABLE_REGIONS 128
#define STATIC_RESERVED_REGIONS STATIC_USABLE_REGIONS

/**
 * Watermark for when initmem should start allocating more memory to extend
 * the vectors.
 */
#define WMARK_LOW_REGION_COUNT 8

static struct initmem_range static_usable_regions[STATIC_USABLE_REGIONS];

struct initmem_vector initmem_usable_ram = {
	.num_entries = 0,
	.max_entries = STATIC_USABLE_REGIONS,
	.entries = static_usable_regions,
	.name = "initmem_usable_ram"
};

__INIT_DATA
static struct initmem_range static_reserved_regions[STATIC_RESERVED_REGIONS];

__INIT_DATA struct initmem_vector initmem_reserved = {
	.num_entries = 0,
	.max_entries = STATIC_RESERVED_REGIONS,
	.entries = static_reserved_regions,
	.name = "initmem_reserved"
};

static inline void
initmem_print (const char *prefix, struct initmem_range *range)
{
	printk (PR_INFO "  %s: [%#lx - %#lx%s%s%s%s%s%s]\n",
		prefix,
		range->start, range->end - 1,
		(range->flags & INITMEM_SPECIAL_PURPOSE) ? " SP" : "",
		(range->flags & INITMEM_PERSISTENT) ? " PMEM" : "",
		(range->flags & INITMEM_RESERVE) ? " RESERVED" : "",
		(range->flags & INITMEM_ACPI_NVS) ? " NVS" : "",
		(range->flags & INITMEM_KERN_RESERVED) ? " KERN" : "",
		(range->flags & INITMEM_AVOID_DURING_INIT) ? " AVOID" : "");
}

__INIT_TEXT
void
initmem_dump (struct initmem_vector *vector)
{
	printk (PR_INFO "Dump of %s:\n", vector->name);
	for (unsigned long i = 0; i < vector->num_entries; i++)
		initmem_print (vector->name, &vector->entries[i]);
}

static bool initmem_extending;

/**
 * Extend an initmem_vector. This is tricky, because we need to allocate memory
 * using initmem, but we must not cause recursion vs ourselves.
 */
__INIT_TEXT
static void
initmem_try_extend (struct initmem_vector *vector)
{
	/* Prevent recursion. */
	if (initmem_extending)
		return;

	unsigned long curr_entries = vector->max_entries;
	unsigned long curr_size = sizeof (struct initmem_range) * curr_entries;
	unsigned long new_entries = 2 * curr_entries;
	unsigned long new_size = 2 * curr_size;

	initmem_extending = true;
	void *new_array = initmem_alloc_virt (new_size,
		sizeof (struct initmem_range));
	initmem_extending = false;

	if (!new_array)
		return;

	printk (PR_INFO "initmem: extended %s (%lu --> %lu entries)\n",
		vector->name, curr_entries, new_entries);

	void *old = vector->entries;
	memcpy (new_array, old, curr_size);
	vector->entries = new_array;
	vector->max_entries = new_entries;

	/* Cannot free the static regions. */
	if (old == (void *) static_usable_regions)	return;
	if (old == (void *) static_reserved_regions)	return;

	initmem_free_virt (old, curr_size);
}

static void
initmem_cond_try_extend (struct initmem_vector *vector)
{
	if (vector->max_entries - vector->num_entries <= WMARK_LOW_REGION_COUNT)
			initmem_try_extend (vector);
}

static inline bool
can_merge (initmem_flags_t a, initmem_flags_t b)
{
	if ((a | b) & INITMEM_DONT_MERGE_MASK)
		return false;
	return a == b;
}

/**
 * Insert 'range' into 'vector', discarding anything already there.
 */
__INIT_TEXT
static void
initmem_append (struct initmem_vector *vector, struct initmem_range *range)
{
	/* sanity check */
	if (range->start == range->end)
		return;

	/* We consume at most two additional entries in the vector. */
	if (vector->max_entries - vector->num_entries < 2)
		panic ("out-of-memory in initmem_append");

	struct initmem_range *inserted = NULL;
	unsigned long i = 0;
	while (i < vector->num_entries) {
		struct initmem_range *e = &vector->entries[i];
		if (e->start <= range->start && e->end >= range->start) {
			/* there is overlap with 'e' */
			if (can_merge (e->flags, range->flags)) {
				/* merge into 'e' */
				if (e->end >= range->end)
					/* we are covered entirely by 'e' */
					return;
				inserted = e;
				inserted->end = range->end;
				break;
			}

			/* adjust 'e' to not overlap with us */
			if (e->end <= range->end) {
				e->end = range->start;
				if (e->start == e->end) {
					/* we removed 'e' entirely, re-use it */
					inserted = e;
					*inserted = *range;
					break;
				}

				i++;
				break;
			}

			/* split 'e' into two and place ourselves in between */
			unsigned long move_n = vector->num_entries - i;
			memmove (e + 2, e, move_n * sizeof(*e));
			vector->num_entries += 2;
			e[0].end = range->start;
			e[1] = *range;
			e[2].start = range->end;
			return;
		}

		if (e->start > range->start)
			break;
		i++;
	}

	if (!inserted) {
		/* create an entry at vector->entries[i] */
		unsigned long move_n = vector->num_entries - i;
		inserted = &vector->entries[i];
		memmove (inserted + 1, inserted, move_n * sizeof (*inserted));
		vector->num_entries++;
		*inserted = *range;
	}

	/* left is clean, now we need to sort out the entries above us */
	unsigned long n_remove = 0;	/* number of entries to remove */
	while (i + n_remove + 1 < vector->num_entries) {
		struct initmem_range *e = &vector->entries[i + n_remove + 1];
		if (e->start <= inserted->end) {
			/* we border or overlap with 'e', check for merging */
			if (can_merge (e->flags, inserted->flags)) {
				/* merge with 'e' */
				if (e->end > inserted->end)
					inserted->end = e->end;
				n_remove++;
				continue;
			}
		}

		if (e->end > inserted->end) {
			if (e->start < inserted->end)
				e->start = inserted->end;
			/* disjoint entries */
			break;
		}

		n_remove++;
	}

	/* i is the inserted entry, n_remove is the number of entries to zap */
	memmove (inserted + 1, inserted + 1 + n_remove,
		(vector->num_entries - i - 1 - n_remove) * sizeof(*inserted));
	vector->num_entries -= n_remove;
}

/**
 * Remove anything within the region given by 'start' and 'size'.
 */
__INIT_TEXT
static void
initmem_delete (struct initmem_vector *vector,
	unsigned long start, unsigned long size)
{
	struct initmem_range range = {
		.start = start,
		.end = start + size,
		.flags = INITMEM_INTERNAL
	};

	initmem_append (vector, &range);

	for (unsigned long i = 0; i < vector->num_entries; i++) {
		struct initmem_range *e = &vector->entries[i];
		if (e->flags != INITMEM_INTERNAL)
			continue;

		memmove (e, e + 1, (vector->num_entries - i) * sizeof(*e));
		vector->num_entries--;
		return;
	}
}

/**
 * Validate that the given flags are allowed to be passed in from the
 * outside world.
 */
#define validate(flags) ({					\
	__auto_type _flags = flags;				\
	if (_flags & ~INITMEM_ALLOWED_FLAGS_MASK)		\
		panic ("Illegal flags %#x passed to %s.",	\
		_flags, __func__);				\
})

/**
 * Register a new memory range to the initmem.
 */
__INIT_TEXT
void
initmem_register (unsigned long start, unsigned long size,
	initmem_flags_t flags)
{
	validate (flags);

	struct initmem_range range = {
		.start = start,
		.end = start + size,
		.flags = flags
	};

	if (flags & INITMEM_RESERVE_MASK) {
		range.flags &= ~INITMEM_RESERVE;
		initmem_append (&initmem_reserved, &range);
		initmem_cond_try_extend (&initmem_reserved);
	} else {
		initmem_cond_try_extend (&initmem_usable_ram);
		initmem_append (&initmem_usable_ram, &range);
	}
}

/**
 * Add a memory range to the reserved list.
 */
__INIT_TEXT
void
initmem_reserve (unsigned long start, unsigned long size,
	initmem_flags_t flags)
{
	validate (flags);

	struct initmem_range range = {
		.start = start,
		.end = start + size,
		.flags = flags
	};

	range.flags &= ~INITMEM_RESERVE;
	initmem_append (&initmem_reserved, &range);
	initmem_cond_try_extend (&initmem_reserved);
}

/**
 * Iterate over memory regions which are free (i.e. regions in initmem_usable
 * that are not INITMEM_DONT_ALLOCATE_MASK and which don't overlap with
 * initmem_reserved). Returns true for each new entry, false when finished.
 *
 * 'it_a' and 'it_b' are pointers to iterator variables that should be
 * initialized to zero, then kept as is.
 */
__INIT_TEXT
int
initmem_next_free (struct initmem_range *out,
	unsigned long *it_a, unsigned long *it_b)
{
	/* strategy: a is usable_ram, b-1 is an entry to whose right we look */

	unsigned long a = *it_a, b = *it_b;
	for (; a < initmem_usable_ram.num_entries; a++) {
		struct initmem_range *e = &initmem_usable_ram.entries[a];
		if (e->flags & INITMEM_DONT_ALLOCATE_MASK)
			continue;

		unsigned long start = e->start;
		unsigned long end = e->end;
		if (!initmem_reserved.num_entries) {
			/* very simple case: no reserved entries */
			out->start = start;
			out->end = end;
			out->flags = e->flags;
			*it_a = a + 1;
			return true;
		}

		for (; b <= initmem_reserved.num_entries; b++) {
			unsigned long prev_end = 0;
			if (b)
				prev_end = initmem_reserved.entries[b - 1].end;

			unsigned long next_start = -1UL;
			if (b < initmem_reserved.num_entries)
				next_start = initmem_reserved.entries[b].start;

			if (next_start <= start)
				continue;

			if (prev_end >= end) {
				/* no more non-reserved holes for this entry */
				break;
			}

			unsigned long hstart = start, hend = end;
			if (prev_end > hstart)	hstart = prev_end;
			if (next_start < hend)	hend = next_start;

			out->start = hstart;
			out->end = hend;
			out->flags = e->flags;

			*it_a = a;
			*it_b = b + 1;
			return true;
		}

		/* don't miss an entry from advancing b too far*/
		if (b)	b--;
	}

	return false;
}

/**
 * Align the region boundaries from initmem_next_free to 'align'.
 */
int
initmem_next_free_aligned (struct initmem_range *out,
	unsigned long *it_a, unsigned long *it_b, unsigned long align)
{
	while (initmem_next_free (out, it_a, it_b)) {
		out->start = ALIGN_UP(out->start, align);
		out->end = ALIGN_DOWN(out->end, align);
		if (out->start < out->end)
			return true;
	}

	return false;
}

/**
 * Allocate physical memory within the given range.
 */
__INIT_TEXT
unsigned long
initmem_alloc_phys_range (unsigned long size, unsigned long align,
	unsigned long start, unsigned long end)
{
	unsigned long a = 0, b = 0;
	unsigned long found = 0;
	unsigned long asize = ALIGN_UP(size, align);
	struct initmem_range range;

	start = ALIGN_UP(start, align);
	end = ALIGN_DOWN(end, align);

	/* Be conservative. */
	unsigned long pgalign = max(unsigned long, align, PAGE_SIZE);

	/**
	 * We rely on the fact that initmem_next_free_align returns ranges
	 * with aligned ends.
	 */
	while (initmem_next_free_aligned (&range, &a, &b, pgalign)) {
		if (start > range.start)	range.start = start;
		if (end < range.end)		range.end = end;

		if (range.start >= range.end)
			continue;

		if (range.end - range.start >= asize) {
			/* NB: range.end is aligned */
			found = range.end - asize;

#ifdef BOTTOMUP_ALLOCATION_THRESHOLD
			/**
			 * We have hit the threshold for when we should switch
			 * over to bottom-up allocation. Architectures like x86
			 * depend on initmem_alloc_phys_range for initialization
			 * and mapping of memory, usually by calling us with
			 * changing 'end's. A bottom-up allocation strategy will
			 * cause fragmentation in such cases.
			 */
			if (found >= BOTTOMUP_ALLOCATION_THRESHOLD) {
				found = BOTTOMUP_ALLOCATION_THRESHOLD;
				if (range.start > found)
					found = range.start;
				break;
			}
#endif /* BOTTOMUP_ALLOCATION_THRESHOLD */
		}
	}

	if (found)
		initmem_reserve (found, size, INITMEM_KERN_RESERVED);
	return found;
}

/**
 * Allocate virtual memory within the given range.
 */
__INIT_TEXT
void *
initmem_alloc_virt_range (unsigned long size, unsigned long align,
	unsigned long start, unsigned long end)
{
	void *p = NULL;
	unsigned long addr = initmem_alloc_phys_range (size, align, start, end);
	if (addr)
		p = (void *) phys_to_virt (addr);
	return p;
}

__INIT_TEXT
unsigned long
initmem_alloc_phys (unsigned long size, unsigned long align)
{
	return initmem_alloc_phys_range (size, align,
		min_mapped_addr, max_mapped_addr);
}

__INIT_TEXT
void *
initmem_alloc_virt (unsigned long size, unsigned long align)
{
	return initmem_alloc_virt_range (size, align,
		min_mapped_addr, max_mapped_addr);
}

/**
 * Free a region of memory that was allocated using an initmem interface or
 * reserved using initmem_reserve or INITMEM_RESERVE. Uses physical addresses.
 */
__INIT_TEXT
void
initmem_free_phys (unsigned long start, unsigned long size)
{
	initmem_delete (&initmem_reserved, start, size);
	initmem_cond_try_extend (&initmem_reserved);
}

/**
 * Same as initmem_free_phys but uses virtual addresses.
 */
__INIT_TEXT
void
initmem_free_virt (void *start, unsigned long size)
{
	initmem_free_phys (virt_to_phys (start), size);
}
