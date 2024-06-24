/**
 * Generic circular linked list.
 * Copyright (C) 2024  dbstream
 *
 * List insertion uses a release store on list->next, so traversing a list
 * atomically is safe against insertion. If users want safety against
 * deletion, they must ensure that the surrounding object stays valid for at
 * least as long as readers can observe it. Atomicity is only provided for
 * forwards iteration.
 */
#ifndef DAVIX_LIST_H
#define DAVIX_LIST_H 1

#include <davix/atomic.h>
#include <davix/stdbool.h>
#include <davix/stddef.h>

struct list {
	struct list *next, *prev;
};

#define DEFINE_EMPTY_LIST(name) struct list name = { &name, &name };

static inline void
list_init (struct list *list)
{
	list->next = list;
	list->prev = list;
}

static inline bool
list_empty (struct list *list)
{
	return list == list->next;
}

static inline void
list_insert (struct list *list, struct list *entry)
{
	entry->next = list->next;
	entry->prev = list;
	list->next->prev = entry;
	atomic_store_release (&list->next, entry);
}

static inline void
list_insert_back (struct list *list, struct list *entry)
{
	entry->next = list;
	entry->prev = list->prev;
	struct list *tmp = list->prev;
	list->prev = entry;
	atomic_store_release (&tmp->next, entry);
}

static inline void
list_delete (struct list *entry)
{
	entry->next->prev = entry->prev;
	atomic_store_relaxed (&entry->prev->next, entry->next);
}

static inline void *
__list_item (struct list *entry, unsigned long offset)
{
	return (void *) ((unsigned long) entry - offset);
}

#define list_item(tp, name, entry) \
	((tp *) __list_item ((entry), offsetof (tp, name)))

#define list_for_each(name, lst)				\
for (struct list *name = (lst);					\
	(name = atomic_load_acquire (&name->next)) != (lst);)

#endif /* DAVIX_LIST_H */
