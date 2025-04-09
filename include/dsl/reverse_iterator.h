/**
 * Utilities for creating reverse iterators.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

namespace dsl {

template<class It>
struct reverse_iterator : It {
	constexpr reverse_iterator &
	operator++ (void)
	{
		It::operator-- ();
		return *this;
	}

	constexpr reverse_iterator &
	operator-- (void)
	{
		It::operator++ ();
		return *this;
	}

	/** NB: postfix increment operator  */
	constexpr reverse_iterator
	operator++ (int dummy)
	{
		(void) dummy;
		reverse_iterator old = *this;
		It::operator-- ();
		return old;
	}

	/** NB: postfix decrement operator  */
	constexpr reverse_iterator
	operator-- (int dummy)
	{
		(void) dummy;
		reverse_iterator old = *this;
		It::operator++ ();
		return old;
	}
};

} /** namespace dsl  */
