/**
 * Slab allocation and operations on slab pages.
 * Copyright (C) 2024  dbstream
 */
#include <davix/align.h>
#include <davix/cpulocal.h>
#include <davix/cpuset.h>
#include <davix/initmem.h>
#include <davix/page.h>
#include <davix/printk.h>
#include <davix/slab.h>
#include <davix/string.h>
#include <asm/cpulocal.h>

static __CPULOCAL struct kmalloc_stat {
	unsigned long nr_quickbin_hit;
	unsigned long nr_quickbin_miss;
	unsigned long nr_alloc_page;
	unsigned long nr_failed_alloc;
	unsigned long nr_quickbin_free;
	unsigned long nr_expensive_free;
	unsigned long nr_free_page;
} pcpu_slab_stat;

#define inc_slab_stat(name) this_cpu_write (&pcpu_slab_stat.name,	\
		this_cpu_read (&pcpu_slab_stat.name) + 1)

#define inc_nr_quickbin_hit() inc_slab_stat(nr_quickbin_hit)
#define inc_nr_quickbin_miss() inc_slab_stat(nr_quickbin_miss)
#define inc_nr_alloc_page() inc_slab_stat(nr_alloc_page)
#define inc_nr_failed_alloc() inc_slab_stat(nr_failed_alloc)
#define inc_nr_quickbin_free() inc_slab_stat(nr_quickbin_free)
#define inc_nr_expensive_free() inc_slab_stat(nr_expensive_free)
#define inc_nr_free_page() inc_slab_stat(nr_free_page)

struct pcpu_quickbin {
	unsigned long num;
	void *ptrs[];
};

struct slab {
	spinlock_t lock;

	/**
	 * Per-CPU quickbins.
	 */
	unsigned long quickbin_len;
	struct pcpu_quickbin *quickbin;

	/**
	 * Full, partial, and empty pages.
	 *
	 * A full page is a page for which all objects are free. For partial
	 * pages, some objects are free, and for empty pages, no objects are free.
	 */
	unsigned long nr_full;
	unsigned long nr_partial;
	unsigned long nr_empty;

	struct list pg_full_list;
	struct list pg_partial_list;

	/**
	 * Per-slab statistics.
	 */
	unsigned long nr_obj_free;

	/**
	 * obj_size is the value passed to slab_create. real_obj_size is what
	 * is actually available for each allocation. objs_per_page is how many
	 * objects fit into one page.
	 */
	unsigned long obj_size;
	unsigned long real_obj_size;
	unsigned long objs_per_page;

	struct list slab_list;

	/**
	 * Symbolic name of this slab. Not null-terminated.
	 * NOTE: this is not a unique identifier for the slab.
	 */
	char name[16];
};

/**
 * Keep a list of all slabs for debugging purposes.
 */

static DEFINE_EMPTY_LIST (slab_list);
static spinlock_t slab_list_lock;

/**
 * New opaque slab handles are themselves allocated from a slab. Use a static
 * variable for this purpose.
 */

static struct slab static_slab;

static void
slab_dump_one (struct slab *slab)
{
	spin_lock (&slab->lock);
	unsigned long nr_full = slab->nr_full;
	unsigned long nr_partial = slab->nr_partial;
	unsigned long nr_empty = slab->nr_empty;
	unsigned long nr_obj_free = slab->nr_obj_free;
	spin_unlock (&slab->lock);

	printk (PR_INFO "  %16.16s %4lu %4lu %4lu  %4lu %4lu %4lu  %4lu %4lu\n",
		slab->name,
		slab->obj_size, slab->real_obj_size, slab->objs_per_page,
		nr_full, nr_partial, nr_empty,
		slab->objs_per_page * (nr_full + nr_partial + nr_empty),
		nr_obj_free);
}

