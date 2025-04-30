/**
 * Framebuffer console driver
 * Copyright (C) 2025-present  dbstream
 */
#include <string.h>
#include <davix/fbcon.h>
#include <davix/kmalloc.h>
#include <davix/printk.h>

/**
 * get_color - convert from RGB to framebuffer color
 * @fmt: framebuffer format descriptor
 * @r: red [0 - 255]
 * @g: green [0 - 255]
 * @b: blue [0 - 255]
 * Returns the resulting color as a uint32_t suitable for putpixel.
 */
static uint32_t
get_color (const fbcon_format *fmt, uint32_t r, uint32_t g, uint32_t b)
{
	if (r > 255)
		r = 255;
	if (g > 255)
		g = 255;
	if (b > 255)
		b = 255;

	if (fmt->red_bits > 8)
		r <<= fmt->red_bits - 8;
	else if (fmt->red_bits < 8)
		r >>= 8 - fmt->red_bits;
	if (fmt->green_bits > 8)
		g <<= fmt->green_bits - 8;
	else if (fmt->green_bits < 8)
		g >>= 8 - fmt->green_bits;
	if (fmt->blue_bits > 8)
		b <<= fmt->blue_bits - 8;
	else if (fmt->blue_bits < 8)
		b >>= 8 - fmt->blue_bits;

	r <<= fmt->red_offset;
	g <<= fmt->green_offset;
	b <<= fmt->blue_offset;
	return r | g | b;
}

enum fbcon_flags {
	FBC_OWN_STRUCT = 1U << 0,	/** we allocated the struct fbcon  */
	FBC_OWN_BACKBUFFER = 1U << 1	/** we allocated the backbuffer  */
};

/**
 * release_fbcon - free allocated resources for a fbcon, incl. the fbcon itself.
 * @fbcon: struct fbcon
 */
static void
release_fbcon (struct fbcon *fbcon)
{
	if (fbcon->flags & FBC_OWN_BACKBUFFER)
		kfree (fbcon->backbuf);
	if (fbcon->flags & FBC_OWN_STRUCT)
		kfree (fbcon);
}

/**
 * validate_format - sanity check the supplied framebuffer format and extent.
 * @width: width
 * @height: height
 * @pitch: pitch
 * @fmt: struct fbcon_format
 * Returns true if the supplied format is useable with fbcon.
 */
static bool
validate_format (uint32_t width, uint32_t height, uint32_t pitch,
		const struct fbcon_format *fmt)
{
	uint32_t bytes_per_pixel;
	switch (fmt->bpp) {
	case 8:
		bytes_per_pixel = 1;
		break;
	case 16:
		bytes_per_pixel = 2;
		break;
	case 24:
		bytes_per_pixel = 3;
		break;
	case 32:
		bytes_per_pixel = 4;
		break;
	default:
		printk (PR_WARN "fbcon: %d bits per pixel is not supported\n",
				fmt->bpp);
		return false;
	}

	if (width < 20 || height < 20) {
		printk (PR_WARN "fbcon: extent %ux%u is too small\n",
				width, height);
		return false;
	}

	uint64_t tmp = width * bytes_per_pixel;
	if (pitch < tmp) {
		printk (PR_WARN "fbcon: pitch %u is smaller than width * bytes_per_pixel\n", pitch);
		return false;
	}

	if ((fmt->bpp == 16 || fmt->bpp == 32) && (pitch & (bytes_per_pixel - 1))) {
		printk (PR_WARN "fbcon: pitch %u is not well-aligned for %u bits per pixel\n",
				pitch, fmt->bpp);
		return false;
	}

	tmp = height * pitch;
	if (tmp > uint32_t(-1)) {
		printk (PR_WARN "fbcon: framebuffer is too big for uint32_t\n");
		return false;
	}

	if (get_color (fmt, 0, 0, 0) == get_color (fmt, 128, 128, 128)) {
		printk (PR_WARN "fbcon: color format is not sane\n");
		return false;
	}

	return true;
}

