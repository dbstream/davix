/**
 * Process MM creation and management.
 * Copyright (C) 2025-present  dbstream
 */
#include <davix/mm.h>
#include <davix/panic.h>
#include <davix/printk.h>
#include <davix/slab.h>
#include <asm/pgtable.h>

static struct slab *proc_mm_slab;

static void
initialize_mm_slabs (void)
{
	proc_mm_slab = slab_create ("process_mm", sizeof (struct process_mm));
	if (!proc_mm_slab)
		panic ("Failed t ocreate process_mm slab!");
}

static inline struct process_mm *
alloc_process_mm (void)
{
	return slab_alloc (proc_mm_slab);
}

static inline void
free_process_mm (struct process_mm *mm)
{
	slab_free (mm);
}

/**
 * Destroy a process address space.
 */
void
__mmput (struct process_mm *mm)
{
	(void) mm;
	printk (PR_WARN "mm/process_mm: __mmput is unimplemented!\n");
}

/**
 * Create a new process address space.
 */
struct process_mm *
mmnew (void)
{
	struct process_mm *mm = alloc_process_mm ();
	if (!mm)
		return NULL;

	vma_tree_init (&mm->vma_tree);
	mm->mm_start = USER_MMAP_LOW;
	mm->mm_end = USER_MMAP_HIGH;
	mm->mmap_base = 0x400000UL;	/**  4MiB  */
	refcount_set (&mm->refcount, 1);
	mm->pgtable_handle = alloc_user_page_table ();
	if (!mm->pgtable_handle) {
		free_process_mm (mm);
		return NULL;
	}
	mutex_init (&mm->mm_mutex);
	return mm;
}

extern void
mmap_init (void);

/**
 * Inititalize the Davix memory manager.
 */
void
mm_init (void)
{
	initialize_mm_slabs ();
	mmap_init ();
}