static struct kmalloc_stat
collect_kmalloc_stat (void)
{
	struct kmalloc_stat stat = { 0, 0, 0, 0, 0, 0, 0 };
	for_each_present_cpu (cpu) {
		struct kmalloc_stat *p = that_cpu_ptr (&pcpu_slab_stat, cpu);
		stat.nr_quickbin_hit += p->nr_quickbin_hit;
		stat.nr_quickbin_miss += p->nr_quickbin_miss;
		stat.nr_alloc_page += p->nr_alloc_page;
		stat.nr_failed_alloc += p->nr_failed_alloc;
		stat.nr_quickbin_free += p->nr_quickbin_free;
		stat.nr_expensive_free += p->nr_expensive_free;
		stat.nr_free_page += p->nr_free_page;
	}
	return stat;
}

void
slab_dump (void)
{
	/**
	 * FIXME: when we introduce a way to destroy slabs, remember to protect
	 * this function using something like RCU. Currently it is safe, because
	 * we never delete anything from the slab_list.
	 */

	printk (PR_INFO "Slab allocators:\n");
	printk (PR_INFO "              name size/real/page  full/part/empt  totl/free\n");

	list_for_each (entry, &slab_list)
		slab_dump_one (list_item (struct slab, slab_list, entry));

	struct kmalloc_stat stat = collect_kmalloc_stat ();
	printk (PR_INFO "Slab statistics:\n");
	printk (PR_INFO "  nr_quickbin_hit    %lu\n", stat.nr_quickbin_hit);
	printk (PR_INFO "  nr_quickbin_miss   %lu\n", stat.nr_quickbin_miss);
	printk (PR_INFO "  nr_alloc_page      %lu\n", stat.nr_alloc_page);
	printk (PR_INFO "  nr_failed_alloc    %lu\n", stat.nr_failed_alloc);
	printk (PR_INFO "  nr_quickbin_free   %lu\n", stat.nr_quickbin_free);
	printk (PR_INFO "  nr_expensive_free  %lu\n", stat.nr_expensive_free);
	printk (PR_INFO "  nr_free_page       %lu\n", stat.nr_free_page);
}

static void
slab_init_new (struct slab *slab, const char *name, unsigned long obj_size)
{
	spinlock_init (&slab->lock);
	slab->nr_full = 0;
	slab->nr_partial = 0;
	slab->nr_empty = 0;
	list_init (&slab->pg_full_list);
	list_init (&slab->pg_partial_list);
	slab->nr_obj_free  = 0;
	slab->obj_size = obj_size;

	obj_size = sizeof (long) > obj_size ? sizeof (long) : obj_size;

	/* compute the index of the most significant bit of obj_size */
	int msb = 1;
	for (; obj_size >> (msb + 1); msb++);

	/**
	 * allow sizes like 8, 16, 24, 32, 48, 64, 96...
	 * allow those that are equal to 1 or 3 left-shifted by some amount
	 * disallow sizes like 6 and 12 (not aligned to sizeof (long))
	 */
	unsigned long real_size = ALIGN_UP (obj_size, 1UL << (msb - 1));
	real_size = ALIGN_UP (real_size, sizeof (long));

	slab->real_obj_size = real_size;
	slab->objs_per_page = PAGE_SIZE / real_size;

	slab->quickbin_len = min (unsigned long, 31, slab->objs_per_page - 1);
	unsigned long quickbin_size = offsetof (struct pcpu_quickbin,
			ptrs[slab->quickbin_len]);
	slab->quickbin = cpulocal_rt_alloc (quickbin_size, 128);
	if (!slab->quickbin)
		printk (PR_ERR "slab: cpulocal_rt_alloc returned NULL, not using quickbins\n");
	else
		for_each_present_cpu (cpu)
			that_cpu_write (&slab->quickbin->num, cpu, 0);

	strncpy (slab->name, name, sizeof (slab->name));

	bool flag = spin_lock_irq (&slab_list_lock);
	list_insert_back (&slab_list, &slab->slab_list);
	spin_unlock_irq (&slab_list_lock, flag);
}

/**
 * Provide wrappers for alloc_page and free_page that work when mm_is_early.
 */

