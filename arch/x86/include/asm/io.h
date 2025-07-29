// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/asm/io.h
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

#include <stdint.h>

static inline uint8_t io_inb(uint16_t port)
{
	uint8_t data;
	asm volatile("inb %1, %0" : "=a"(data) : "Nd"(port) : "memory");
	return data;
}

static inline uint16_t io_inw(uint16_t port)
{
	uint16_t data;
	asm volatile("inw %1, %0" : "=a"(data) : "Nd"(port) : "memory");
	return data;
}

static inline uint32_t io_inl(uint16_t port)
{
	uint32_t data;
	asm volatile("inl %1, %0" : "=a"(data) : "Nd"(port) : "memory");
	return data;
}

static inline void io_outb(uint16_t port, uint8_t data)
{
	asm volatile("outb %0, %1" :: "a"(data), "Nd"(port) : "memory");
}

static inline void io_outw(uint16_t port, uint16_t data)
{
	asm volatile("outw %0, %1" :: "a"(data), "Nd"(port) : "memory");
}

static inline void io_outl(uint16_t port, uint32_t data)
{
	asm volatile("outl %0, %1" :: "a"(data), "Nd"(port) : "memory");
}

static inline void io_outsb(uint16_t port, const uint8_t *data, unsigned long num)
{
	asm volatile("rep outsb" :: "S"(data), "d"(port), "c"(num) : "memory");
}

