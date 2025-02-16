/**
 * Framebuffer console driver.
 * Copyright (C) 2025-present  dbstream
 */
#include <davix/console.h>
#include <davix/fbcon.h>
#include <davix/kmalloc.h>
#include <davix/printk.h>
#include <davix/snprintf.h>
#include <davix/string.h>

extern uint8_t VGA9_F16[];

#define FONT_WIDTH 8
#define FONT_HEIGHT 16

#define CHAR_WIDTH 9
#define LINE_HEIGHT 17

static inline uint32_t
get_color (const struct fbcon_format_info *fmt, uint8_t r, uint8_t g, uint8_t b)
{
	uint32_t x = 0;

	x |= ((uint32_t) r << fmt->red_offset) & fmt->red_mask;
	x |= ((uint32_t) g << fmt->green_offset) & fmt->green_mask;
	x |= ((uint32_t) b << fmt->blue_offset) & fmt->blue_mask;

	return x;
}

struct fbcon {
	struct console con;
	struct fbcon_format_info fmt;
	uint32_t width, height, pitch;
	unsigned long nbytes;
	void *front, *back;
	bool is_internal_back;

	uint32_t cx, cy;

	uint32_t c_background;
	uint32_t c_info;
	uint32_t c_notice;
	uint32_t c_warn;
	uint32_t c_err;
	uint32_t c_crit;
	uint32_t c_msgtime;
};

static inline uint8_t *
fbcon_row (struct fbcon *con, uint32_t row)
{
	return (uint8_t *) con->back + (row * (unsigned long) con->pitch);
}

static inline uint8_t *
fbcon_pixel (struct fbcon *con, uint32_t row, uint32_t column)
{
	uint8_t *p = fbcon_row (con, row);

	// bpp is always 32
	return p + (4 * column);
}

static inline void
putpixel32 (uint8_t *ptr, uint32_t x)
{
	*(uint32_t *) ptr = x;
}

static inline void
putpixel (uint8_t **pptr, uint32_t x, uint8_t bpp)
{
	(void) bpp; // bpp is always 32

	putpixel32 (*pptr, x);
	(*pptr) += 4;
}

static void
fbcon_scroll (struct fbcon *fbcon, uint32_t scroll, uint32_t bg)
{
	uint8_t bpp = fbcon->fmt.bpp;
	unsigned long nbytes = scroll * fbcon->pitch;
	memmove (fbcon_row (fbcon, 0), fbcon_row (fbcon, scroll),
			fbcon->nbytes - nbytes);
	fbcon->cy -= scroll;
	for (uint32_t i = fbcon->cy; i < fbcon->height; i++) {
		uint8_t *p = fbcon_row (fbcon, i);
		for (uint32_t j = 0; j < fbcon->width; j++)
			putpixel (&p, bg, bpp);
	}
}

static inline void
fbcon_putc (struct fbcon *fbcon, char c, uint32_t bg, uint32_t fg)
{
	if (c == '\n') {
		fbcon->cx = 1;
		fbcon->cy += LINE_HEIGHT;
		if (fbcon->cy + LINE_HEIGHT > fbcon->height) {
			uint32_t scroll = fbcon->cy + LINE_HEIGHT - fbcon->height;
			fbcon_scroll (fbcon, scroll, bg);
		}

		return;
	}

	uint8_t bpp = fbcon->fmt.bpp;
	for (uint32_t i = 0; i < FONT_HEIGHT; i++) {
		uint8_t data = VGA9_F16[FONT_HEIGHT * (int) c + i];
		uint8_t mask = 0x80;
		uint8_t *p = fbcon_pixel (fbcon, fbcon->cy + i, fbcon->cx);
		for (uint32_t j = 0; j < FONT_WIDTH; j++) {
			uint32_t x = (data & mask) ? fg : bg;
			mask >>= 1;
			putpixel (&p, x, bpp);
		}
	}

	fbcon->cx += CHAR_WIDTH;
	if (fbcon->cx + CHAR_WIDTH > fbcon->width) {
		fbcon->cx = 1;
		fbcon->cy += LINE_HEIGHT;
		if (fbcon->cy + LINE_HEIGHT > fbcon->height) {
			uint32_t scroll = fbcon->cy + LINE_HEIGHT - fbcon->height;
			fbcon_scroll (fbcon, scroll, bg);
		}
	}
}

