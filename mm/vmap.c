/**
 * Kernel virtual memory manager.
 * Copyright (C) 2024  dbstream
 */
#include <davix/align.h>
#include <davix/mutex.h>
#include <davix/page.h>
#include <davix/panic.h>
#include <davix/printk.h>
#include <davix/slab.h>
#include <davix/stddef.h>
#include <davix/string.h>
#include <davix/vmap.h>
#include <asm/mm_init.h>
#include <asm/page_defs.h>
#include <asm/pgtable.h>
#include <asm/tlb.h>

static struct slab *vmap_slab;

static struct vma_tree vmap_tree;

/**
 * vmap_lock
 *
 * The vmap_lock must be held during any operation that reads or modifies the
 * vmap_tree.
 */
static struct mutex vmap_lock;

static bool has_initialised_vmap = false;

static void
vmap_insert_default_area (unsigned long first, unsigned long last,
	const char *purpose)
{
	struct vmap_area *area = slab_alloc (vmap_slab);
	if (!area)
		panic ("vmap_init: failed to insert default area \"%s\"\n", purpose);

	area->vma_node.first = first;
	area->vma_node.last = last;
	strncpy (area->purpose, purpose, sizeof (area->purpose));
	area->purpose[sizeof (area->purpose) - 1] = 0;

	vma_tree_insert (&vmap_tree, &area->vma_node);
}

void
vmap_init (void)
{
	mutex_init (&vmap_lock);
	vma_tree_init (&vmap_tree);

	vmap_slab = slab_create ("vmap_area", sizeof (struct vmap_area));
	if (!vmap_slab)
		panic ("vmap_init: Failed to create the vmap slab!");

	has_initialised_vmap = true;

	arch_insert_vmap_areas (vmap_insert_default_area);
	vmap_dump ();
}

void
vmap_dump (void)
{
	if (!has_initialised_vmap)
		return;

	printk ("vmap_dump:\n");

	mutex_lock (&vmap_lock);

	struct vma_tree_iterator it;
	bool flag = vma_tree_first (&it, &vmap_tree);
	while (flag) {
		struct vma_node *node = vma_node (&it);
		struct vmap_area *area = struct_parent (struct vmap_area, vma_node, node);
		printk ("... 0x%lx - 0x%lx    %s\n",
			area->vma_node.first, area->vma_node.last, area->purpose);

		flag = vma_tree_next (&it);
	}

	mutex_unlock (&vmap_lock);
}

static void
__clear_vmap_range (struct tlb *, pgtable_t *, int,
	unsigned long, unsigned long, unsigned long);

static void
__clear_vmap_entry (struct tlb *tlb, pgtable_t *entry, int level, unsigned long vaddr)
{
	unsigned long entry_size = 1UL << pgtable_shift (level);

	pte_t val = __pte_clear (entry);
	if (!pte_is_present (level, val))
		return;

	if (!pte_is_pgtable (level, val)) {
		tlb_flush_range (tlb, vaddr, vaddr + entry_size);
		return;
	}

	pgtable_t *table = pte_pgtable (level, val);
	int entries = pgtable_entries (level);
	entry_size = 1UL << pgtable_shift (--level);
	for (int i = 0; i < entries; i++) {
		entry = table + pgtable_addr_index (vaddr, level);
		__clear_vmap_entry (tlb, entry, level, vaddr);
		vaddr += entry_size;
	}

	if (level < max_vunmap_pgtable_level) {
		printk ("vmap: freed page table at %d\n", level + 1);
		tlb_flush_pgtable (tlb, table, level + 1, vaddr);
	}
}

static void
__clear_vmap_range (struct tlb *tlb, pgtable_t *table, int level,
	unsigned long vaddr, unsigned long offset, unsigned long size)
{
	unsigned long entry_size = 1UL << pgtable_shift (level);

	while (size) {
		pgtable_t *entry = table + pgtable_addr_index (vaddr + offset, level);
		unsigned long next = ALIGN_UP (offset + 1UL, entry_size);
		if (offset + size && offset + size < next)
			next = offset + size;
		else if (offset == ALIGN_DOWN (offset, entry_size)) {
			__clear_vmap_entry (tlb, entry, level, vaddr + offset);
			size -= next - offset;
			offset = next;
		}

		pte_t val = __pte_read (entry);
		if (pte_is_present (level, val) && pte_is_pgtable (level, val)) {
			entry = pte_pgtable (level, val);
			__clear_vmap_range (tlb, entry, level - 1,
				vaddr + (offset & ~(entry_size - 1)), offset & (entry_size - 1), next - offset);
		}

		size -= next - offset;
		offset = next;
	}
}

/**
 * Remove mappings for @area.
 */
static void
clear_vmap_range (struct vmap_area *area)
{
	unsigned long align = 1UL << pgtable_shift (max_vunmap_pgtable_level);

	// NB: unsigned overflow can make end==0
	unsigned long start = ALIGN_DOWN (area->vma_node.first, align);
	unsigned long end = ALIGN_UP (area->vma_node.last + 1UL, align);

	struct vma_tree_iterator it;
	vma_tree_make_iterator (&it, &vmap_tree, &area->vma_node);
	if (vma_tree_prev (&it)) {
		start = max (unsigned long, start, vma_node (&it)->last + 1UL);
		vma_tree_make_iterator (&it, &vmap_tree, &area->vma_node);
	}
	if (vma_tree_next (&it)) {
		if (end)
			end = min (unsigned long, end, vma_node (&it)->first);
		else
			end = vma_node (&it)->first;
	}

	struct tlb tlb;
	tlbflush_init (&tlb, NULL);
	__clear_vmap_range (&tlb, get_vmap_page_table (),
		max_pgtable_level, 0, start, end - start);
	tlb_flush (&tlb);
}

