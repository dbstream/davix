/**
 * Framebuffer console driver - public interface
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <davix/console.h>

struct fbcon_format {
	uint8_t bpp;		/** _bits_  per pixel */
	uint8_t red_offset;
	uint8_t green_offset;
	uint8_t blue_offset;
	uint8_t red_bits;
	uint8_t green_bits;
	uint8_t blue_bits;
};

struct fbcon {
	/**
	 * These struct members are internal to fbcon.cc  - they must be
	 * declared for users of the API who preallocate struct fbcon.
	 */
	Console con;
	fbcon_format fmt;
	uint32_t width, height, pitch;
	size_t nbytes;
	void *fbmem, *backbuf;
	unsigned int flags;
	uint32_t cx, cy;
	uint32_t c_background;
	uint32_t c_info;
	uint32_t c_notice;
	uint32_t c_warn;
	uint32_t c_err;
	uint32_t c_msgtime;
};

/**
 * fbcon_add_framebuffer - register a framebuffer with fbcon.
 * @fbcon: preallocated struct fbcon
 * @width: framebuffer width in pixels
 * @height: framebuffer height in pixels
 * @pitch: bytes per row
 * @format: color format
 * @fbmem: framebuffer memory; never read from
 * @backbuf: backbuffer
 * Returns a pointer to a struct fbcon.
 */
struct fbcon *
fbcon_add_framebuffer (struct fbcon *fbcon,
		uint32_t width, uint32_t height, uint32_t pitch,
		const struct fbcon_format *format, void *fbmem, void *backbuf);
