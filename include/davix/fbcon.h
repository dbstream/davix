/**
 * Framebuffer console driver - public interface
 * Copyright (C) 2025-present  dbstream
 */
#ifndef _DAVIX_FBCON_H
#define _DAVIX_FBCON_H 1

#include <davix/stdint.h>

struct fbcon;

struct fbcon_format_info {
	uint8_t bpp;		/* bits per pixel  */
	uint8_t red_offset;
	uint8_t green_offset;
	uint8_t blue_offset;
	uint32_t red_mask;
	uint32_t green_mask;
	uint32_t blue_mask;
};

/**
 * Register a framebuffer with fbcon.
 *
 * @width	Framebuffer width in pixels.
 * @height	Framebuffer height in pixels.
 * @pitch	Bytes per row.
 * @fmt		Framebuffer format.
 * @front	Frontbuffer.  This is never read from.
 * @back	Backbuffer.  If this is NULL, fbcon will allocate a backbuffer
 *		itself.
 *
 * Returns a pointer to a struct fbcon that can be used with fbcon_remove(), or
 * NULL on failure.
 */
extern struct fbcon *
fbcon_register_framebuffer (uint32_t width, uint32_t height, uint32_t pitch,
		const struct fbcon_format_info *fmt, void *front, void *back);

/**
 * Remove a framebuffer device with fbcon.
 */
extern void
fbcon_remove (struct fbcon *con);

#endif /* _DAVIX_FBCON_H  */