/**
 * fbcon_flush - flush the output.
 * @fbcon: struct fbcon
 */
static void
fbcon_flush (struct fbcon *fbcon)
{
	memcpy (fbcon->fbmem, fbcon->backbuf, fbcon->nbytes);
	asm volatile ("" : "+r" (fbcon->fbmem) :: "memory");
}

static inline uint8_t *
get_row (struct fbcon *fbcon, uint32_t row)
{
	return (uint8_t *) fbcon->backbuf + row * fbcon->pitch;
}

static inline uint8_t *
get_pixel (struct fbcon *fbcon, uint32_t row, uint32_t column)
{
	return get_row (fbcon, row) + (fbcon->fmt.bpp / 8) * column;
}

static inline void
putpixel8 (uint8_t *&ptr, uint32_t x)
{
	*(ptr++) = x;
}

static inline void
putpixel16 (uint8_t *&ptr, uint32_t x)
{
	*(ptr++) = x;
	*(ptr++) = x >> 8;
}

static inline void
putpixel24 (uint8_t *&ptr, uint32_t x)
{
	*(ptr++) = x;
	*(ptr++) = x >> 8;
	*(ptr++) = x >> 16;
}

static inline void
putpixel32 (uint8_t *&ptr, uint32_t x)
{
	*(ptr++) = x;
	*(ptr++) = x >> 8;
	*(ptr++) = x >> 16;
	*(ptr++) = x >> 24;
}

/**
 * putpixel - put a pixel on the output
 * @ptr: output buffer pointer
 * @x: color, as given by get_color
 * @bpp: current bits per pixel
 * The output buffer pointer is incremented accordingly.
 */
static inline void
putpixel (uint8_t *&ptr, uint32_t x, uint8_t bpp)
{
	switch (bpp) {
	case 8:
		putpixel8 (ptr, x);
		return;
	case 16:
		putpixel16 (ptr, x);
		return;
	case 24:
		putpixel24 (ptr, x);
		return;
	case 32:
		putpixel32 (ptr, x);
		return;
	}
}

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
		const struct fbcon_format *fmt, void *fbmem, void *backbuf)
{
	printk ("fbcon_add_framebuffer\n");
	if (!validate_format (width, height, pitch, fmt))
		return nullptr;

	if (fbcon)
		fbcon->flags = 0;
	else {
		fbcon = (struct fbcon *) kmalloc (sizeof (*fbcon), ALLOC_KERNEL);
		if (!fbcon)
			return nullptr;

		fbcon->flags = FBC_OWN_STRUCT;
	}

	fbcon->fmt = *fmt;
	fbcon->width = width;
	fbcon->height = height;
	fbcon->pitch = pitch;
	fbcon->nbytes = height * pitch;
	if (!backbuf) {
		backbuf = kmalloc (fbcon->nbytes, ALLOC_KERNEL);
		if (!backbuf) {
			release_fbcon (fbcon);
			return nullptr;
		}

		fbcon->flags |= FBC_OWN_BACKBUFFER;
	}
	fbcon->backbuf = backbuf;

	asm ("" : "+r" (fbmem));
	fbcon->fbmem = fbmem;

	fbcon->cx = 1;
	fbcon->cy = 1;
	fbcon->c_background	= get_color (fmt,  20,  20,  20);
	fbcon->c_info		= get_color (fmt, 220, 220, 220);
	fbcon->c_notice		= get_color (fmt, 255, 255, 255);
	fbcon->c_warn		= get_color (fmt, 255, 128,   0);
	fbcon->c_err		= get_color (fmt, 255,   0,   0);
	fbcon->c_msgtime	= get_color (fmt,   0, 255,   0);

	uint32_t c = fbcon->c_background;
	uint32_t bpp = fbcon->fmt.bpp;
	for (uint32_t i = 0; i < fbcon->height; i++) {
		uint8_t *p = get_row (fbcon, i);
		for (uint32_t j = 0; j < fbcon->width; j++)
			putpixel (p, c, bpp);
	}
	fbcon_flush (fbcon);
	return fbcon;
}
