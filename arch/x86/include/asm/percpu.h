/**
 * percpu variables support.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#include <stdint.h>

namespace pcpu_detail {

extern uintptr_t offsets[];

template<class T>
static inline T
read__seg_gs [[gnu::always_inline]] (T *ptr)
{
	T value;
	if constexpr (sizeof (T) == 1) {
		asm volatile ("movb %%gs:%1, %0" : "=r" (value) : "m" (*ptr));
	} else if constexpr (sizeof (T) == 2) {
		asm volatile ("movw %%gs:%1, %0" : "=r" (value) : "m" (*ptr));
	} else if constexpr (sizeof (T) == 4) {
		asm volatile ("movl %%gs:%1, %0" : "=r" (value) : "m" (*ptr));
	} else if constexpr (sizeof (T) == 8) {
		asm volatile ("movq %%gs:%1, %0" : "=r" (value) : "m" (*ptr));
	} else {
		asm volatile ("addq %%gs:0, %0" : "+r" (ptr) :: "cc");
		value = *ptr;
	}
	return value;
}

template<class T>
static inline void
write__seg_gs [[gnu::always_inline]] (T *ptr, T value)
{
	if constexpr (sizeof (T) == 1) {
		asm volatile ("movb %0, %%gs:%1" :: "Nr" (value), "m" (*ptr));
	} else if constexpr (sizeof (T) == 2) {
		asm volatile ("movw %0, %%gs:%1" :: "Nr" (value), "m" (*ptr));
	} else if constexpr (sizeof (T) == 4) {
		asm volatile ("movl %0, %%gs:%1" :: "Nr" (value), "m" (*ptr));
	} else if constexpr (sizeof (T) == 8) {
		asm volatile ("movq %0, %%gs:%1" :: "Nr" (value), "m" (*ptr));
	} else {
		asm volatile ("addq %%gs:0, %0" : "+r" (ptr) :: "cc");
		*ptr = value;
	}
}

template<class T> using storage = T;

#define DEFINE_PERCPU(type, name) ::pcpu_detail::storage<type> name [[gnu::section (".percpu")]];

} /**  namespace percpu_detail  */

template<class T>
class percpu_ptr {
	T *m_ptr;
public:
	constexpr inline
	percpu_ptr (T &st)
		: m_ptr (&st)
	{}

	inline
	operator T * (void) const
	{
		T *ret = m_ptr;
		asm volatile ("addq %%gs:0, %0" : "+r" (ret) :: "cc");
		return ret;
	}

	inline T *
	on (unsigned int cpu) const
	{
		return reinterpret_cast<T *> (
			reinterpret_cast<uintptr_t> (
				m_ptr
			) + pcpu_detail::offsets[cpu]
		);
	}

	inline T
	read (void) const
	{
		return pcpu_detail::read__seg_gs<T> (m_ptr);
	}

	inline void
	write (T value) const
	{
		pcpu_detail::write__seg_gs<T> (m_ptr, value);
	}
};

template<class T>
static inline T
percpu_read (T &st)
{
	return percpu_ptr (st).read ();
}

template<class T>
static inline void
percpu_write (T &st, T value)
{
	percpu_ptr (st).write (value);
}

#define _PERCPU_CONSTRUCTOR(name)					\
asm (									\
	".pushsection \".pcpu_constructors\", \"a\", @progbits\n"	\
	".globl __p_" #name "\n"					\
	"__p_" #name ":\n"						\
	".quad " #name "\n"						\
	".popsection\n"							\
);									\
extern "C" void								\
name [[gnu::used]] (unsigned int cpu)

#define PERCPU_CONSTRUCTOR(name) _PERCPU_CONSTRUCTOR(__pcpu_constructor_ ## name)
