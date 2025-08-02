// SPDX-License-Identifier: GPL-3.0 OR BSD-3-Clause
/*
 * File: include/container_of.h
 * offset_of and container_of implementations.
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

template<class T, class F>
static inline unsigned long offset_of(F T::*member)
{
	return __builtin_bit_cast(unsigned long, member);
}

template<class T, class F>
static inline T *container_of(F T::*member, F *ptr)
{
	return (T *) ((unsigned long) ptr - offset_of<T, F>(member));
}