static struct pfn_entry *
slab_alloc_page (void)
{
	if (mm_is_early) {
		unsigned long addr = initmem_alloc_phys (PAGE_SIZE, PAGE_SIZE);
		if (!addr)
			return NULL;
		return phys_to_pfn_entry (addr);
	}

	return alloc_page (ALLOC_KERNEL);
}

static void
slab_free_page (struct pfn_entry *page)
{
	set_page_flags (page, 0);
	if (mm_is_early) {
		initmem_free_phys (pfn_entry_to_phys (page), PAGE_SIZE);
		return;
	}
	free_page (page, ALLOC_KERNEL);
}

/**
 * Allocate an object from a slab.
 */
void *
slab_alloc (struct slab *slab)
{
	if (__builtin_expect (in_irq (), 0)) {
		printk (PR_ERR "slab_alloc was called in IRQ context!  This is a kernel bug.\n");
		return NULL;
	}

	/**
	 * Allocation fastpath.
	 *
	 * This fastpath should be kept as small as possible.  It should compile
	 * to something like this on x86:
	 *
	 *	movq offsetof(slab, quickbin)(%rdi), %rsi
	 *	incl %gs:__preempt_state
	 *	testq %rsi, %rsi
	 *	jz .Lallocation_slowpath
	 *	movq %gs:(%rsi), %rdx
	 *	testq %rdx, %rdx
	 *	jz .Lallocation_slowpath
	 *	movq %gs:(%rsi,%rdx,8), %rax
	 *	decl %rdx
	 *	movq %rdx, %gs:(%rsi)
	 *	decl %gs:__preempt_state
	 *	jz .Lcall_preempt_resched
	 *	ret
	 * .Lcall_preempt_resched:
	 *	movq %rax, -N(%rbp)
	 *	call preempt_resched
	 *	movq -N(%rbp), %rax
	 *	leave
	 *	ret
	 * .Lallocation_slowpath:
	 */
	struct pcpu_quickbin *quick = slab->quickbin;
	preempt_off ();
	if (__builtin_expect (!!quick, 1)) {
		unsigned long num = this_cpu_read (&quick->num);
		if (__builtin_expect (!!num, 1)) {
			num--;
			this_cpu_write (&quick->num, num);
			void *obj = this_cpu_read (&quick->ptrs[num]);
			inc_nr_quickbin_hit ();
			preempt_on ();
			return obj;
		}
	}
	inc_nr_quickbin_miss ();

	__spin_lock (&slab->lock);
	struct pfn_entry *page = NULL;
	if (slab->nr_full) {
		page = list_item (struct pfn_entry, pfn_slab.list,
				slab->pg_full_list.next);
		slab->nr_full--;
		list_delete (&page->pfn_slab.list);
	} else if (slab->nr_partial) {
		page = list_item (struct pfn_entry, pfn_slab.list,
				slab->pg_partial_list.next);
		slab->nr_partial--;
		list_delete (&page->pfn_slab.list);
	}

	void *obj, *head;
	if (page) {
		obj = page->pfn_slab.pobj;
		head = *(void **) obj;
		unsigned long nalloc = 1;

		if (__builtin_expect (!!quick, 1)) {
			unsigned long i = 0;
			for (; i < slab->quickbin_len; i++) {
				if (!head)
					break;
				this_cpu_write (&quick->ptrs[i], head);
				head = *(void **) head;
			}
			this_cpu_write (&quick->num, i);
			nalloc += i;
		}

		page->pfn_slab.pobj = head;
		page->pfn_slab.nfree -= nalloc;
		slab->nr_obj_free -= nalloc;
		if (head) {
			slab->nr_partial++;
			list_insert(&slab->pg_partial_list, &page->pfn_slab.list);
		} else
			slab->nr_empty++;
	} else {
		inc_nr_alloc_page ();
		page = slab_alloc_page ();
		if (__builtin_expect (!page, 0)) {
			inc_nr_failed_alloc ();
			obj = NULL;	/* OOM, womp womp...  */
		} else {
			set_page_flags (page, PFN_SLAB);
			page->pfn_slab.slab = slab;

			obj = (void *) pfn_entry_to_virt (page);
			unsigned long nalloc = 1;
			unsigned long i = 1;
			if (__builtin_expect (!!quick, 1)) {
				nalloc = 1 + slab->quickbin_len;
				for (; i < nalloc; i++) {
					head = obj + i * slab->real_obj_size;
					this_cpu_write (&quick->ptrs[i - 1], head);
				}
				this_cpu_write (&quick->num, slab->quickbin_len);
			}

			head = NULL;
			page->pfn_slab.nfree = slab->objs_per_page - i;
			for (; i < slab->objs_per_page; i++) {
				void *tmp = obj + i * slab->real_obj_size;
				*(void **) tmp = head;
				head = tmp;
			}

			page->pfn_slab.pobj = head;
			if (head) {
				slab->nr_partial++;
				slab->nr_obj_free += page->pfn_slab.nfree;
				list_insert (&slab->pg_partial_list, &page->pfn_slab.list);
			} else
				slab->nr_empty++;
		}
	}
	__spin_unlock (&slab->lock);
	preempt_on ();
	return obj;
}

