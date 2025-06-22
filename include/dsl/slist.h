/**
 * Singly-linked list.
 * Copyright (C) 2025-present  dbstream
 *
 * This file provides an implementation of a singly-linked list, suitable for
 * use in e.g. hash maps.
 */
#pragma once

#include <container_of.h>

namespace dsl {

struct SListHead {
	SListHead *next, **link;

	constexpr void
	remove (void)
	{
		if (next)
			next->link = link;
		*link = next;

		/** poison next and link */
		next = (SListHead *) 0xdeadbeefUL;
		link = (SListHead **) 0xcafebabeUL;
	}
};

struct SList {
	SListHead *head = nullptr;

	constexpr void
	init (void)
	{
		head = nullptr;
	}

	constexpr void
	push (SListHead *entry)
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

	typedef _iterator_impl<SListHead> iterator;
	typedef _iterator_impl<SListHead> const_iterator;

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

template<class T, SListHead T::*F>
struct TypedSList {
public:
	SList m_list;

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

	constexpr bool
	empty (void) const
	{
		return m_list.empty ();
	}

	static inline T *
	container_of (SListHead *node)
	{
		return ::container_of<T, SListHead> (F, node);
	}

	static inline const T *
	const_container_of (const SListHead *node)
	{
		return ::container_of<const T, const SListHead> (F, node);
	}

	struct iterator : SList::iterator {
		T *
		operator * (void) const
		{
			return container_of (SList::iterator::operator * ());
		}

		T *
		operator-> (void) const
		{
			return container_of (SList::iterator::operator * ());
		}
	};

	struct const_iterator : SList::const_iterator {
		const T *
		operator * (void) const
		{
			return const_container_of (SList::const_iterator::operator * ());
		}

		const T *
		operator-> (void) const
		{
			return const_container_of (SList::const_iterator::operator * ());
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
