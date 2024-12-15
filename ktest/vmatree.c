/**
 * Unit testing for the VMA tree.
 * Copyright (C) 2024  dbstream
 */

#if CONFIG_KTEST_VMATREE

#include <davix/panic.h>
#include <davix/printk.h>
#include <davix/slab.h>
#include <davix/vma_tree.h>

static struct slab *node_allocator;

static inline struct vma_node *
new_node (unsigned long first, unsigned long last)
{
	struct vma_node *node = slab_alloc (node_allocator);
	if (!node)
		panic ("OOM in ktest_vmatree");

	node->first = first;
	node->last = last;
	return node;
}

static inline void
free_node (struct vma_node *node)
{
	slab_free (node);
}

static struct vma_tree tree;

static int
assert_lookup_eq (int id, unsigned long addr,
	unsigned long expected_first, unsigned long expected_last)
{
	struct vma_tree_iterator it;
	if (!vma_tree_find (&it, &tree, addr)) {
		printk (PR_WARN "ktest_vmatree[%d]: assert_lookup_eq failed! (expected [%lu; %lu], got nil)\n",
			id, expected_first, expected_last);
		return 1;
	}

	struct vma_node *node = vma_node (&it);
	if (node->first != expected_first || node->last != expected_last) {
		printk (PR_WARN "ktest_vmatree[%d]: assert_lookup_eq failed! (expected [%lu; %lu], got [%lu; %lu])\n",
			id, expected_first, expected_last, node->first, node->last);
		return 1;
	}

	return 0;
}

static int
assert_lookup_nil (int id, unsigned long addr)
{
	struct vma_tree_iterator it;
	if (vma_tree_find (&it, &tree, addr)) {
		struct vma_node *node = vma_node (&it);
		printk (PR_WARN "ktest_vmatree[%d]: assert_lookup_nil failed! (expected nil, got [%lu; %lu])\n",
			id, node->first, node->last);
		return 1;
	}

	return 0;
}

static int
assert_bottomup_free_eq (int id, unsigned long size, unsigned long align,
	unsigned long low, unsigned long high, unsigned long expected)
{
	unsigned long addr;
	if (!vma_tree_find_free_bottomup (&addr, &tree, size, align, low, high)) {
		printk (PR_WARN "ktest_vmatree[%d]: assert_bottomup_free_eq failed! (expected %lu, got nil)\n",
			id, expected);
		return 1;
	}

	if (addr != expected) {
		printk (PR_WARN "ktest_vmatree[%d]: assert_bottomup_free_eq failed! (expected %lu, got %lu)\n",
			id, expected, addr);
		return 1;
	}

	return 0;
}

static int
assert_topdown_free_eq (int id, unsigned long size, unsigned long align,
	unsigned long low, unsigned long high, unsigned long expected)
{
	unsigned long addr;
	if (!vma_tree_find_free_topdown (&addr, &tree, size, align, low, high)) {
		printk (PR_WARN "ktest_vmatree[%d]: assert_topdown_free_eq failed! (expected %lu, got nil)\n",
			id, expected);
		return 1;
	}

	if (addr != expected) {
		printk (PR_WARN "ktest_vmatree[%d]: assert_topdown_free_eq failed! (expected %lu, got %lu)\n",
			id, expected, addr);
		return 1;
	}

	return 0;
}

static int
assert_bottomup_free_nil (int id, unsigned long size, unsigned long align,
	unsigned long low, unsigned long high)
{
	unsigned long addr;
	if (vma_tree_find_free_bottomup (&addr, &tree, size, align, low, high)) {
		printk (PR_WARN "ktest_vmatree[%d]: assert_bottomup_free_nil failed! (expected nil, got %lu)\n",
			id, addr);
		return 1;
	}

	return 0;
}

static int
assert_topdown_free_nil (int id, unsigned long size, unsigned long align,
	unsigned long low, unsigned long high)
{
	unsigned long addr;
	if (vma_tree_find_free_topdown (&addr, &tree, size, align, low, high)) {
		printk (PR_WARN "ktest_vmatree[%d]: assert_topdown_free_nil failed! (expected nil, got %lu)\n",
			id, addr);
		return 1;
	}

	return 0;
}

static void
insert_node (unsigned long first, unsigned long last)
{
	vma_tree_insert (&tree, new_node (first, last));
}

static void
remove_node (unsigned long addr)
{
	struct vma_tree_iterator it;
	if (vma_tree_find (&it, &tree, addr)) {
		vma_tree_remove (&tree, vma_node (&it));
		free_node (vma_node (&it));
	}
}

static void
adjust_node (unsigned long addr, unsigned long first, unsigned long last)
{
	struct vma_tree_iterator it;
	if (vma_tree_find (&it, &tree, addr)) {
		vma_node (&it)->first = first;
		vma_node (&it)->last = last;
		vma_tree_adjust (&tree, vma_node (&it));
	}
}

static int
run_testcases (void);

void
ktest_vmatree (void)
{
	printk (PR_NOTICE "Running vmatree ktests...\n");

	node_allocator = slab_create ("ktest_vmatree", sizeof (struct vma_node));
	if (!node_allocator)
		panic ("OOM in ktest_vmatree");

	vma_tree_init (&tree);

	int num_failed = run_testcases ();
#if 0
	insert_node (1000UL, 2000UL);
	num_failed += assert_lookup_nil (0, 999UL);
	num_failed += assert_lookup_nil (1, 2001UL);
	num_failed += assert_lookup_eq (2, 2000UL, 1000UL, 2000UL);
	num_failed += assert_lookup_eq (3, 1000UL, 1000UL, 2000UL);
	insert_node (3000UL, -1UL);
	num_failed += assert_lookup_nil (4, 0UL);
	num_failed += assert_lookup_nil (5, 2999UL);
	num_failed += assert_lookup_eq (6, 3000UL, 3000UL, -1UL);
	num_failed += assert_bottomup_free_nil (7, 1001UL, 1UL, 0UL, -1UL);
	num_failed += assert_topdown_free_nil (8, 1000UL, 1UL, 1UL, -1UL);
	num_failed += assert_bottomup_free_eq (9, 999UL, 1UL, 2UL, 10001UL, 2001UL);
	num_failed += assert_topdown_free_eq (10, 10UL, 16UL, 0UL, -1UL, 2976UL);
	adjust_node (3000UL, 3000UL, 3999UL);
	num_failed += assert_topdown_free_eq (11, 5UL, 2UL, 0UL, -1UL, -6UL);
	num_failed += assert_bottomup_free_eq (12, 5UL, 1UL, 3500UL, 4500UL, 4000UL);
	remove_node (3000UL);
	num_failed += assert_lookup_nil (13, 3000UL);
#endif

	printk (PR_NOTICE "ktest_vmatree: %d testcases failed\n", num_failed);
}

static int
run_testcases (void)
{
	int num_failed = 0;
#include "vmatree_testcases.inl"
	return num_failed;
}

#endif
