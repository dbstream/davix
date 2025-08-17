// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/dsl/hlist.h
 * Intrusive linked list datastructure.
 *
 * Copyright (C) 2025  dbstream
 *
 * HList is very similar indeed to List.
 */
#pragma once

#include <container_of.h>

namespace dsl {

struct HListEntry {
	HListEntry *next, **link;

	constexpr void remove(void)
	{
		if (next)
			next->link = link;
		*link = next;

		next = (HListEntry *) 0xdeadbeefUL;
		link = (HListEntry **) 0xcafebabeUL;
	}
};

struct HList {
	HListEntry *head = nullptr;

	constexpr void init(void)
	{
		head = nullptr;
	}

	constexpr void push(HListEntry *entry)
	{
		entry->next = head;
		entry->link = &head;
		if (head)
			head->link = &entry->next;
		head = entry;
	}

	constexpr bool empty(void) const
	{
		return head == nullptr;
	}

	template<class T>
	struct _iterator_impl {
	private:
		T *m_current;
	public:
		typedef _iterator_impl<T> type;
		typedef T value_type;

		constexpr _iterator_impl(T *node)
			: m_current (node)
		{}

		constexpr T *operator *(void) const
		{
			return m_current;
		}

		constexpr T *operator->(void) const
		{
			return m_current;
		}

		constexpr bool operator==(const _iterator_impl<T> &other) const
		{
			return m_current == other.m_current;
		}

		constexpr bool operator!=(const _iterator_impl<T> &other) const
		{
			return m_current != other.m_current;
		}

		constexpr _iterator_impl<T> &operator++(void)
		{
			m_current = m_current->next;
			return *this;
		}

		constexpr _iterator_impl<T> operator++(int dummy)
		{
			(void) dummy;
			T *old = m_current;
			m_current = m_current->next;
			return { old };
		}
	};

	typedef _iterator_impl<HListEntry> iterator;
	typedef _iterator_impl<const HListEntry> const_iterator;

	constexpr iterator begin(void)
	{
		return iterator(head);
	}

	constexpr iterator end(void)
	{
		return iterator(nullptr);
	}

	constexpr const_iterator begin(void) const
	{
		return const_iterator(head);
	}

	constexpr const_iterator end(void) const
	{
		return const_iterator(nullptr);
	}

	constexpr const_iterator cbegin(void) const
	{
		return const_iterator(head);
	}

	constexpr const_iterator cend(void) const
	{
		return const_iterator(nullptr);
	}
};

template<class T, HListEntry T::*F>
struct TypedHList {
public:
	HList m_list;

	constexpr void init(void)
	{
		m_list.init();
	}

	constexpr void push(T *entry)
	{
		m_list.push(&(entry->*F));
	}

	T *pop(void)
	{
		T *entry = *begin();
		(entry->*F).remove();
		return entry;
	}

	constexpr bool empty(void) const
	{
		return m_list.empty();
	}

	static inline T *container_of(HListEntry *node)
	{
		return ::container_of<T, HListEntry>(F, node);
	}

	static inline const T *const_container_of(const HListEntry *node)
	{
		return ::container_of<const T, const HListEntry>(F, node);
	}

	struct iterator : HList::iterator {
		T *operator *(void) const
		{
			return container_of(HList::iterator::operator *());
		}

		T *operator->(void) const
		{
			return container_of(HList::iterator::operator *());
		}
	};

	struct const_iterator : HList::const_iterator {
		const T *operator *(void) const
		{
			return const_container_of(HList::const_iterator::operator *());
		}

		const T *operator->(void) const
		{
			return const_container_of(HList::const_iterator::operator *());
		}
	};

	constexpr iterator begin(void)
	{
		return iterator(m_list.head);
	}

	constexpr iterator end(void)
	{
		return iterator(nullptr);
	}

	constexpr const_iterator begin(void) const
	{
		return const_iterator(m_list.head);
	}

	constexpr const_iterator end(void) const
	{
		return const_iterator(nullptr);
	}

	constexpr const_iterator cbegin(void) const
	{
		return const_iterator(m_list.head);
	}

	constexpr const_iterator cend(void) const
	{
		return const_iterator(nullptr);
	}
};

} /* namespace dsl  */

