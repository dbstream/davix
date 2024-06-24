/**
 * Instructions for accessing the port-mapped IO space.
 * Copyright (C) 2024  dbstream
 */
#ifndef _ASM_IO_H
#define _ASM_IO_H 1

#include <davix/stdint.h>

#define build_io_rw(bits, insn_suffix)				\
static inline void						\
io_out##insn_suffix (uint16_t port, uint##bits##_t value)	\
{								\
	asm volatile (						\
		"out" #insn_suffix " %0, %1"			\
		:: "a" (value), "Nd" (port) : "memory"		\
	);							\
}								\
								\
static inline uint##bits##_t					\
io_in##insn_suffix (uint16_t port)				\
{								\
	uint##bits##_t value;					\
	asm volatile (						\
		"in" #insn_suffix " %1, %0"			\
		: "=a" (value) : "Nd" (port) : "memory"		\
	);							\
	return value;						\
}

build_io_rw(8, b)
build_io_rw(16, w)
build_io_rw(32, l)

#undef build_io_rw

#endif /* _ASM_IO_H  */
