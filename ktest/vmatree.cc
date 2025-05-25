/**
 * VMATree ktest module.
 * Copyright (C) 2025-present  dbstream
 */
#include <davix/panic.h>
#include <davix/printk.h>
#include <davix/slab.h>
#include <dsl/vmatree.h>

static SlabAllocator *node_alloc;

static inline dsl::VMANode *
new_node (uintptr_t first, uintptr_t last)
{
	dsl::VMANode *node = (dsl::VMANode *) slab_alloc (node_alloc, ALLOC_KERNEL);
	if (!node)
		panic ("OOM in ktest_vmatree");

	node->first = first;
	node->last = last;
	return node;
}

static inline void
free_node (dsl::VMANode *node)
{
	slab_free (node);
}

static dsl::VMATree tree;

[[gnu::noinline]]
static inline int
assert_lookup_eq (int id, uintptr_t addr,
		uintptr_t first, uintptr_t last)
{
	dsl::VMANode *node = tree.find (addr);
	if (!node) {
		printk (PR_WARN "ktest_vmatree[%d]: assert_lookup_eq failed!  expected [%tu; %tu] but got nil\n",
				id, first, last);
		return 1;
	}

	if (node->first != first || node->last != last) {
		printk (PR_WARN "ktest_vmatree[%d]: assert_lookup_eq failed!  expected [%tu; %tu] but got [%tu; %tu]\n",
				id, first, last, node->first, node->last);
		return 1;
	} else
		return 0;
}

[[gnu::noinline]]
static inline int
assert_lookup_nil (int id, uintptr_t addr)
{
	dsl::VMANode *node = tree.find (addr);
	if (node) {
		printk (PR_WARN "ktest_vmatree[%d]: assert_lookup_nil failed!  expected nil but got [%tu; %tu]\n",
				id, node->first, node->last);
		return 1;
	} else
		return 0;
}

[[gnu::noinline]]
static inline int
assert_bottomup_free_eq (int id, uintptr_t size, uintptr_t align,
		uintptr_t low, uintptr_t high, uintptr_t expected)
{
	(void) id;
	uintptr_t addr;
	if (!tree.find_free_bottomup (&addr, size, align, low, high)) {
		printk (PR_WARN "ktest_vmatree[%d]: assert_bottomup_free_eq failed!  expected %tu but got nil\n",
				id, expected);
		return 1;
	}

	if (addr != expected) {
		printk (PR_WARN "ktest_vmatree[%d]: assert_bottomup_free_eq failed!  expected %tu but got %tu\n",
				id, expected, addr);
		return 1;
	} else
		return 0;
}

[[gnu::noinline]]
static inline int
assert_topdown_free_eq (int id, uintptr_t size, uintptr_t align,
		uintptr_t low, uintptr_t high, uintptr_t expected)
{
	(void) id;
	uintptr_t addr;
	if (!tree.find_free_topdown (&addr, size, align, low, high)) {
		printk (PR_WARN "ktest_vmatree[%d]: assert_topdown_free_eq failed!  expected %tu but got nil\n",
				id, expected);
		return 1;
	}

	if (addr != expected) {
		printk (PR_WARN "ktest_vmatree[%d]: assert_topdown_free_eq failed!  expected %tu but got %tu\n",
				id, expected, addr);
		return 1;
	} else
		return 0;
}

[[gnu::noinline]]
static inline int
assert_bottomup_free_nil (int id, uintptr_t size, uintptr_t align,
		uintptr_t low, uintptr_t high)
{
	uintptr_t addr;
	if (tree.find_free_bottomup (&addr, size, align, low, high)) {
		printk (PR_WARN "ktest_vmatree[%d]: assert_bottomup_free_nil failed!  expected nil but got %tu\n",
				id, addr);
		return 1;
	} else
		return 0;
}

[[gnu::noinline]]
static inline int
assert_topdown_free_nil (int id, uintptr_t size, uintptr_t align,
		uintptr_t low, uintptr_t high)
{
	uintptr_t addr;
	if (tree.find_free_topdown (&addr, size, align, low, high)) {
		printk (PR_WARN "ktest_vmatree[%d]: assert_topdown_free_nil failed!  expected nil but got %tu\n",
				id, addr);
		return 1;
	} else
		return 0;
}

[[gnu::noinline]]
static inline void
insert_node (uintptr_t first, uintptr_t last)
{
	tree.insert (new_node (first, last));
}

[[gnu::noinline]]
static inline void
remove_node (uintptr_t addr)
{
	dsl::VMANode *node = tree.find (addr);
	if (node) {
		tree.remove (node);
		free_node (node);
	}
}

[[gnu::noinline]]
static inline void
adjust_node (uintptr_t addr, uintptr_t first, uintptr_t last)
{
	dsl::VMANode *node = tree.find (addr);
	if (node) {
		node->first = first;
		node->last = last;
		tree.adjust (node);
	}
}

static int
run_testcases (void);

void
ktest_vmatree (void)
{
	printk (PR_NOTICE "Running VMATree ktests...\n");

	node_alloc = slab_create ("ktest_vmatree", sizeof (dsl::VMANode), alignof (dsl::VMANode));
	if (!node_alloc)
		panic ("OOM in ktest_vmatree");

	tree.init ();

	int num_failed = run_testcases ();

	if (num_failed == 0)
		printk (PR_NOTICE "ktest_vmatree: SUCCESS!\n");
	else
		printk (PR_ERROR "ktest_vmatree: FAIL!  %d testcases failed.\n", num_failed);
}

static int
run_testcases (void)
{
	int num_failed = 0;
#include "vmatree_testcases.inl"
	return num_failed;
}
