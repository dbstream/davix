/**
 * Framebuffer console driver
 * Copyright (C) 2025-present  dbstream
 */
#include <container_of.h>
#include <string.h>
#include <vsnprintf.h>
#include <davix/fbcon.h>
#include <davix/fbcon_internal.h>
#include <davix/kmalloc.h>
#include <davix/printk.h>

#include "font.inl"

static constexpr const uint8_t *default_font = VGA9_F16;

enum : uint32_t {
	FONT_WIDTH = 8,
	FONT_HEIGHT = 16,
	SYMBOL_WIDTH = 9,
	LINE_HEIGHT = 17
};

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
void
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
 * fbcon_scroll - scroll the framebuffer
 * @fbcon: struct fbcon
 * @scroll: number of lines to scroll by
 */
static void
fbcon_scroll (struct fbcon *fbcon, uint32_t scroll)
{
	uint8_t bpp = fbcon->fmt.bpp;
	uint32_t c = fbcon->c_background;
	size_t nbytes = fbcon->nbytes - scroll * fbcon->pitch;

	memmove (get_row (fbcon, 0), get_row (fbcon, scroll), nbytes);
	fbcon->cy -= scroll;

	for (uint32_t i = fbcon->height - scroll; i < fbcon->height; i++) {
		uint8_t *p = get_row (fbcon, i);
		for (uint32_t j = 0; j < fbcon->width; j++)
			putpixel (p, c, bpp);
	}
}

/**
 * fbcon_blit_character - draw a character to the framebuffer
 * @fbcon: struct fbcon
 * @c: character to draw
 * @x: top left x
 * @y: top left y
 * @fg: foreground color
 * @bg: background color
 */
static void
fbcon_blit_character (struct fbcon *fbcon, uint8_t c,
		uint32_t x, uint32_t y, uint32_t fg, uint32_t bg)
{
	uint8_t bpp = fbcon->fmt.bpp;
	for (uint32_t i = 0; i < FONT_HEIGHT; i++) {
		uint8_t data = default_font[FONT_HEIGHT * c + i];
		uint8_t mask = 0x80;
		uint8_t *p = get_pixel (fbcon, y + i, x);
		for (uint32_t j = 0; j < FONT_WIDTH; j++) {
			uint32_t x = (data & mask) ? fg : bg;
			mask >>= 1;
			putpixel (p, x, bpp);
		}
		for (uint32_t j = FONT_WIDTH; j < SYMBOL_WIDTH; j++)
			putpixel (p, bg, bpp);
	}
	for (uint32_t i = FONT_HEIGHT; i < LINE_HEIGHT; i++) {
		uint8_t *p = get_pixel (fbcon, y + i, x);
		for (uint32_t j = 0; j < SYMBOL_WIDTH; j++)
			putpixel (p, bg, bpp);
	}
}

/**
 * fbcon_putc - put a character on the framebuffer
 * @fbcon: struct fbcon
 * @c: character to display
 * @fg: foreground color
 * @bg: background color
 */
static void
fbcon_putc (struct fbcon *fbcon, char c, uint32_t fg, uint32_t bg)
{
	if (c == '\n') {
		fbcon->cx = 0;
		fbcon->cy += LINE_HEIGHT;
		if (fbcon->cy + LINE_HEIGHT > fbcon->height) {
			uint32_t n = fbcon->cy + LINE_HEIGHT - fbcon->height;
			fbcon_scroll (fbcon, n);
		}

		return;
	}

	fbcon_blit_character (fbcon, (uint8_t) c, fbcon->cx, fbcon->cy, fg, bg);
	fbcon->cx += SYMBOL_WIDTH;
	if (fbcon->cx + SYMBOL_WIDTH > fbcon->width) {
		fbcon->cx = 0;
		fbcon->cy += LINE_HEIGHT;
		if (fbcon->cy + LINE_HEIGHT > fbcon->height) {
			uint32_t n = fbcon->cy + LINE_HEIGHT - fbcon->height;
			fbcon_scroll (fbcon, n);
		}
	}
}

/**
 * fbcon_print - print a string to the fbcon
 * @fbcon: struct fbcon
 * @msg: string to display
 * @fg: foreground color
 * @bg: background color
 */
static void
fbcon_print (struct fbcon *fbcon, const char *msg, uint32_t fg, uint32_t bg)
{
	for (; *msg; msg++)
		fbcon_putc (fbcon, *msg, fg, bg);
}

/**
 * fbcon_emit_message - printk handler
 * @con: pointer to the Console member variable of struct fbcon
 * @level: loglevel
 * @msg_time: message time
 * @msg: message contents
 */
static void
fbcon_emit_message (Console *con, int level, usecs_t msg_time, const char *msg)
{
	struct fbcon *fbcon = container_of (&fbcon::con, con);
	char buf[24];
	snprintf (buf, sizeof (buf), "[%5llu.%06llu] ",
			(unsigned long long) (msg_time / 1000000),
			(unsigned long long) (msg_time % 1000000));

	if (level < 0)
		level = 0;
	else if (level > 4)
		level = 4;
	uint32_t fg_colors[5] = {
		fbcon->c_info,
		fbcon->c_info,
		fbcon->c_notice,
		fbcon->c_warn,
		fbcon->c_err
	};

	uint32_t bg = fbcon->c_background;
	uint32_t fg = fg_colors[level];

	fbcon_print (fbcon, buf, fbcon->c_msgtime, bg);
	fbcon_print (fbcon, msg, fg, bg);
	fbcon_flush (fbcon);
}

struct fbcon *last_registered_fbcon = nullptr;

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

	fbcon->cx = 0;
	fbcon->cy = 0;
	fbcon->c_background	= get_color (fmt,  35,  38,  39);
	fbcon->c_info		= get_color (fmt, 200, 200, 200);
	fbcon->c_notice		= get_color (fmt, 255, 255, 255);
	fbcon->c_warn		= get_color (fmt, 253, 188,  75);
	fbcon->c_err		= get_color (fmt, 237,  21,  21);
	fbcon->c_msgtime	= get_color (fmt,  17, 209,  22);

	uint32_t c = fbcon->c_background;
	uint32_t bpp = fbcon->fmt.bpp;
	for (uint32_t i = 0; i < fbcon->height; i++) {
		uint8_t *p = get_row (fbcon, i);
		for (uint32_t j = 0; j < fbcon->width; j++)
			putpixel (p, c, bpp);
	}
	fbcon_flush (fbcon);

	fbcon->con.emit_message = fbcon_emit_message;
	console_register (&fbcon->con);
	last_registered_fbcon = fbcon;
	return fbcon;
}

struct fbcon *
fbcon_find_one (void)
{
	return last_registered_fbcon;
}

uint32_t
fbcon_get_color (struct fbcon *fbcon, uint32_t r, uint32_t g, uint32_t b)
{
	return get_color (&fbcon->fmt, r, g, b);
}

uint8_t *
fbcon_get_pixel (struct fbcon *fbcon, uint32_t x, uint32_t y)
{
	return get_pixel (fbcon, y, x);
}

void
fbcon_put_pixel (struct fbcon *fbcon, uint8_t *pixel, uint32_t color)
{
	putpixel (pixel, color, fbcon->fmt.bpp);
}
