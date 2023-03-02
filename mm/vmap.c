/* SPDX-License-Identifier: MIT */
#include <davix/avltree.h>
#include <davix/errno.h>
#include <davix/mm.h>
#include <davix/panic.h>
#include <davix/page_table.h>
#include <davix/printk.h>
#include <davix/list.h>
#include <davix/kmalloc.h>
#include <asm/page.h>

/*
 * A ``struct mm`` used for purely kernel-mode tasks and to manage page tables
 * for kernel mappings.
 */
struct mm kernelmode_mm = {
	.mm_lock = SPINLOCK_INIT(kernelmode_mm.mm_lock),
	.page_tables = NULL,
	.vma_list = LIST_INIT(kernelmode_mm.vma_list),
	.vma_tree = AVLTREE_INIT(kernelmode_mm.vma_tree),
	.refcnt = 1
};

static inline void insert_into_kernelmode_mm(struct vm_area *area)
{
	struct avlnode *parent = NULL;
	struct avlnode *node = kernelmode_mm.vma_tree.root;
	struct avlnode **link = &kernelmode_mm.vma_tree.root;
	struct list *prev = &kernelmode_mm.vma_list;
	while(node) {
		parent = node;
		struct vm_area *node_vma = vma_of(node);
		if(area->start > node_vma->start) {
			prev = &node_vma->list;
			link = &node->right;
			node = node->right;
		} else {
			prev = node_vma->list.prev;
			link = &node->left;
			node = node->left;
		}
	}
	list_add(prev, &area->list);
	area->avl.left = NULL;
	area->avl.right = NULL;
	vma_avl_update_callback(&area->avl);
	__avl_insert(&area->avl, parent, link,
		&kernelmode_mm.vma_tree, vma_avl_update_callback);
}

static inline void delete_from_kernelmode_mm(struct vm_area *area)
{
	struct vm_area *next_vma = vma_next(&kernelmode_mm, area);
	list_del(&area->list);
	__avl_delete(&area->avl, &kernelmode_mm.vma_tree, vma_avl_update_callback);
	if(next_vma)
		__avl_update_node_and_parents(&next_vma->avl,
			vma_avl_update_callback);
}

struct vm_area *alloc_vmap_area(unsigned long size, unsigned shift)
{
	if(!has_hugepage_size(shift)) {
		warn("vmap: An allocation attempt was made with unsupported hugepage size %u. Caller: %p\n",
			shift, RET_ADDR);
		return NULL;
	}

	size = align_up(size, PAGE_SIZE << shift);
	if(!size) {
		return NULL;
	}

	struct vm_area *area = kmalloc(sizeof(struct vm_area));
	if(!area)
		return NULL;

	spinlock_init(&area->vma_lock);
	area->mm = &kernelmode_mm;
	area->hugepage_shift = shift;

	int irqflag = interrupts_enabled();
	disable_interrupts();
	mm_lock(&kernelmode_mm);

	struct get_unmapped_area_info info = {
		.align = PAGE_SIZE << shift,
		.start = VMAP_START,
		.end = VMAP_END,
		.topdown = 0
	};

	unsigned long start = get_unmapped_area(&kernelmode_mm, size, info);
	if(IS_ERROR(start)) {
		mm_unlock(&kernelmode_mm);
		if(irqflag)
			enable_interrupts();
		kfree(area);
		return NULL;
	}

	area->start = start;
	area->end = start + size;

	struct pgop pgop;
	pgop_begin(&pgop, &kernelmode_mm, shift);
	int err = alloc_ptes(&pgop, area->start, area->end);
	pgop_end(&pgop);

	if(err) {
		mm_unlock(&kernelmode_mm);
		if(irqflag)
			enable_interrupts();
		kfree(area);
		return NULL;
	}

	insert_into_kernelmode_mm(area);
	mm_unlock(&kernelmode_mm);
	if(irqflag)
		enable_interrupts();

	return area;
}

void free_vmap_area(struct vm_area *area)
{
	int irqflag = interrupts_enabled();
	disable_interrupts();
	mm_lock(&kernelmode_mm);

	struct pgop pgop;
	pgop_begin(&pgop, &kernelmode_mm, area->hugepage_shift);
	for(unsigned long i = area->start; i < area->end;
		i += PAGE_SIZE << pgop.shift)
			clear_pte(&pgop, i);
	free_ptes(&pgop, area->start, area->end);
	pgop_end(&pgop);

	delete_from_kernelmode_mm(area);
	mm_unlock(&kernelmode_mm);
	if(irqflag)
		enable_interrupts();
}

static void _dump_kernel_vmap_areas(void)
{
	info("Kernel vmap() areas:\n");
	struct vm_area *vma;
	list_for_each(vma, &kernelmode_mm.vma_list, list) {
		info("        [mem %p - %p] hugepage shift %u\n",
			vma->start, vma->end - 1, vma->hugepage_shift);
	}
}

void dump_kernel_vmap_areas(void)
{
	int irqflag = interrupts_enabled();
	disable_interrupts();
	mm_lock(&kernelmode_mm);

	_dump_kernel_vmap_areas();

	mm_unlock(&kernelmode_mm);
	if(irqflag)
		enable_interrupts();
}

void *vmap(unsigned long phys_addr, unsigned long len,
	pgmode_t mode, pgcachemode_t cachemode)
{

	if(len & (PAGE_SIZE - 1) || phys_addr & (PAGE_SIZE - 1)) {
		warn("vmap(): trying to map unaligned memory [%p - %p]. caller: %p\n",
			phys_addr, phys_addr + len - 1, RET_ADDR);
		return NULL;
	}

	int irqflag = interrupts_enabled();
	disable_interrupts();

	unsigned shift = arch_hugepage_size(phys_addr, len);
	struct vm_area *vma = alloc_vmap_area(len, shift);

	if(!vma)
		return NULL;

	struct pgop pgop;
	pgop_begin(&pgop, &kernelmode_mm, shift);
	for(unsigned long i = 0; i < len; i += PAGE_SIZE << shift)
		set_pte(&pgop, i + vma->start, i + phys_addr, cachemode, mode);
	pgop_end(&pgop);

	if(irqflag)
		enable_interrupts();

	return (void *) vma->start;
}

void vunmap(void *mem)
{
	int irqflag = interrupts_enabled();
	disable_interrupts();
	mm_lock(&kernelmode_mm);

	struct vm_area *vma = NULL;
	struct avlnode *node = kernelmode_mm.vma_tree.root;
	while(node) {
		vma = vma_of(node);
		if(vma->start == (unsigned long) mem)
			goto has_vma;

		node = vma->start < (unsigned long) mem
			? node->right
			: node->left;
	}
	_dump_kernel_vmap_areas();
	panic("vunmap(): Trying to unmap non-vmap() address %p\n", mem);

has_vma:;
	struct pgop pgop;
	pgop_begin(&pgop, &kernelmode_mm, vma->hugepage_shift);
	for(unsigned long i = vma->start; i < vma->end;
		i += PAGE_SIZE << vma->hugepage_shift)
			clear_pte(&pgop, i);
	free_ptes(&pgop, vma->start, vma->end);
	pgop_end(&pgop);

	delete_from_kernelmode_mm(vma);
	mm_unlock(&kernelmode_mm);
	if(irqflag)
		enable_interrupts();
}