static void
fbcon_print (struct fbcon *fbcon, const char *msg, uint32_t bg, uint32_t fg)
{
	for (; *msg; msg++)
		fbcon_putc (fbcon, *msg, bg, fg);
}

static void
fbcon_flush (struct fbcon *con)
{
	memcpy (con->front, con->back, con->nbytes);
}

static void
fbcon_emit_message (struct console *con,
		int level, usecs_t msg_time, const char *msg)
{
	struct fbcon *fbcon = struct_parent (struct fbcon, con, con);

	char buf[24];
	snprintf (buf, sizeof (buf), "[%5llu.%06llu] ",
		msg_time / 1000000, msg_time % 1000000);

	uint32_t fg, bg = fbcon->c_background;
	uint32_t fg_colors[6] = {
		fbcon->c_info,
		fbcon->c_info,
		fbcon->c_notice,
		fbcon->c_warn,
		fbcon->c_err,
		fbcon->c_crit
	};
	if (level >= 6)
		fg = fbcon->c_crit;
	else
		fg = fg_colors[level];

	fbcon_print (fbcon, buf, bg, fbcon->c_msgtime);
	fbcon_print (fbcon, msg, bg, fg);
	fbcon_flush (fbcon);
}

struct fbcon *
fbcon_register_framebuffer (uint32_t width, uint32_t height, uint32_t pitch,
		const struct fbcon_format_info *fmt, void *front, void *back)
{
	if (fmt->bpp != 32) {
		printk (PR_WARN "fbcon: fbcon_register_framebuffer(bpp=%d) is not supported\n",
				fmt->bpp);
		return NULL;
	}

	if (width < 32 || height < 32) {
		printk (PR_WARN "fbcon: fbcon_register_framebuffer(%ux%u) is too small\n", width, height);
		return NULL;
	}

	struct fbcon *fbcon = kmalloc (sizeof (*fbcon));
	if (!fbcon)
		return NULL;

	fbcon->con.emit_message = fbcon_emit_message;
	fbcon->fmt = *fmt;
	fbcon->width = width;
	fbcon->height = height;
	fbcon->pitch = pitch;
	fbcon->nbytes = (unsigned long) height * pitch;
	fbcon->front = front;
	fbcon->cx = 1;
	fbcon->cy = 1;
	fbcon->c_background =	get_color (fmt,   0,   0,   0);
	fbcon->c_info =		get_color (fmt, 220, 220, 220);
	fbcon->c_notice =	get_color (fmt, 255, 255, 255);
	fbcon->c_warn =		get_color (fmt, 255, 127,   0);
	fbcon->c_err =		get_color (fmt, 220,   0,   0);
	fbcon->c_crit =		get_color (fmt, 255,   0,   0);
	fbcon->c_msgtime =	get_color (fmt,   0, 255,   0);
	if (back) {
		fbcon->back = back;
		fbcon->is_internal_back = false;
	} else {
		fbcon->back = kmalloc (fbcon->nbytes);
		fbcon->is_internal_back = true;

		if (!fbcon->back) {
			kfree (fbcon);
			return NULL;
		}
	}

	uint32_t c = fbcon->c_background;
	for (uint32_t i = 0; i < fbcon->height; i++) {
		uint8_t *p = fbcon_row (fbcon, i);
		for (uint32_t j = 0; j < fbcon->width; j++)
			putpixel (&p, c, 32);
	}
	fbcon_flush (fbcon);

	console_register (&fbcon->con);
	printk ("fbcon:  registered new framebuffer at virt %p  (%ux%ux%d)\n",
			front, width, height, fmt->bpp);
	return fbcon;
}

void
fbcon_remove (struct fbcon *fbcon)
{
	console_unregister (&fbcon->con);
	if (fbcon->is_internal_back)
		kfree (fbcon->back);
	kfree (fbcon);
}

