/**
 * Metaprogramming basics.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

namespace dsl {

template<class T, T V>
struct integral_constant {
	using type = integral_constant;
	using value_type = T;
	static constexpr value_type value = V;
	constexpr operator value_type () const noexcept { return value; }
	constexpr value_type operator() (void) const noexcept { return value; }
};

template<bool B> using bool_constant = integral_constant<bool, B>;

typedef bool_constant<true> true_type;
typedef bool_constant<false> false_type;

template<class T, class U> struct is_same : false_type {};
template<class T> struct is_same<T, T> : true_type {};
template<class T, class U> constexpr bool is_same_v = is_same<T, U>::value;

} /** namespace dsl  */
