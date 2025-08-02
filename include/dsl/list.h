// SPDX-License-Identifier: GPL-3.0 OR BSD-3-Clause
/*
 * File: include/dsl/list.h
 * Intrusive linked list datastructure.
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

#include <container_of.h>

namespace dsl {

struct ListHead {
	ListHead *next, *prev;

	constexpr void init(void)
	{
		next = this;
		prev = this;
	}

	constexpr void push_front(ListHead *node)
	{
		node->prev = this;
		node->next = next;
		next->prev = node;
		next = node;
	}

	constexpr void push_back(ListHead *node)
	{
		node->prev = prev;
		node->next = this;
		prev->next = node;
		prev = node;
	}

	constexpr void remove(void)
	{
		next->prev = prev;
		prev->next = next;

		prev = (ListHead *) 0xdeadbeefUL;
		next = (ListHead *) 0xcafebabeUL;
	}

	/**
	 * ListHead::adopt - move all list entries from @other to this ListHead.
	 * @other: ListHead to move entries from
	 *
	 * This function effectively initializes this ListHead with the contents
	 * of @other, leaving @other as the empty list.
	 */
	constexpr void adopt(ListHead *other)
	{
		ListHead *first = other->next;
		ListHead *last = other->prev;
		if (first == other) {
			next = this;
			prev = this;
		} else {
			other->next = other;
			other->prev = other;
			first->prev = this;
			last->next = this;
			next = first;
			prev = last;
		}
	}

	constexpr bool empty(void) const
	{
		return next == this;
	}

	template<class T>
	struct _iterator_impl {
	private:
		T *m_current;
	public:
		typedef _iterator_impl<T> type;
		typedef T value_type;

		constexpr _iterator_impl(T *node)
			: m_current(node)
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

		constexpr _iterator_impl<T> &operator--(void)
		{
			m_current = m_current->prev;
			return *this;
		}

		constexpr _iterator_impl<T> operator++(int dummy)
		{
			(void) dummy;
			ListHead *old = m_current;
			m_current = m_current->next;
			return { old };
		}

		constexpr _iterator_impl<T> operator--(int dummy)
		{
			(void) dummy;
			ListHead *old = m_current;
			m_current = m_current->prev;
			return { old };
		}
	};

	typedef _iterator_impl<ListHead> iterator;
	typedef _iterator_impl<const ListHead> const_iterator;

	constexpr iterator begin(void)
	{
		return iterator(next);
	}

	constexpr iterator end(void)
	{
		return iterator(this);
	}

	constexpr const_iterator begin(void) const
	{
		return const_iterator(next);
	}

	constexpr const_iterator end(void) const
	{
		return const_iterator(this);
	}

	constexpr const_iterator cbegin(void) const
	{
		return const_iterator (next);
	}

	constexpr const_iterator cend(void) const
	{
		return const_iterator (this);
	}

	constexpr ListHead *first(void) const
	{
		if (next == this)
			return nullptr;
		else
			return next;
	}

	constexpr ListHead *last(void) const
	{
		if (prev == this)
			return nullptr;
		else
			return prev;
	}
};

template<class T, ListHead T::*F>
struct TypedList {
public:
	ListHead m_list;

	constexpr TypedList(void)
		: m_list { &m_list, &m_list }
	{}

	constexpr void init(void)
	{
		m_list.init();
	}

	constexpr void push_front(T *value)
	{
		m_list.push_front(&(value->*F));
	}

	constexpr void push_back(T *value)
	{
		m_list.push_back(&(value->*F));
	}

	constexpr void adopt(TypedList<T, F> *other)
	{
		m_list.adopt(&other->m_list);
	}

	constexpr bool empty(void) const
	{
		return m_list.empty();
	}

	static inline T *container_of(ListHead *node)
	{
		return ::container_of<T, ListHead> (F, node);
	}

	static inline const T *const_container_of(const ListHead *node)
	{
		return ::container_of<const T, const ListHead> (F, node);
	}

	struct iterator : ListHead::iterator {
		T *operator *(void) const
		{
			return container_of(ListHead::iterator::operator *());
		}

		T *operator->(void) const
		{
			return container_of(ListHead::iterator::operator *());
		}
	};

	struct const_iterator : ListHead::const_iterator {
		const T *operator *(void) const
		{
			return const_container_of(ListHead::const_iterator::operator *());
		}

		const T *operator->(void) const
		{
			return container_of(ListHead::const_iterator::operator *());
		}
	};

	constexpr iterator begin(void)
	{
		return iterator(m_list.next);
	}

	constexpr iterator end(void)
	{
		return iterator(&m_list);
	}

	constexpr const_iterator begin(void) const
	{
		return const_iterator(m_list.next);
	}

	constexpr const_iterator end(void) const
	{
		return const_iterator(&m_list);
	}

	constexpr const_iterator cbegin(void) const
	{
		return const_iterator(m_list.next);
	}

	constexpr const_iterator cend(void) const
	{
		return const_iterator(&m_list);
	}

	T *first(void)
	{
		ListHead *entry = m_list.first();
		if (entry == nullptr)
			return nullptr;
		else
			return container_of(entry);
	}

	T *last(void)
	{
		ListHead *entry = m_list.last();
		if (entry == nullptr)
			return nullptr;
		else
			return container_of(entry);
	}

	T *pop_front(void)
	{
		T *elem = first();
		if (elem)
			(&(elem->*F))->remove();
		return elem;
	}

	T *pop_back(void)
	{
		T *elem = last();
		if (elem)
			(&(elem->*F))->remove();
		return elem;
	}
};

} /* namespace dsl */

