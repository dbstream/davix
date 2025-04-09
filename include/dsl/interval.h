/**
 * first_last and start_end.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#include "integral.h"

namespace dsl {

template<Integral T> struct first_last;
template<Integral T> struct start_end;

template<Integral T>
struct first_last {
	typedef first_last<T> first_last_type;
	typedef start_end<T> start_end_type;
	constexpr static T one = static_cast<T> (1);
private:
	T m_first, m_last;
public:
	constexpr inline
	first_last (void) = default;

	constexpr inline
	first_last (T first, T last)
		: m_first (first), m_last (last)
	{}

	constexpr inline T
	first (void) const
	{
		return m_first;
	}

	constexpr inline T
	last (void) const
	{
		return m_last;
	}

	constexpr inline T
	start (void) const
	{
		return m_first;
	}

	constexpr inline T
	end (void) const
	{
		return m_last + one;
	}

	constexpr inline operator
	start_end_type (void) const
	{
		return start_end_type (m_first, m_last + one);
	}
};

template<Integral T>
struct start_end {
	typedef first_last<T> first_last_type;
	typedef start_end<T> start_end_type;
	constexpr static T one = static_cast<T> (1);
private:
	T m_start, m_end;
public:
	constexpr inline
	start_end (void) = default;

	constexpr inline
	start_end (T start, T end)
		: m_start (start), m_end (end)
	{}

	constexpr inline T
	first (void) const
	{
		return m_start;
	}

	constexpr inline T
	last (void) const
	{
		return m_end - one;
	}

	constexpr inline T
	start (void) const
	{
		return m_start;
	}

	constexpr inline T
	end (void) const
	{
		return m_end;
	}

	constexpr inline operator
	first_last_type (void) const
	{
		return first_last_type (m_start, m_end - one);
	}
};

} /** namespace dsl  */
