/**
 * stdint.h freestanding header
 * Copyright (C) 2025-present  dbstream
 *
 * We extend this header to provide INTx_C, UINTx_C to assembler source.
 */
#pragma once

#ifdef __ASSEMBLER__
#	define INT8_C(x) x
#	define INT16_C(x) x
#	define INT32_C(x) x
#	define INT64_C(x) x
#	define UINT8_C(x) x
#	define UINT16_C(x) x
#	define UINT32_C(x) x
#	define UINT64_C(x) x
#else /** __ASSEMBLER__  */
	typedef __INT8_TYPE__ int8_t;
	typedef __INT16_TYPE__ int16_t;
	typedef __INT32_TYPE__ int32_t;
	typedef __INT64_TYPE__ int64_t;
	typedef __UINT8_TYPE__ uint8_t;
	typedef __UINT16_TYPE__ uint16_t;
	typedef __UINT32_TYPE__ uint32_t;
	typedef __UINT64_TYPE__ uint64_t;
	typedef __UINTPTR_TYPE__ uintptr_t;
	typedef __INTPTR_TYPE__ intptr_t;
	typedef __INTMAX_TYPE__ intmax_t;
	typedef __UINTMAX_TYPE__ uintmax_t;
#	ifdef __clang__
#		define __INTN_C_JOIN(a, b) ___INTN_C_JOIN(a, b)
#		define ___INTN_C_JOIN(a, b) a ## b
#		define __INT8_C(x) __INTN_C_JOIN(x, __INT8_C_SUFFIX__)
#		define __INT16_C(x) __INTN_C_JOIN(x, __INT16_C_SUFFIX__)
#		define __INT32_C(x) __INTN_C_JOIN(x, __INT32_C_SUFFIX__)
#		define __INT64_C(x) __INTN_C_JOIN(x, __INT64_C_SUFFIX__)
#		define __UINT8_C(x) __INTN_C_JOIN(x, __UINT8_C_SUFFIX__)
#		define __UINT16_C(x) __INTN_C_JOIN(x, __UINT16_C_SUFFIX__)
#		define __UINT32_C(x) __INTN_C_JOIN(x, __UINT32_C_SUFFIX__)
#		define __UINT64_C(x) __INTN_C_JOIN(x, __UINT64_C_SUFFIX__)
#	endif /** __clang__  */
#	define INT8_C(x) __INT8_C(x)
#	define INT16_C(x) __INT16_C(x)
#	define INT32_C(x) __INT32_C(x)
#	define INT64_C(x) __INT64_C(x)
#	define UINT8_C(x) __UINT8_C(x)
#	define UINT16_C(x) __UINT16_C(x)
#	define UINT32_C(x) __UINT32_C(x)
#	define UINT64_C(x) __UINT64_C(x)
#endif /** !__ASSEMBLER__  */
