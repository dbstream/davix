/**
 * Doubly-linked list.
 * Copyright (C) 2025-present  dbstream
 *
 * This file provides an implementation of a doubly-linked list, suitable for
 * use in hash maps.  It differs from dsl::List in that the 'list' itself only
 * takes up one word (a pointer to the first element; thus, it is not possible
 * to quickly access the last element of a HList).
 */
#pragma once

#include <container_of.h>

namespace dsl {

struct HListHead {
	HListHead *next, **link;

	constexpr void
	remove (void)
	{
		if (next)
			next->link = link;
		*link = next;

		/** poison next and link */
		next = (HListHead *) 0xdeadbeefUL;
		link = (HListHead **) 0xcafebabeUL;
	}
};

struct HList {
	HListHead *head = nullptr;

	constexpr void
	init (void)
	{
		head = nullptr;
	}

	constexpr void
	push (HListHead *entry)
	{
		entry->next = head;
		entry->link = &head;
		if (head)
			head->link = &entry->next;
		head = entry;
	}

	constexpr bool
	empty (void) const
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

		/** NB: postfix increment  */
		constexpr _iterator_impl<T>
		operator++ (int dummy)
		{
			(void) dummy;
			T *old = m_current;
			m_current = m_current->next;
			return { old };
		}
	};

	typedef _iterator_impl<HListHead> iterator;
	typedef _iterator_impl<HListHead> const_iterator;

	constexpr iterator
	begin (void)
	{
		return iterator (head);
	}

	constexpr iterator
	end (void)
	{
		return iterator (nullptr);
	}

	constexpr const_iterator
	begin (void) const
	{
		return const_iterator (head);
	}

	constexpr const_iterator
	end (void) const
	{
		return const_iterator (nullptr);
	}

	constexpr const_iterator
	cbegin (void) const
	{
		return const_iterator (head);
	}

	constexpr const_iterator
	cend (void) const
	{
		return const_iterator (nullptr);
	}
};

template<class T, HListHead T::*F>
struct TypedHList {
public:
	HList m_list;

	constexpr void
	init (void)
	{
		m_list.init ();
	}

	constexpr void
	push (T *entry)
	{
		m_list.push (&(entry->*F));
	}

	T *
	pop (void)
	{
		T *entry = *begin ();
		(entry->*F).remove ();
		return entry;
	}

	constexpr bool
	empty (void) const
	{
		return m_list.empty ();
	}

	static inline T *
	container_of (HListHead *node)
	{
		return ::container_of<T, HListHead> (F, node);
	}

	static inline const T *
	const_container_of (const HListHead *node)
	{
		return ::container_of<const T, const HListHead> (F, node);
	}

	struct iterator : HList::iterator {
		T *
		operator * (void) const
		{
			return container_of (HList::iterator::operator * ());
		}

		T *
		operator-> (void) const
		{
			return container_of (HList::iterator::operator * ());
		}
	};

	struct const_iterator : HList::const_iterator {
		const T *
		operator * (void) const
		{
			return const_container_of (HList::const_iterator::operator * ());
		}

		const T *
		operator-> (void) const
		{
			return const_container_of (HList::const_iterator::operator * ());
		}
	};

	constexpr iterator
	begin (void)
	{
		return iterator (m_list.head);
	}

	constexpr iterator
	end (void)
	{
		return iterator (nullptr);
	}

	constexpr const_iterator
	begin (void) const
	{
		return const_iterator (m_list.head);
	}

	constexpr const_iterator
	end (void) const
	{
		return const_iterator (nullptr);
	}

	constexpr const_iterator
	cbegin (void) const
	{
		return const_iterator (m_list.head);
	}

	constexpr const_iterator
	cend (void) const
	{
		return const_iterator (nullptr);
	}
};

} /** namespace dsl  */
