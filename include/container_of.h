/**
 * offset_of, container_of
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#include <stdint.h>

template<class T, class F>
static inline uintptr_t
offset_of (F T::*member)
{
	return __builtin_bit_cast (uintptr_t, member);
}

template<class T, class F>
static inline T *
container_of (F T::*member, F *ptr)
{
	return reinterpret_cast<T *> (
		reinterpret_cast <uintptr_t> (
			ptr
		) - offset_of<T, F> (member)
	);
}
