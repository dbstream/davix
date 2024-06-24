/**
 * Standard integer types.
 * Copyright (C) 2024  dbstream
 */
#ifndef _DAVIX_STDINT_H
#define _DAVIX_STDINT_H 1

#ifndef __ASSEMBLER__

#define INT8_TYPE char
#define INT16_TYPE short
#define INT32_TYPE int
#if __SIZEOF_LONG__ == 8
#define INT64_TYPE long
#define INT64_C(n) _CONST(n, L)
#define UINT64_C(n) _CONST(n, UL)
#else
#define INT64_TYPE long long
#define INT64_C(n) _CONST(n, LL)
#define UINT64_C(n) _CONST(n, ULL)
#endif

_Static_assert (sizeof(INT8_TYPE) == 1, "sizeof(INT8_TYPE)");
_Static_assert (sizeof(INT16_TYPE) == 2, "sizeof(INT16_TYPE)");
_Static_assert (sizeof(INT32_TYPE) == 4, "sizeof(INT32_TYPE)");
_Static_assert (sizeof(INT64_TYPE) == 8, "sizeof(INT64_TYPE)");
_Static_assert (sizeof(long) == sizeof(void *), "sizeof(long)");

typedef signed INT8_TYPE	int8_t;
typedef signed INT16_TYPE	int16_t;
typedef signed INT32_TYPE	int32_t;
typedef signed INT64_TYPE	int64_t;
typedef unsigned INT8_TYPE	uint8_t;
typedef unsigned INT16_TYPE	uint16_t;
typedef unsigned INT32_TYPE	uint32_t;
typedef unsigned INT64_TYPE	uint64_t;

/* Define aliases that are used to clarify endianness in struct definitions. */
typedef uint16_t le16_t;
typedef uint32_t le32_t;
typedef uint64_t le64_t;
typedef uint16_t be16_t;
typedef uint32_t be32_t;
typedef uint64_t be64_t;

#endif

#if __SIZEOF_LONG__ == 8
#define MACHINE_BITS	64
#else
#define MACHINE_BITS	32
#endif

#ifdef __ASSEMBLER__
#define _CONST(n, suffix) n
#else
#define _CONST(n, suffix) n##suffix
#endif

#define _U(n) _CONST(n, U)
#define _L(n) _CONST(n, L)
#define _UL(n) _CONST(n, UL)

#define INT8_C(n)	n
#define INT16_C(n)	n
#define INT32_C(n)	n
#define UINT8_C(n)	n
#define UINT16_C(n)	n
#define UINT32_C(n)	_U(n)

#define INT8_MAX	INT8_C(0x7f)
#define INT16_MAX	INT16_C(0x7fff)
#define INT32_MAX	INT32_C(0x7fffffff)
#define INT64_MAX	INT64_C(0x7fffffffffffffff)
#define UINT8_MAX	UINT8_C(0xff)
#define UINT16_MAX	UINT16_C(0xffff)
#define UINT32_MAX	UINT32_C(0xffffffff)
#define UINT64_MAX	UINT64_C(0xffffffffffffffff)

#endif /* _DAVIX_STDINT_H */
