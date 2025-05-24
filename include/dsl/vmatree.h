/**
 * VMA trees.
 * Copyright (C) 2025-present  dbstream
 *
 * The VMA tree is an augmented AVL tree that, in addition to keeping track of
 * nodes and node ranges, also keeps track of gaps in between nodes for faster
 * allocations.
 */
#pragma once

#include <container_of.h>
#include "list.h"

namespace dsl {

static constexpr uintptr_t VMA_TREE_MAX = uintptr_t(-1);

struct VMANode {
	/* Linked list of VMA nodes.  */
	ListHead list;

	/* The range that this node covers, inclusive.  */
	uintptr_t first, last;

	/* Normal AVL tree linkage.  */
	VMANode *child[2];
	VMANode *parent;
	int height;

	/* Keep track of the gap to the previous entry and propagate upwards. */
	uintptr_t prev_gap, biggest_gap;
};

typedef TypedList<VMANode, &VMANode::list> VMAList;

struct VMATree {
	enum Dir { LEFT = 0, RIGHT = 1 };

	VMAList list;
	VMANode *root;

	/**
	 * VMATree::init - initialize the tree.
	 */
	constexpr void
	init (void)
	{
		list.init ();
		root = nullptr;
	}

	/**
	 * VMATree::empty - test if a tree is empty.
	 */
	inline bool
	empty (void)
	{
		return !root;
	}

	/**
	 * VMATree::next - get the next node.
	 * @node: pointer to VMA node or nullptr for first node
	 * Returns nullptr if there is no next node.
	 */
	inline VMANode *
	next (VMANode *node)
	{
		ListHead *entry;

		if (node)
			entry = &node->list;
		else
			entry = &list.m_list;

		entry = entry->next;

		if (entry == &list.m_list)
			return nullptr;
		else
			return container_of (&VMANode::list, entry);
	}

	/**
	 * VMATree::prev - get the previous node.
	 * @node: pointeer to VMA node or nullptr for last node
	 * Returns nullptr if there is no previous node.
	 */
	inline VMANode *
	prev (VMANode *node)
	{
		ListHead *entry;

		if (node)
			entry = &node->list;
		else
			entry = &list.m_list;

		entry = entry->prev;

		if (entry == &list.m_list)
			return nullptr;
		else
			return container_of (&VMANode::list, entry);
	}

	/**
	 * VMATree::first - get the first node.
	 * Returns nullptr if the tree is empty.
	 */
	inline VMANode *
	first (void)
	{
		return next (nullptr);
	}

	/**
	 * VMATree::last - get the last node.
	 * Returns nullptr if the tree is empty.
	 */
	inline VMANode *
	last (void)
	{
		return prev (nullptr);
	}

	/**
	 * VMATree::find - find the node which contains addr.
	 * @addr: address to find
	 * Returns nullptr if no node contains addr.
	 */
	VMANode *
	find (uintptr_t addr);

	/**
	 * VMATree::find_above - find the first node on or above addr.
	 * @addr: address to find
	 * Returns nullptr if there is no node on or above addr.
	 */
	VMANode *
	find_above (uintptr_t addr);

	/**
	 * VMATree::find_before - find the first node below addr.
	 * @addr: address to find
	 * Returns nullptr if there is no node below addr.
	 */
	VMANode *
	find_below (uintptr_t addr);

	/**
	 * VMATree::find_free_bottomup - find the first free hole.
	 * @out: pointer to which the address of the found hole is stored
	 * @size: required size
	 * @align: required start address alignment
	 * @min_addr: the lowest acceptable address
	 * @max_addr: the highest acceptable address
	 * Returns false if no suitable free hole was found.
	 */
	bool
	find_free_bottomup (uintptr_t *out,
			uintptr_t size, uintptr_t align,
			uintptr_t min_addr, uintptr_t max_addr);

	/**
	 * VMATree::find_free_topdown - find the last free hole.
	 * @out: pointer to which the address of the found hole is stored
	 * @size: required size
	 * @align: required start address alignment
	 * @min_addr: the lowest acceptable address
	 * @max_addr: the highest acceptable address
	 * Returns false if no suitable free hole was found.
	 */
	bool
	find_free_topdown (uintptr_t *out,
			uintptr_t size, uintptr_t align,
			uintptr_t min_addr, uintptr_t max_addr);

	/**
	 * VMATree::insert - insert a node into the tree.
	 * @node: node to insert
	 */
	void
	insert (VMANode *node);

	/**
	 * VMATree::remove - remove a node from the tree.
	 * @node: node to remove
	 */
	void
	remove (VMANode *node);

	/**
	 * VMATree::adjust - fixup the VMA tree after having modified a node.
	 * @node: the node which was modified
	 */
	void
	adjust (VMANode *node);
private:
	/**
	 * VMATree::fixup - fixup the tree after modification.
	 * @node: node to start the fixup at
	 */
	void
	fixup (VMANode *node);
};

template<class T, VMANode T::*F>
class TypedVMATree {
	VMATree m_tree;

public:
	constexpr
	TypedVMATree (void)
	{}

	constexpr void
	init (void)
	{
		m_tree.init ();
	}

	static inline T *
	container_of (VMANode *node)
	{
		return ::container_of (F, node);
	}

	static inline T *
	container_of_or_null (VMANode *node)
	{
		return node ? container_of (node) : nullptr;
	}

	inline void
	insert (T *node)
	{
		m_tree.insert (&(node->*F));
	}

	inline void
	remove (T *node)
	{
		m_tree.remove (&(node->*F));
	}

	inline T *
	first (void)
	{
		return container_of_or_null (m_tree.first ());
	}

	inline T *
	last (void)
	{
		return container_of_or_null (m_tree.last ());
	}

	inline T *
	next (T *node)
	{
		return container_of_or_null (m_tree.next (&(node->*F)));
	}

	inline T *
	prev (T *node)
	{
		return container_of_or_null (m_tree.prev (&(node->*F)));
	}

	inline T *
	find (uintptr_t addr)
	{
		return container_of_or_null (m_tree.find (addr));
	}

	inline T *
	find_above (uintptr_t addr)
	{
		return container_of_or_null (m_tree.find_above (addr));
	}

	inline T *
	find_below (uintptr_t addr)
	{
		return container_of_or_null (m_tree.find_below (addr));
	}

	inline bool
	find_free_bottomup (uintptr_t *out,
			uintptr_t size, uintptr_t align,
			uintptr_t min_addr, uintptr_t max_addr)
	{
		return m_tree.find_free_bottomup (out, size, align,
				min_addr, max_addr);
	}

	inline bool
	find_free_topdown (uintptr_t *out,
			uintptr_t size, uintptr_t align,
			uintptr_t min_addr, uintptr_t max_addr)
	{
		return m_tree.find_free_topdown (out, size, align,
				min_addr, max_addr);
	}
};

}
