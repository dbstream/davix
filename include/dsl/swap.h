/**
 * swap()
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

namespace dsl {

template<class T>
constexpr static inline void
swap (T &a, T &b)
{
	T tmp = a;
	a = b;
	b = tmp;
}

} /** namespace dsl  */
