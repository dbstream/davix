/**
 * Memory manager internals - struct vm_pgtable_op
 * Copyright (C) 2025-present  dbstream
 */
#ifndef _MM_INTERNAL_H
#define _MM_INTERNAL_H 1

#include <davix/mm.h>
#include <asm/pgtable.h>
#include <asm/tlb.h>

/**
 * vm_pgtable_op allows to perform "fake" page table modification.  This is
 * useful for mmap(), which wants to ensure atomicity of MAP_FIXED and therefore
 * not overwrite old memory regions until we are sure we will succeed.  vm
 * provides functions for modifying page tables indirectly using vm_pgtable_op.
 */

struct vm_pgtable_op {
	pgtable_t *root_table;
	struct tlb *tlb;
};

static inline void
vm_init_offline_pgtable_op (struct vm_pgtable_op *op)
{
	op->root_table = NULL;
}

static inline void
vm_init_online_pgtable_op (struct vm_pgtable_op *op,
		struct process_mm *mm, struct tlb *tlb)
{
	op->root_table = get_pgtable (mm->pgtable_handle);
	op->tlb = tlb;
}

extern void
vm_cancel_offline_pgtable_op (struct vm_pgtable_op *op);

extern void
vm_apply_offline_pgtable_op (struct vm_pgtable_op *op,
		struct process_mm *mm, struct tlb *tlb,
		unsigned long start, unsigned long end);

/**
 * Free a range from the page tables.
 */
extern void
vm_free_pgtable_range (struct vm_pgtable_op *op,
		unsigned long start, unsigned long end);

/**
 * Free an entire page table.
 */
extern void
vm_free_pgtable (struct process_mm *mm);

#endif /** _MM_INTERNAL_H  */
