/**
 * is_integral, is_(un)signed, and related concepts
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#include "cv.h"
#include "integral_constant.h"

namespace dsl {

template<class T>
struct is_integral : bool_constant<requires (T value, T *ptr, void (*pfn) (T))
{
	reinterpret_cast<T> (value);
	pfn (0);
	ptr + value;
}> {};

template<class T> constexpr bool is_integral_v = is_integral<T>::value;

namespace detail {

template<class T, bool = is_integral_v<T>>
struct _is_signed : bool_constant<T (-1) < T (0)> {};

template<class T>
struct _is_signed<T, false> : false_type {};

template<class T, bool = is_integral_v<T>>
struct _is_unsigned : bool_constant<T (0) < T (-1)> {};

template<class T>
struct _is_unsigned<T, false> : false_type {};

} /** namespace detail  */

template<class T>
struct is_signed : detail::_is_signed<T>::type {};

template<class T>
struct is_unsigned : detail::_is_unsigned<T>::type {};

template<class T> constexpr bool is_signed_v = is_signed<T>::value;
template<class T> constexpr bool is_unsigned_v = is_unsigned<T>::value;

namespace detail {

template<class T>
struct _make_signed_unsigned;

template<>
struct _make_signed_unsigned<char> {
	typedef signed char signed_type;
	typedef unsigned char unsigned_type;
};

template<>
struct _make_signed_unsigned<signed char> : _make_signed_unsigned<char> {};

template<>
struct _make_signed_unsigned<unsigned char> : _make_signed_unsigned<char> {};

template<>
struct _make_signed_unsigned<short> {
	typedef signed short signed_type;
	typedef unsigned short unsigned_type;
};

template<>
struct _make_signed_unsigned<unsigned short> : _make_signed_unsigned<short> {};

template<>
struct _make_signed_unsigned<int> {
	typedef signed int signed_type;
	typedef unsigned int unsigned_type;
};

template<>
struct _make_signed_unsigned<unsigned int> : _make_signed_unsigned<int> {};

template<>
struct _make_signed_unsigned<long> {
	typedef signed long signed_type;
	typedef unsigned long unsigned_type;
};

template<>
struct _make_signed_unsigned<unsigned long> : _make_signed_unsigned<long> {};

template<>
struct _make_signed_unsigned<long long> {
	typedef signed long long signed_type;
	typedef unsigned long long unsigned_type;
};

template<>
struct _make_signed_unsigned<unsigned long long> : _make_signed_unsigned<long long> {};

} /** namespace detail  */

template<class T>
struct make_signed {
	typedef copy_cv_qualifiers_t<T,
			typename detail::_make_signed_unsigned<remove_cv_t<T>>::signed_type> type;
};

template<class T>
struct make_unsigned {
	typedef copy_cv_qualifiers_t<T,
			typename detail::_make_signed_unsigned<remove_cv_t<T>>::unsigned_type> type;
};

template<class T> using make_signed_t = typename make_signed<T>::type;
template<class T> using make_unsigned_t = typename make_unsigned<T>::type;

template<class T> concept Integral = is_integral_v<T>;
template<class T> concept Signed = is_signed_v<T>;
template<class T> concept Unsigned = is_unsigned_v<T>;

} /** namespace dsl  */