/**
 * Free an object back to the slab.
 */
void
slab_free (void *ptr)
{
	if (__builtin_expect (in_irq (), 0)) {
		printk (PR_ERR "slab_free was called in IRQ context!  This is a kernel bug.\n");
		printk (PR_WARN "slab_free: leaking memory...\n");
		return;
	}

	struct pfn_entry *page = virt_to_pfn_entry (ptr);
	struct slab *slab = page->pfn_slab.slab;

	struct pcpu_quickbin *quick = slab->quickbin;
	preempt_off ();
	if (__builtin_expect (!!quick, 1)) {
		unsigned long num = this_cpu_read (&quick->num);
		if (num != slab->quickbin_len) {
			this_cpu_write (&quick->ptrs[num], ptr);
			this_cpu_write (&quick->num, num + 1);
			inc_nr_quickbin_free ();
			preempt_on ();
			return;
		}
	}

	inc_nr_expensive_free ();
	bool should_free_page = false;

	__spin_lock (&slab->lock);
	*(void **) ptr = page->pfn_slab.pobj;
	page->pfn_slab.pobj = ptr;
	page->pfn_slab.nfree++;
	slab->nr_obj_free++;
	if (page->pfn_slab.nfree == 1) {
		/** objs_per_page is never equal to 1.  */
		slab->nr_empty--;
		slab->nr_partial++;
		list_insert (&slab->pg_partial_list, &page->pfn_slab.list);
	} else if (page->pfn_slab.nfree == slab->objs_per_page) {
		slab->nr_partial--;
		list_delete (&page->pfn_slab.list);
		if (slab->nr_full < 2) {
			slab->nr_full++;
			list_insert (&slab->pg_full_list, &page->pfn_slab.list);
		} else {
			should_free_page = true;
			slab->nr_obj_free -= slab->objs_per_page;
			inc_nr_free_page ();
		}
	}
	__spin_unlock (&slab->lock);
	preempt_on ();

	if (should_free_page)
		slab_free_page (page);
}

static int slab_initialized;
static spinlock_t slab_init_mutex;

static inline void
slab_inited (void)
{
	if (atomic_load_acquire (&slab_initialized))
		return;

	bool flag = spin_lock_irq (&slab_init_mutex);
	if (!slab_initialized) {
		slab_init_new (&static_slab, "slab", sizeof (struct slab));
		atomic_store_release (&slab_initialized, 1);
	}
	spin_unlock_irq (&slab_init_mutex, flag);
}

struct slab *
slab_create (const char *name, unsigned long objsize)
{
	if (!objsize || objsize > (PAGE_SIZE / 2)) {
		/**
		 * A zero object size is invalid, as is an object size larger
		 * than half the page size.
		 */
		printk (PR_WARN "slab: Attempt to create slab %s with object size %lu\n",
			name, objsize);
		return NULL;
	}

	slab_inited ();
	struct slab *handle = slab_alloc (&static_slab);
	if (!handle)
		return NULL;

	slab_init_new (handle, name, objsize);
	return handle;
}
