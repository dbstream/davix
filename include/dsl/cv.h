/**
 * cv-qualifiers metaprogramming
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#include "integral_constant.h"

namespace dsl {

template<class T> struct is_const : false_type {};
template<class T> struct is_const<const T> : true_type {};
template<class T> struct is_volatile : false_type {};
template<class T> struct is_volatile<volatile T> : true_type {};
template<class T> constexpr bool is_const_v = is_const<T>::value;
template<class T> constexpr bool is_volatile_v = is_volatile<T>::value;

template<class T> struct remove_const { typedef T type; };
template<class T> struct remove_const<const T> { typedef T type; };
template<class T> struct remove_volatile { typedef T type; };
template<class T> struct remove_volatile<volatile T> { typedef T type; };
template<class T> using remove_const_t = typename remove_const<T>::type;
template<class T> using remove_volatile_t = typename remove_volatile<T>::type;

template<class T> struct remove_cv { typedef remove_const_t<remove_volatile_t<T>> type; };
template<class T> using remove_cv_t = typename remove_cv<T>::type;

template<class T> struct add_const { typedef const T type; };
template<class T> struct add_volatile {typedef volatile T type; };
template<class T> struct add_cv { typedef const volatile T type; };
template<class T> using add_const_t = typename add_const<T>::type;
template<class T> using add_volatile_t = typename add_volatile<T>::type;
template<class T> using add_cv_t = typename add_cv<T>::type;

namespace detail {

template<class T1, class T2, bool B> struct _cv_select;
template<class T1, class T2> struct _cv_select<T1, T2, false> { typedef T1 type; };
template<class T1, class T2> struct _cv_select<T1, T2, true> { typedef T2 type; };

template<class T, class U>
struct _copy_cv_qualifiers {
	typedef _cv_select<U, add_const_t<U>, is_const_v<T>>::type maybe_const_type;
	typedef _cv_select<U, add_volatile_t<U>, is_volatile_v<T>>::type maybe_volatile_type;

	typedef _cv_select<maybe_const_type, add_volatile_t<maybe_const_type>,
			is_volatile_v<T>>::type type;
};

} /** namespace detail  */

template<class T, class U>
struct copy_cv_qualifiers {
	using type = typename detail::_copy_cv_qualifiers<T, U>::type;
};

template<class T, class U> using copy_cv_qualifiers_t = typename copy_cv_qualifiers<T, U>::type;

}
