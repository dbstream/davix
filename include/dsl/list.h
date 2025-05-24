/**
 * Intrusive linked list datastructure.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#include <container_of.h>
#include "reverse_iterator.h"

namespace dsl {

struct ListHead {
	ListHead *next, *prev;

	constexpr void
	init (void)
	{
		next = this;
		prev = this;
	}

	constexpr void
	push_front (ListHead *node)
	{
		node->prev = this;
		node->next = next;
		next->prev = node;
		next = node;
	}

	constexpr void
	push_back (ListHead *node)
	{
		node->prev = prev;
		node->next = this;
		prev->next = node;
		prev = node;
	}

	constexpr void
	remove (void)
	{
		next->prev = prev;
		prev->next = next;

		/** poison this node to prevent mistakes  */
		prev = nullptr;
		next = nullptr;
	}

	constexpr bool
	empty (void) const
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

		constexpr
		_iterator_impl (T *node)
			: m_current (node)
		{}

		constexpr T *
		operator * (void) const
		{
			return m_current;
		}

		constexpr T *
		operator-> (void) const
		{
			return m_current;
		}

		constexpr bool
		operator== (const _iterator_impl<T> &other) const
		{
			return m_current == other.m_current;
		}

		constexpr bool
		operator!= (const _iterator_impl<T> &other) const
		{
			return m_current != other.m_current;
		}

		constexpr _iterator_impl<T> &
		operator++ (void)
		{
			m_current = m_current->next;
			return *this;
		}

		constexpr _iterator_impl<T> &
		operator-- (void)
		{
			m_current = m_current->prev;
			return *this;
		}

		/** NB: postfix increment  */
		constexpr _iterator_impl<T>
		operator++ (int dummy)
		{
			(void) dummy;
			ListHead *old = m_current;
			m_current = m_current->next;
			return { old };
		}

		/** NB: postfix decrement  */
		constexpr _iterator_impl<T>
		operator-- (int dummy)
		{
			(void) dummy;
			ListHead *old = m_current;
			m_current = m_current->prev;
			return { old };
		}
	};

	typedef _iterator_impl<ListHead> iterator;
	typedef _iterator_impl<const ListHead> const_iterator;
	typedef ::dsl::reverse_iterator<iterator> reverse_iterator;
	typedef ::dsl::reverse_iterator<const_iterator> const_reverse_iterator;

	constexpr iterator
	begin (void)
	{
		return iterator (next);
	}

	constexpr iterator
	end (void)
	{
		return iterator (this);
	}

	constexpr const_iterator
	begin (void) const
	{
		return const_iterator (next);
	}

	constexpr const_iterator
	end (void) const
	{
		return const_iterator (this);
	}

	constexpr const_iterator
	cbegin (void) const
	{
		return const_iterator (next);
	}

	constexpr const_iterator
	cend (void) const
	{
		return const_iterator (this);
	}

	constexpr reverse_iterator
	rbegin (void)
	{
		return reverse_iterator (prev);
	}

	constexpr reverse_iterator
	rend (void)
	{
		return reverse_iterator (this);
	}

	constexpr const_reverse_iterator
	rbegin (void) const
	{
		return const_reverse_iterator (prev);
	}

	constexpr const_reverse_iterator
	rend (void) const
	{
		return const_reverse_iterator (this);
	}

	constexpr const_reverse_iterator
	crbegin (void) const
	{
		return const_reverse_iterator (prev);
	}

	constexpr const_reverse_iterator
	crend (void) const
	{
		return const_reverse_iterator (this);
	}
};

template<class T, ListHead T::*F>
struct TypedList {
public:
	ListHead m_list;

	constexpr
	TypedList (void)
		: m_list { &m_list, &m_list }
	{}

	constexpr void
	init (void)
	{
		m_list.init ();
	}

	constexpr void
	push_front (T *value)
	{
		m_list.push_front (&(value->*F));
	}

	constexpr void
	push_back (T *value)
	{
		m_list.push_back (&(value->*F));
	}

	constexpr bool
	empty (void) const
	{
		return m_list.empty ();
	}

	static inline T *
	container_of (ListHead *node)
	{
		return ::container_of<T, ListHead> (F, node);
	}

	static inline const T *
	const_container_of (const ListHead *node)
	{
		return ::container_of<const T, const ListHead> (F, node);
	}

	struct iterator : ListHead::iterator {
		T *
		operator * (void) const
		{
			return container_of (ListHead::iterator::operator * ());
		}

		T *
		operator-> (void) const
		{
			return container_of (ListHead::iterator::operator * ());
		}
	};

	struct const_iterator : ListHead::const_iterator {
		const T *
		operator * (void) const
		{
			return const_container_of (ListHead::const_iterator::operator * ());
		}

		const T *
		operator-> (void) const
		{
			return container_of (ListHead::const_iterator::operator * ());
		}
	};

	struct reverse_iterator : ListHead::reverse_iterator {
		T *
		operator * (void) const
		{
			return container_of (ListHead::reverse_iterator::operator * ());
		}

		T *
		operator-> (void) const
		{
			return container_of (ListHead::reverse_iterator::operator * ());
		}
	};

	struct const_reverse_iterator : ListHead::const_reverse_iterator {
		const T *
		operator * (void) const
		{
			return const_container_of (ListHead::const_reverse_iterator::operator * ());
		}

		const T *
		operator-> (void) const
		{
			return container_of (ListHead::const_reverse_iterator::operator * ());
		}
	};

	constexpr iterator
	begin (void)
	{
		return iterator (m_list.next);
	}

	constexpr iterator
	end (void)
	{
		return iterator (&m_list);
	}

	constexpr const_iterator
	begin (void) const
	{
		return const_iterator (m_list.next);
	}

	constexpr const_iterator
	end (void) const
	{
		return const_iterator (&m_list);
	}

	constexpr const_iterator
	cbegin (void) const
	{
		return const_iterator (m_list.next);
	}

	constexpr const_iterator
	cend (void) const
	{
		return const_iterator (&m_list);
	}

	constexpr reverse_iterator
	rbegin (void)
	{
		return reverse_iterator (m_list.prev);
	}

	constexpr reverse_iterator
	rend (void)
	{
		return reverse_iterator (&m_list);
	}

	constexpr const_reverse_iterator
	rbegin (void) const
	{
		return const_reverse_iterator (m_list.prev);
	}

	constexpr const_reverse_iterator
	rend (void) const
	{
		return const_reverse_iterator (&m_list);
	}

	constexpr const_reverse_iterator
	crbegin (void) const
	{
		return const_reverse_iterator (m_list.prev);
	}

	constexpr const_reverse_iterator
	crend (void) const
	{
		return const_reverse_iterator (&m_list);
	}

	T *
	pop_front (void)
	{
		iterator it = begin ();
		(&((*it)->*F))->remove ();
		return *it;
	}

	T *
	pop_back (void)
	{
		reverse_iterator it = rbegin ();
		(&((*it)->*F))->remove ();
		return *it;
	}
};

} /** namespace dsl  */
