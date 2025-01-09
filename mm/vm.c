/**
 * Virtual memory subsystem - the "heart" of the memory manager
 * Copyright (C) 2025-present  dbstream
 *
 * This file is responsible for two main things:
 * - managing the page tables
 * - handling page faults
 */
#include <davix/align.h>
#include <davix/pagefault.h>
#include <davix/panic.h>
#include <davix/mm.h>
#include <davix/page.h>
#include "internal.h"

static bool
is_present_pgtable (int level, pte_t pte)
{
	return pte_is_present (level, pte) && pte_is_pgtable (level, pte);
}

static void
__vm_clear_pte (struct tlb *tlb, pgtable_t *entry, int level, unsigned long addr)
{
	unsigned long entry_size = 1UL << pgtable_shift (level);

	pte_t val = __pte_clear (entry);
	if (!pte_is_present (level, val))
		return;

	if (!pte_is_pgtable (level, val)) {
		if (tlb)
			tlb_flush_range (tlb, addr, addr + entry_size);
		return;
	}

	pgtable_t *table = pte_pgtable (level, val);
	entry_size = 1UL << pgtable_shift (--level);
	unsigned long addr_old = addr;
	for (int i = pgtable_entries (level); i; i--) {
		entry = table + pgtable_addr_index (addr, level);
		__vm_clear_pte (tlb, entry, level, addr);
		addr += entry_size;
	}

	if (tlb)
		tlb_flush_pgtable (tlb, table, level + 1, addr_old);
	else
		free_page_table (level + 1, table);
}

static void
__vm_clear_range (struct tlb *tlb, pgtable_t *table, int level,
		unsigned long addr, unsigned long offset, unsigned long size)
{
	unsigned long entry_size = 1UL << pgtable_shift (level);

	while (size) {
		pgtable_t *entry = table + pgtable_addr_index (addr + offset, level);
		unsigned long next = ALIGN_UP (offset + 1UL, entry_size);
		if (offset + size && offset + size < next)
			next = offset + size;
		else if (!(offset & (entry_size - 1))) {
			__vm_clear_pte (tlb, entry, level, addr + offset);
			size -= next - offset;
			offset = next;
			continue;
		}

		pte_t val = __pte_read (entry);
		if (is_present_pgtable (level, val)) {
			entry = pte_pgtable (level, val);
			__vm_clear_range (tlb, entry, level - 1,
					addr + (offset & ~(entry_size - 1)),
					offset & (entry_size - 1), next - offset);
		}

		size -= next - offset;
		offset = next;
	}
}

/**
 * Clear page tables and PTEs in the range [start; end).
 */
void
vm_free_pgtable_range (struct vm_pgtable_op *op,
		unsigned long start, unsigned long end)
{
	__vm_clear_range (op->tlb, op->root_table, max_pgtable_level,
			0, start, end - start);
}

/**
 * Free @table and any child page table.
 */
void
vm_free_pgtable (struct process_mm *mm)
{
	__vm_clear_range (NULL, get_pgtable (mm->pgtable_handle),
			max_pgtable_level, 0, USER_MMAP_LOW, USER_MMAP_HIGH);
	free_user_page_table (mm->pgtable_handle);
}

void
vm_cancel_offline_pgtable_op (struct vm_pgtable_op *op)
{
	__vm_clear_range (NULL, op->root_table, max_pgtable_level,
			0, USER_MMAP_LOW, USER_MMAP_HIGH);
	free_page_table (max_pgtable_level, op->root_table);
}

static void
replace_pgtable_partial (pgtable_t *target, pgtable_t *source, int level,
		unsigned long start, unsigned long end, struct tlb *tlb);

static void
__vm_clear_pte_val (struct tlb *tlb, pte_t val, int level, unsigned long addr)
{
	pgtable_t fake;
	__pte_install_always (&fake, val);

	__vm_clear_pte (tlb, &fake, level, addr);
}

static void
copy_one_entry (pgtable_t *target_entry, pgtable_t *source_entry, int level,
		unsigned long start, unsigned long end, struct tlb *tlb)
{
	unsigned long entry_size = 1UL << pgtable_shift (level);

	pte_t source_pte = __pte_read (source_entry);
	pte_t target_pte = __pte_read (target_entry);

	/* If unaligned start or next, this is a partial overwrite.  */
	bool is_partial = (start & (entry_size - 1)) || (end & (entry_size - 1));

	/* If we are copying a full entry, simply put it in place directly.  */
	if (!is_partial) {
		__pte_install_always (target_entry, source_pte);
		if (pte_is_present (level, target_pte))
			__vm_clear_pte_val (tlb, target_pte, level, start);
		return;
	}

	/* If there was nothing previously mapped, install source directly.  */
	if (!is_present_pgtable (level, target_pte)) {
		__pte_install_always (target_entry, source_pte);
		return;
	}

	/* If source is null, __vm_clear_range instead.  */
	pgtable_t *target = pte_pgtable (level, target_pte);
	if (!is_present_pgtable (level, source_pte)) {
		__vm_clear_range (tlb, target, level - 1,
				start & ~(entry_size - 1),
				start & (entry_size - 1), end - start);
		return;
	}

	pgtable_t *source = pte_pgtable (level, source_pte);
	replace_pgtable_partial (target, source, level - 1, start, end, tlb);
}

