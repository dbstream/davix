/**
 * Port-based input and output.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#include <stdint.h>

[[gnu::always_inline]] static inline uint8_t
io_inb (uint16_t port)
{
	uint8_t data;
	asm volatile ("inb %1, %0" : "=a" (data) : "Nd" (port) : "memory");
	return data;
}

[[gnu::always_inline]] static inline uint16_t
io_inw (uint16_t port)
{
	uint16_t data;
	asm volatile ("inw %1, %0" : "=a" (data) : "Nd" (port) : "memory");
	return data;
}

[[gnu::always_inline]] static inline uint32_t
io_inl (uint16_t port)
{
	uint32_t data;
	asm volatile ("inl %1, %0" : "=a" (data) : "Nd" (port) : "memory");
	return data;
}

[[gnu::always_inline]] static inline void
io_outb (uint16_t port, uint8_t data)
{
	asm volatile ("outb %0, %1" :: "a" (data), "Nd" (port) : "memory");
}

[[gnu::always_inline]] static inline void
io_outw (uint16_t port, uint16_t data)
{
	asm volatile ("outw %0, %1" :: "a" (data), "Nd" (port) : "memory");
}

[[gnu::always_inline]] static inline void
io_outl (uint16_t port, uint32_t data)
{
	asm volatile ("outl %0, %1" :: "a" (data), "Nd" (port) : "memory");
}
