// SPDX-License-Identifier: GPL-3.0 OR BSD-3-Clause
/*
 * File: lib/vmamodify.cc
 * VMA tree modifications.
 *
 * Copyright (C) 2025  dbstream
 */
#include <dsl/vmatree.h>

static inline uintptr_t max(uintptr_t a, uintptr_t b)
{
	return (a > b) ? a : b;
}

namespace dsl {

static constexpr VMATree::Dir LEFT = VMATree::LEFT;
static constexpr VMATree::Dir RIGHT = VMATree::RIGHT;

static inline int node_height(VMANode *node)
{
	return node ? node->height : 0;
}

static inline int node_balance(VMANode *node)
{
	return node_height(node->child[RIGHT]) - node_height(node->child[LEFT]);
}

static inline uintptr_t node_max_gap(VMANode *node)
{
	return node ? node->biggest_gap : 0;
}

static inline void propagate(VMANode *node)
{
	int l_height = node_height(node->child[LEFT]);
	uintptr_t l_gap = node_max_gap(node->child[LEFT]);
	int r_height = node_height(node->child[RIGHT]);
	uintptr_t r_gap = node_max_gap(node->child[RIGHT]);

	int new_height = 1 + max(l_height, r_height);
	uintptr_t new_gap = max(node->prev_gap, max(l_gap, r_gap));

	node->height = new_height;
	node->biggest_gap = new_gap;
}

static inline VMANode *rotate(VMANode *node, VMATree::Dir dir)
{
	VMATree::Dir oth = (dir != LEFT) ? LEFT : RIGHT;
	VMANode *Z = node->child[oth];
	VMANode *tmp = Z->child[dir];

	node->child[oth] = tmp;
	if (tmp)
		tmp->parent = node;

	Z->child[dir] = node;
	node->parent = Z;
	propagate(node);
	propagate(Z);
	return Z;
}

void VMATree::fixup(VMANode *node)
{
	while (node) {
		int balance = node_balance(node);

		if (-1 <= balance && balance <= 1) {
			propagate(node);
			node = node->parent;
			continue;
		}

		VMANode *parent = node->parent;
		VMATree::Dir me = LEFT;
		if (parent && parent->child[LEFT] != node)
			me = RIGHT;

		VMATree::Dir dir = LEFT;
		VMATree::Dir oth = RIGHT;
		if (balance >= 0) {
			dir = RIGHT;
			oth = LEFT;
		}

		VMANode *Z = node->child[dir];
		if (dir == LEFT && node_balance(Z) > 0) {
			Z = rotate(Z, LEFT);
			Z->parent = node;
			node->child[LEFT] = Z;
		} else if (dir == RIGHT && node_balance(Z) < 0) {
			Z = rotate(Z, RIGHT);
			Z->parent = node;
			node->child[RIGHT] = Z;
		}

		Z = node;
		node = rotate(node, oth);
		node->parent = parent;
		if (parent)
			parent->child[me] = node;
		else
			root = node;

		node = parent;
	}
}

void VMATree::insert(VMANode *node)
{
	node->child[LEFT] = nullptr;
	node->child[RIGHT] = nullptr;
	node->height = 1;

	VMANode *parent = nullptr;
	VMANode *x = root;
	Dir dir = LEFT;
	while (x) {
		if (node->first < x->first)
			dir = LEFT;
		else
			dir = RIGHT;

		parent = x;
		x = x->child[dir];
	}

	VMANode *pred, *succ;
	if (dir == RIGHT) {
		pred = parent;
		succ = next(parent);
	} else {
		pred = prev(parent);
		succ = parent;
	}

	if (pred)
		node->prev_gap = node->first - pred->last - 1;
	else
		node->prev_gap = node->first;

	if (succ)
		succ->prev_gap = succ->first - node->last - 1;

	node->parent = parent;
	if (parent)
		parent->child[dir] = node;
	else
		root = node;

	if (pred)
		pred->list.push_front(&node->list);
	else
		list.push_front(node);

	fixup(node);
}

void VMATree::remove(VMANode *node)
{
	VMANode *pred, *succ, *Z, *parent = node->parent;

	Dir me = LEFT;
	if (parent && parent->child[LEFT] != node)
		me = RIGHT;

	pred = prev(node);
	succ = next(node);

	if (succ) {
		if (pred)
			succ->prev_gap = succ->first - pred->last - 1;
		else
			succ->prev_gap = succ->first;
	}

	node->list.remove();

	if (!node->child[RIGHT]) {
		Z = node->child[LEFT];
		if (parent)
			parent->child[me] = Z;
		else
			root = Z;

		if (Z)
			Z->parent = parent;
		else
			Z = parent;
		fixup(Z);
		return;
	}

	if (!node->child[LEFT]) {
		Z = node->child[RIGHT];
		Z->parent = parent;
		if (parent)
			parent->child[me] = Z;
		else
			root = Z;

		fixup(succ);
		return;
	}

	Z = succ;
	VMANode *Y = Z->parent;
	Z->parent = parent;
	Z->child[LEFT] = node->child[LEFT];
	Z->child[LEFT]->parent = Z;
	if (parent)
		parent->child[me] = Z;
	else
		root = Z;

	if (Y == node) {
		fixup(Z);
		return;
	}

	Y->child[LEFT] = Z->child[RIGHT];
	if (Y->child[LEFT])
		Y->child[LEFT]->parent = Y;
	Z->child[RIGHT] = node->child[RIGHT];
	Z->child[RIGHT]->parent = Z;
	fixup(Y);
}

void VMATree::adjust(VMANode *node)
{
	VMANode *pred = prev(node), *succ = next(node);

	if (pred)
		node->prev_gap = node->first - pred->last - 1;
	else
		node->prev_gap = node->first;

	if (succ) {
		succ->prev_gap = succ->first - node->last - 1;
		if (succ->height < node->height)
			node = succ;
	}

	do {
		propagate(node);
		node = node->parent;
	} while (node);
}

}

