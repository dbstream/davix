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

#define bool _Bool

#define NULL ((void *) 0)

#define auto __auto_type

#define packed __attribute__((packed))
#define section(name) __attribute__((section(name)))

#define alignas _Alignas

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

#define warn_unused __attribute__((warn_unused_result))

unsigned long strlen(const char *s);
int strcmp(const char *a, const char *b);
int strncmp(const char *a, const char *b, unsigned long n);
char *strcpy(char *dst, const char *src);
char *strncpy(char *dst, const char *src, unsigned long n);
char *strcat(char *dst, const char *src);
char *strncat(char *dst, const char *src, unsigned long n);
int memcmp(const void *mem1, const void *m2, unsigned long n);
void *memcpy(void *dst, const void *src, unsigned long n);
void *memset(void *dst, int value, unsigned long n);
void *memmove(void *dst, const void *src, unsigned long n);

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

struct avlnode {
	struct avlnode *left, *right;
	struct avlnode *parent;
	int height;
};

struct avltree {
	struct avlnode *root;
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
	__aligndown_x - (__aligndown_x % __aligndown_align); \
})

#define align_up(x, align) ({ \
	auto __alignup_x = (x); \
	auto __alignup_align = (align); \
	align_down(__alignup_x + __alignup_align - 1, __alignup_align); \
})

typedef unsigned long bitwise alloc_flags_t;

#define __ALLOC_BIT(idx) ((force alloc_flags_t) (1UL << idx))

#define __ALLOC_ZERO __ALLOC_BIT(0)

#define __ALLOC_ARCH_SHIFT 1
#include <asm/arch_alloc_flags.h>

#define ALLOC_KERNEL (0)

#define THIS_ADDR ({ \
	__label__ __this_addr; \
	__this_addr: (unsigned long) &&__this_addr; \
})

#define RET_ADDR __builtin_extract_return_addr(__builtin_return_address(0))

#endif /* __KERNEL__ */

static_assert(sizeof(__i8) == 1, "sizeof(i8)");
static_assert(sizeof(__i16) == 2, "sizeof(i16)");
static_assert(sizeof(__i32) == 4, "sizeof(i32)");
static_assert(sizeof(__i64) == 8, "sizeof(i64)");
static_assert(sizeof(__u8) == 1, "sizeof(u8)");
static_assert(sizeof(__u16) == 2, "sizeof(u16)");
static_assert(sizeof(__u32) == 4, "sizeof(u32)");
static_assert(sizeof(__u64) == 8, "sizeof(u64)");

static_assert(sizeof(int) == sizeof(i32), "sizeof(int)");
static_assert(sizeof(long) == sizeof(void *), "sizeof(long)");

#endif /* __DAVIX_TYPES_H */
