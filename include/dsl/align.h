/**
 * Integer and pointer alignment.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#include <stdint.h>
#include "integral.h"

namespace dsl {

template<Integral T, Integral A> T
align_up (T value, A alignment)
{
	typedef make_unsigned_t<T> T_unsigned;
	typedef make_unsigned_t<A> A_unsigned;

	A_unsigned a = static_cast<A_unsigned> (alignment);
	T_unsigned v = static_cast<T_unsigned> (value);

	if (a)
		a--;
	return static_cast<T> ((v + a) & ~a);
}

template<Integral T, Integral A> T
align_down (T value, A alignment)
{
	typedef make_unsigned_t<T> T_unsigned;
	typedef make_unsigned_t<A> A_unsigned;

	A_unsigned a = static_cast<A_unsigned> (alignment);
	T_unsigned v = static_cast<T_unsigned> (value);

	if (a)
		a--;
	return static_cast<T> (v & ~a);
}

template<class T, Integral A> T *
align_up_ptr (T *value, A alignment)
{
	typedef make_unsigned_t<A> A_unsigned;

	A_unsigned a = static_cast<A_unsigned> (alignment);
	uintptr_t v = reinterpret_cast<uintptr_t> (value);

	if (a)
		a--;
	return reinterpret_cast <T *> ((v + a) & ~a);
}

template<class T, Integral A> T *
align_down_ptr (T *value, A alignment)
{
	typedef make_unsigned_t<A> A_unsigned;

	A_unsigned a = static_cast<A_unsigned> (alignment);
	uintptr_t v = reinterpret_cast<uintptr_t> (value);

	if (a)
		a--;
	return reinterpret_cast <T *> (v & ~a);
}

} /** namespace dsl  */
