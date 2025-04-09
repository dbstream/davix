/**
 * Promotion rules for mixed integral types.
 * Copyright (C) 2025-present  dbstream
 *
 * We implement the following semantics:
 * - if sizeof(A) > sizeof(B), use A.
 * - otherwise, if sizeof(B) > sizeof(A), use B.
 * - otherwise, if A is an unsigned integer type, use A.
 * - otherwise, use B.
 */
#pragma once

#include "integral.h"

namespace dsl {

namespace detail {

template<Integral A, Integral B, bool = (sizeof(A) > sizeof(B))
		|| (sizeof(A) == sizeof(B) && is_unsigned_v<A>)>
struct _promote {
	typedef A type;
};
	
template<Integral A, Integral B>
struct _promote<A, B, false> {
	typedef B type;
};

} /** namespace detail  */

template<Integral A, Integral B> struct promote : detail::_promote<A, B> {};

} /** namespace dsl  */
