/**
 * IRQL management.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#include <stdint.h>

typedef unsigned int irql_t;

enum : irql_t {
	IRQL_NORMAL,
	IRQL_DISPATCH,
	IRQL_HIGH
};

enum : uint8_t {
	__IRQL_NONE_PENDING	= 0x80U
};

static inline uint8_t
__read_irql_dispatch (void)
{
	uint8_t value;
	asm volatile ("movb %%gs:13, %0" : "=r" (value) :: "memory");
	return value;
}

static inline uint8_t
__read_irql_high (void)
{
	uint8_t value;
	asm volatile ("movb %%gs:14, %0" : "=r" (value) :: "memory");
	return value;
}

static inline void
__write_irql_dispatch (uint8_t value)
{
	asm volatile ("movb %0, %%gs:13" :: "Nr" (value) : "memory");
}

static inline void
__write_irql_high (uint8_t value)
{
	asm volatile ("movb %0, %%gs:14" :: "Nr" (value) : "memory");
}

static inline void
__raise_irql_dispatch (void)
{
	asm volatile ("incb %%gs:13" ::: "cc");
}

static inline void
__raise_irql_high (void)
{
	asm volatile ("incb %%gs:14" ::: "cc");
}

static inline bool
__lower_irql_dispatch (void)
{
	bool zf;
	asm volatile ("decb %%gs:13" : "=@cce" (zf) :: "cc");
	return zf;
}

static inline bool
__lower_irql_high (void)
{
	bool zf;
	asm volatile ("decb %%gs:14" : "=@cce" (zf) :: "cc");
	return zf;
}

extern void
__pending_dpcs (void);

extern void
__pending_high (void);

static inline void
disable_dpc (void)
{
	__raise_irql_dispatch ();
}

static inline void
enable_dpc (void)
{
	if (__lower_irql_dispatch ())
		__pending_dpcs ();
}

static inline void
disable_irq (void)
{
	__raise_irql_high ();
}

static inline void
enable_irq (void)
{
	if (__lower_irql_high ())
		__pending_high ();
}

static inline void
irql_set_pending_dpc (void)
{
	__write_irql_dispatch (__read_irql_dispatch () & ~__IRQL_NONE_PENDING);
}

extern void
irql_begin_irq_from_user (void);

/** Returns true if IRQ handling can proceed, false if it should be deferred  */
extern bool
irql_begin_irq_from_kernel (int irq);

extern void
irql_leave_irq (void);

static inline void
wait_for_interrupt (void)
{
	asm volatile ("nop; hlt");
}
