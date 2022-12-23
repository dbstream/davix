/* SPDX-License-Identifier: MIT */
#ifndef __DAVIX_LIST_H
#define __DAVIX_LIST_H

/*
 * Functions and macros for dealing with linked lists (`struct list`s).
 */

#include <davix/types.h>

#define LIST_INIT(name) {&name, &name}
#define DEFINE_LIST(name) struct list name = LIST_INIT(name)

/*
 * Initialize the list.
 */
static inline void list_init(struct list *head)
{
	head->prev = head;
	head->next = head;
}

/*
 * Helper for insertion operations.
 */
static inline void __list_add(struct list *prev, struct list *next, struct list *elem)
{
	elem->prev = prev;
	elem->next = next;
	next->prev = elem;
	prev->next = elem;
}

/*
 * Insert `elem` first.
 */
static inline void list_add(struct list *head, struct list *elem)
{
	__list_add(head, head->next, elem);
}

/*
 * Insert `elem` last.
 */
static inline void list_radd(struct list *head, struct list *elem)
{
	__list_add(head->prev, head, elem);
}

/*
 * Remove `elem` from the list.
 */
static inline void list_del(struct list *elem)
{
	elem->next->prev = elem->prev;
	elem->prev->next = elem->next;
}

/*
 * Replace `old_elem` with `new_elem`.
 */
static inline void list_replace(struct list *old_elem, struct list *new_elem)
{
	new_elem->prev = old_elem->prev;
	new_elem->next = old_elem->next;
	old_elem->next->prev = new_elem;
	old_elem->prev->next = new_elem;
}

/*
 * Swap the locations of `e1` and `e2`. They **can** be in different lists.
 */
static inline void list_swap(struct list *e1, struct list *e2)
{
	struct list *prev = e1->prev;
	list_del(e1);
	list_replace(e2, e1);
	if(prev == e2)
		prev = e1;
	list_add(prev, e2);
}

static inline int list_empty(struct list *list)
{
	return list == list->next;
}

/*
 * Re-insert `elem` into `list`.
 */
static inline void list_move(struct list *list, struct list *elem)
{
	list_del(elem);
	list_add(list, elem);
}

/*
 * Re-insert `elem` into `list`, but with `list_radd()`.
 */
static inline void list_rmove(struct list *list, struct list *elem)
{
	list_del(elem);
	list_radd(list, elem);
}

/*
 * Like list_move, but many at once.
 */
static inline void list_move_bulk(struct list *list,
	struct list *first, struct list *last)
{
	last->next->prev = first->prev;
	first->prev->next = last->next;

	first->prev = list;
	last->next = list->next;
	list->next->prev = last;
	list->next = first;
}

static inline void list_move_all(struct list *to, struct list *from)
{
	if(!list_empty(from))
		list_move_bulk(to, from->next, from->prev);
}

/*
 * Usually `struct list`s are embedded in other structs. These macros are
 * utilities for iterating over lists.
 */

/*
 * Get the next element of the list.
 * @ob: A pointer to the struct that contains the list.
 * @name: The name of the `struct list` member within `ob`.
 */
#define __list_next(ob, name) \
	container_of(ob->name.next, typeof(*ob), name)

/*
 * Similar to `__list_next()`, but in the other direction.
 */
#define __list_prev(ob, name) \
	container_of(ob->name.prev, typeof(*ob), name)

#define list_for_each(iter, list, name) \
	for(iter = container_of((list)->next, typeof(*iter), name); \
		&iter->name != (list); iter = __list_next(iter, name))

#define list_for_each_safe(iter, list, name, tmp) \
	for(iter = container_of((list)->next, typeof(*iter), name), \
		tmp = __list_next(iter, name); &iter->name != (list); \
		iter = tmp, tmp = __list_next(iter, name))

#endif /* __DAVIX_LIST_H */
