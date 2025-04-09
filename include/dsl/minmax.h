/**
 * min(), max(), and clamp().
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#include "integral.h"
#include "promote.h"

namespace dsl {

namespace detail {

template<Integral A, Integral B>
struct _minmax {
	using result_type = promote<A, B>::type;

	constexpr static inline result_type
	min (A a, B b)
	{
		result_type _a = static_cast<result_type> (a);
		result_type _b = static_cast<result_type> (b);
		return (_a < _b) ? _a : _b;
	}

	constexpr static inline result_type
	max (A a, B b)
	{
		result_type _a = static_cast<result_type> (a);
		result_type _b = static_cast<result_type> (b);
		return (_a > _b) ? _a : _b;
	}
};

} /** namespace detail  */

template<Integral A, Integral B>
constexpr static inline auto
min (A a, B b)
{
	return detail::_minmax<A, B>::min (a, b);
}

template<Integral A, Integral B>
constexpr static inline auto
max (A a, B b)
{
	return detail::_minmax<A, B>::max (a, b);
}

template<Integral F, Integral T, Integral C>
constexpr static inline auto
clamp (F floor, T value, C ceiling)
{
	return max (floor, min (value, ceiling));
}

} /** namespace dsl  */
