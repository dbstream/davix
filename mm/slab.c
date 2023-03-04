/* SPDX-License-Identifier: MIT */
#include <davix/slab.h>
#include <davix/mm.h>
#include <davix/page_alloc.h>
#include <davix/printk.h>
#include <davix/list.h>
#include <asm/page.h>

struct free_slab_object {
	struct free_slab_object *next;
};

void slab_init(struct slab_alloc *alloc, unsigned long ob_size,
	alloc_flags_t flags)
{
	/* can't be bothered to handle ``ob_size < 0`` */
	ob_size = align_up(ob_size ?: 8, sizeof(unsigned long));

	list_init(&alloc->slab_list);
	alloc->total_slabs = 0;
	alloc->total_obs = 0;
	alloc->free_obs = 0;

	unsigned order = 0;
	while((PAGE_SIZE << order) < ob_size * 4)
		order++;

	alloc->slab_order = order;
	alloc->obs_per_slab = (PAGE_SIZE << order) / ob_size;
	alloc->ob_size = ob_size;
	alloc->alloc_flags = flags;
	spinlock_init(&alloc->lock);
}

static void free_slab(struct page *page, struct slab_alloc *alloc)
{
	clear_page_bit(page, PAGE_SLAB_HEAD);
	free_page(page, alloc->slab_order);
}

static struct page *create_new_slab(struct slab_alloc *alloc)
{
	struct page *page = alloc_page(alloc->alloc_flags, alloc->slab_order);
	if(!page)
		return NULL;

	set_page_bit(page, PAGE_SLAB_HEAD);

	for(unsigned long i = 1; i < (1 << alloc->slab_order); i++)
		page[i].slab.head = page;

	page->slab.allocator = alloc;
	page->slab.ob_count = alloc->obs_per_slab;
	page->slab.ob_head = NULL;
	unsigned long virt = page_to_virt(page);
	for(unsigned long i = 0; i < alloc->obs_per_slab; i++) {
		struct free_slab_object *ob =
			(struct free_slab_object *) (virt + i * alloc->ob_size);
		ob->next = page->slab.ob_head;
		page->slab.ob_head = ob;
	}

	return page;
}

void slab_destroy(struct slab_alloc *alloc)
{
	if(alloc->total_obs != alloc->free_obs)
		warn("slab: destroying a slab with in-flight objects.\n");

	struct page *slab, *tmp;
	list_for_each_safe(slab, &alloc->slab_list, slab.slab_list, tmp) {
		list_del(&slab->slab.slab_list);
		free_slab(slab, alloc);
	}
}

/*
 * Creates a slab and inserts it to the allocator.
 * Returns the newly created slab.
 */
static inline struct page *insert_new_slab(struct slab_alloc *alloc)
must_hold(&alloc->lock)
{
	struct page *slab = create_new_slab(alloc);
	if(!slab)
		return NULL;

	list_add(&alloc->slab_list, &slab->slab.slab_list);
	alloc->total_slabs++;
	alloc->total_obs += alloc->obs_per_slab;
	alloc->free_obs += alloc->obs_per_slab;
	return slab;
}

void *slab_allocate(struct slab_alloc *alloc)
{
	struct free_slab_object *ret = NULL;
	int irqflag = spin_acquire_irq(&alloc->lock);
	struct page *slab;
	if(list_empty(&alloc->slab_list)) {
		/* no slabs available, create a new slab */
		slab = insert_new_slab(alloc);
		if(!slab)
			/* we couldn't create a new slab */
			goto out;
	} else {
		/* get the first slab */
		slab = container_of(alloc->slab_list.next,
			struct page, slab.slab_list);
	}

	ret = (struct free_slab_object *) slab->slab.ob_head;
	slab->slab.ob_head = ret->next;

	if(--(slab->slab.ob_count) == 0)
		list_del(&slab->slab.slab_list);

	alloc->free_obs--;

out:
	spin_release_irq(&alloc->lock, irqflag);
	return (void *) ret;
}

static struct page *slab_of(void *mem)
{
	struct page *page = virt_to_page(mem);
	if(!test_page_bit(page, PAGE_SLAB_HEAD))
		page = page->slab.head;
	return page;
}

void slab_free(void *mem)
{
	struct page *slab = slab_of(mem);
	struct slab_alloc *alloc = slab->slab.allocator;
	int irqflag = spin_acquire_irq(&alloc->lock);

	alloc->free_obs++;

	if(++(slab->slab.ob_count) == 1) {
		list_add(&alloc->slab_list, &slab->slab.slab_list);
	}

	if(slab->slab.ob_count == alloc->obs_per_slab) {
		list_del(&slab->slab.slab_list);
		alloc->free_obs -= alloc->obs_per_slab;
		alloc->total_obs -= alloc->obs_per_slab;
		alloc->total_slabs--;
		free_slab(slab, alloc);
	} else {
		struct free_slab_object *ob =
			(struct free_slab_object *) mem;
		ob->next = (struct free_slab_object *) slab->slab.ob_head;
		slab->slab.ob_head = (void *) ob;
	}

	spin_release_irq(&alloc->lock, irqflag);
}