struct vmap_area *
find_vmap_area (void *ptr)
{
	unsigned long addr = (unsigned long) ptr;
	struct vma_tree_iterator it;

	mutex_lock (&vmap_lock);
	bool result = vma_tree_find (&it, &vmap_tree, addr);
	mutex_unlock (&vmap_lock);

	if (result)
		return struct_parent (struct vmap_area, vma_node, vma_node (&it));
	return NULL;
}

struct vmap_area *
allocate_vmap_area (const char *name,
	unsigned long size, unsigned long align,
	unsigned long low, unsigned long high)
{
	/**
	 * Make sure that area->vma_node.last + 1 never overflows.
	 */
	high = min (unsigned long, high, -2UL);

	size = ALIGN_UP (size, PAGE_SIZE);
	align = max (unsigned long, align, PAGE_SIZE);
	if (!size || !align)
		return NULL;

	struct vmap_area *area = slab_alloc (vmap_slab);
	if (!area)
		return NULL;

	strncpy (area->purpose, name, sizeof (area->purpose));
	area->purpose[sizeof (area->purpose) - 1] = 0;

	mutex_lock (&vmap_lock);

	unsigned long addr;
	if (vma_tree_find_free_bottomup (&addr, &vmap_tree, size, align, low, high)) {
		area->vma_node.first = addr;
		area->vma_node.last = addr + size - 1;
		vma_tree_insert (&vmap_tree, &area->vma_node);
	} else {
		slab_free (area);
		area = NULL;
	}

	mutex_unlock (&vmap_lock);
	return area;
}

void
free_vmap_area (struct vmap_area *area)
{
	mutex_lock (&vmap_lock);
	clear_vmap_range (area);
	vma_tree_remove (&vmap_tree, &area->vma_node);
	mutex_unlock (&vmap_lock);

	slab_free (area);
}

static pgtable_t *
get_pgtable_entry (unsigned long addr, int level)
{
	int current = max_pgtable_level;
	pgtable_t *pgtable = get_vmap_page_table ();
	pgtable += pgtable_addr_index (addr, current);

	while (current > level) {
		pte_t entry = __pte_read (pgtable);
		if (pte_is_present (current, entry))
			pgtable = pte_pgtable (current, entry);
		else {
			pgtable_t *new_table = alloc_page_table (current - 1);
			if (!new_table)
				return NULL;

			pgtable = __pgtable_install (pgtable, &new_table, true);
			if (new_table)
				free_page_table (current - 1, new_table);
		}

		pgtable += pgtable_addr_index (addr, --current);
	}

	return pgtable;
}

static bool
map_phys_range (unsigned long virt_addr,
	unsigned long addr, unsigned long size, pte_flags_t prot)
{
	for (; size; addr += PAGE_SIZE, virt_addr += PAGE_SIZE, size -= PAGE_SIZE) {
		pgtable_t *entry = get_pgtable_entry (virt_addr, 1);
		if (!entry)
			return false;

		__pte_install_always (entry, make_pte (1, addr, prot));
	}

	return true;
}

static bool
map_pages_range (unsigned long virt_addr,
	struct pfn_entry **pages, unsigned long size, pte_flags_t prot)
{
	for (; size; pages++, virt_addr += PAGE_SIZE, size -= PAGE_SIZE) {
		pgtable_t *entry = get_pgtable_entry (virt_addr, 1);
		if (!entry)
			return false;

		unsigned long phys = pfn_entry_to_phys (*pages);
		__pte_install_always (entry, make_pte (1, phys, prot));
	}

	return true;
}

void *
vmap_phys_explicit (const char *name,
	unsigned long addr, unsigned long size, pte_flags_t prot,
	unsigned long align, unsigned long low, unsigned long high)
{
	unsigned long offset = addr & (PAGE_SIZE - 1);

	size = ALIGN_UP (addr + size, PAGE_SIZE);
	addr = ALIGN_DOWN (addr, PAGE_SIZE);
	size -= addr;

	struct vmap_area *area = allocate_vmap_area (name, size, align, low, high);
	if (!area)
		return NULL;

	if (map_phys_range (area->vma_node.first, addr, size, prot))
		return (void *) (area->vma_node.first + offset);

	free_vmap_area (area);
	return NULL;
}

void *
vmap_phys (const char *name,
	unsigned long addr, unsigned long size, pte_flags_t prot)
{
	return vmap_phys_explicit (name, addr, size, prot,
		PAGE_SIZE, KERNEL_VMAP_LOW, KERNEL_VMAP_HIGH);
}

void *
vmap_pages (const char *name,
	struct pfn_entry **pages, unsigned long nr_pages, pte_flags_t prot)
{
	unsigned long size = (2 + nr_pages) * PAGE_SIZE;
	struct vmap_area *area = allocate_vmap_area (name, size, PAGE_SIZE,
		KERNEL_VMAP_LOW, KERNEL_VMAP_HIGH);

	if (!area)
		return NULL;

	area->u.pages = pages;

	if (map_pages_range (area->vma_node.first + PAGE_SIZE, pages, nr_pages * PAGE_SIZE, prot))
		return (void *) (area->vma_node.first + PAGE_SIZE);

	free_vmap_area (area);
	return NULL;
}

void
vunmap (void *mem)
{
	struct vmap_area *area = find_vmap_area (mem);
	if (area)
		free_vmap_area (area);
}
