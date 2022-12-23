/* SPDX-License-Identifier: MIT */
#ifndef __DAVIX_TYPES_H
#define __DAVIX_TYPES_H

/*
 * What goes in this file and what gets its own header file?
 *
 * In this file, we put definitions for trivial types like `uXX`. We also define
 * some aliases for compiler attributes (and aliases for `sparse` if it is
 * running).
 *
 * We also define the value NULL, and the functions mem* and str*, because they
 * are used very commonly.
 *
 * Non-trivial, but still quite simple, data-types like `struct list` can be
 * placed in this file, but please give any helper functions their separate
 * header, unless they're *very* simple (max 2LOC).
 *
 * Also, remember to put everything inside the `#ifdef __KERNEL__`, except for
 * the data types defined below.
 */

typedef __INT8_TYPE__ __i8;
typedef __INT16_TYPE__ __i16;
typedef __INT32_TYPE__ __i32;
typedef __INT64_TYPE__ __i64;

typedef __UINT8_TYPE__ __u8;
typedef __UINT16_TYPE__ __u16;
typedef __UINT32_TYPE__ __u32;
typedef __UINT64_TYPE__ __u64;

#ifdef __KERNEL__

typedef __i8 i8;
typedef __i16 i16;
typedef __i32 i32;
typedef __i64 i64;

typedef __u8 u8;
typedef __u16 u16;
typedef __u32 u32;
typedef __u64 u64;

#define NULL ((void *) 0)

#define auto __auto_type

#define packed __attribute__((packed))
#define section(name) __attribute__((section(name)))

#ifdef __CHECKER__
#define force __attribute__((force))
#define bitwise __attribute__((bitwise))
#define noderef __attribute__((noderef))
#define nocast __attribute___((nocast))
#define acquires(lock) __attribute__((context(lock, 0, 1)))
#define releases(lock) __attribute__((context(lock, 1, 0)))
#define must_hold(lock) __attribute__((context(lock, 1, 1)))
#define designated_init __attribute__((designated_init))
#else
#define force
#define bitwise
#define noderef
#define nocast
#define acquires(lock)
#define releases(lock)
#define must_hold(lock)
#define designated_init
#endif

#define strlen __builtin_strlen
#define strcmp __builtin_strcmp
#define strncmp __builtin_strncmp
#define strcpy __builtin_strcpy
#define strncpy __builtin_strncpy
#define strcat __builtin_strcat
#define strncat __builtin_strncat
#define memcmp __builtin_memcmp
#define memcpy __builtin_memcpy
#define memset __builtin_memset
#define memmove __builtin_memmove

typedef __builtin_va_list va_list;
#define va_start __builtin_va_start
#define va_end __builtin_va_end
#define va_arg __builtin_va_arg
#define va_copy __builtin_va_copy

#define offsetof __builtin_offsetof
#define static_assert _Static_assert

#define container_of(ob, type, name) \
	((type *) ((unsigned long) (ob) - offsetof(type, name)))

struct list {
	struct list *next, *prev;
};

#define swap(a, b) ({ \
	auto __tmp = b; \
	b = a; \
	a = __tmp; \
})

#define max(a, b) ({ \
	auto __a = (a); \
	auto __b = (b); \
	__a > __b ? __a : __b; \
})

#define min(a, b) ({ \
	auto __a = (a); \
	auto __b = (b); \
	__a < __b ? __a : __b; \
})

#define align_down(x, align) ({ \
	auto __aligndown_x = (x); \
	auto __aligndown_align = (align); \
	__aligndown_x - (__aligndown_x % align); \
})

#define align_up(x, align) ({ \
	auto __alignup_x = (x); \
	auto __alignup_align = (align); \
	align_down(__alignup_x + __alignup_align - 1, __alignup_align); \
})

#endif /* __KERNEL__ */
#endif /* __DAVIX_TYPES_H */