/**
 * Copy PTEs from @source to @target.
 */
static void
replace_pgtable_partial (pgtable_t *target, pgtable_t *source, int level,
		unsigned long start, unsigned long end, struct tlb *tlb)
{
	unsigned long entry_size = 1UL << pgtable_shift (level);
	do {
		unsigned long next = ALIGN_UP (start + 1UL, entry_size);

		if (end && end < next)
			next = end;

		pgtable_t *target_entry = target + pgtable_addr_index (start, level);
		pgtable_t *source_entry = source + pgtable_addr_index (start, level);

		copy_one_entry (target_entry, source_entry, level, start, next, tlb);
		start = next;
	} while (start != end);
}

/**
 * Copy any PTEs in the range [@start; @end) from @op to @mm->pgtable.
 */
void
vm_apply_offline_pgtable_op (struct vm_pgtable_op *op,
		struct process_mm *mm, struct tlb *tlb,
		unsigned long start, unsigned long end)
{
	pgtable_t *target = get_pgtable (mm->pgtable_handle);
	replace_pgtable_partial (target, op->root_table, max_pgtable_level,
			start, end, tlb);
}

static pgtable_t *
get_pgtable_entry (struct vm_pgtable_op *op, unsigned long addr)
{
	int current = max_pgtable_level;
	if (!op->root_table)
		op->root_table = alloc_page_table (current);
	if (!op->root_table)
		return NULL;

	pgtable_t *table = op->root_table + pgtable_addr_index (addr, current);
	while (current > 1) {
		pte_t entry = __pte_read (table);
		if (pte_is_present (current, entry))
			table = pte_pgtable (current, entry);
		else {
			pgtable_t *new_table = alloc_page_table (current - 1);
			if (!new_table)
				return NULL;

			table = __pgtable_install (table, &new_table, false);
			if (new_table)
				free_page_table (current - 1, new_table);
		}

		table += pgtable_addr_index (addr, --current);
	}

	return table;
}

static void
anon_unmap_vma_range (struct vm_pgtable_op *op, struct vm_area *vma,
		unsigned long start, unsigned long end)
{
	(void) vma;

	do {
		pgtable_t *entry = get_pgtable_entry (op, start);
		if (entry) {
			pte_t pte = __pte_clear (entry);
			if (pte_is_present (1, pte) && op->tlb)
				tlb_flush_range (op->tlb, start, start + PAGE_SIZE);
			if (pte_is_nonnull (1, pte))
				free_page (phys_to_pfn_entry (pte_addr (1, pte)),
						ALLOC_USER);
		}
		start += PAGE_SIZE;
	} while (start != end);
}

static errno_t
anon_map_vma_range (struct vm_pgtable_op *op, struct vm_area *vma,
		unsigned long start, unsigned long end)
{
	pte_flags_t flags = make_pte_flags (
			vma->vm_flags & VM_READ,
			vma->vm_flags & VM_WRITE,
			vma->vm_flags & VM_EXEC);

	unsigned long i = start;
	do {
		pgtable_t *entry = get_pgtable_entry (op, i);
		if (!entry) {
			if (i != start)
				anon_unmap_vma_range (op, vma, start, i);
			return ENOMEM;
		}

		struct pfn_entry *page = alloc_page (ALLOC_USER);
		if (!page) {
			if (i != start)
				anon_unmap_vma_range (op, vma, start, i);
			return ENOMEM;
		}

		__pte_install_always (entry, make_user_pte (1,
				pfn_entry_to_phys (page), flags));

		i += PAGE_SIZE;
	} while (i != end);
	return ESUCCESS;
}

static errno_t
anon_mprotect_vma_range (struct vm_pgtable_op *op, struct vm_area *vma,
		unsigned long start, unsigned long end)
{
	pte_flags_t flags = make_pte_flags (
			vma->vm_flags & VM_READ,
			vma->vm_flags & VM_WRITE,
			vma->vm_flags & VM_EXEC);

	unsigned long i = start;
	do {
		pgtable_t *entry = get_pgtable_entry (op, i);
		if (!entry)
			panic ("anon_mprotect_vma_range: no page was mapped!");

		pte_t old = __pte_read (entry);
		if (!pte_is_nonnull (1, old))
			panic ("anon_mprotect_vma_range: no page was allocated!");

		pte_t pte = make_user_pte (1, pte_addr (1, old), flags);
		__pte_install_always (entry, pte);
		if (pte_update_needs_flush (old, pte) && op->tlb)
			tlb_flush_range (op->tlb, i, i + PAGE_SIZE);

		i += PAGE_SIZE;
	} while (i != end);
	return ESUCCESS;
}

const struct vm_area_ops anon_vma_ops = {
	.map_vma_range = anon_map_vma_range,
	.unmap_vma_range = anon_unmap_vma_range,
	.mprotect_vma_range = anon_mprotect_vma_range
};

pagefault_status_t
vm_handle_pagefault (struct pagefault_info *info)
{
	(void) info;
	return FAULT_SIGSEGV;
}
