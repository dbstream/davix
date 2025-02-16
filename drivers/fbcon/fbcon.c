/**
 * Framebuffer console driver.
 * Copyright (C) 2025-present  dbstream
 */
#include <davix/console.h>
#include <davix/fbcon.h>
#include <davix/kmalloc.h>
#include <davix/printk.h>
#include <davix/string.h>

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
};

static inline uint8_t *
fbcon_row (struct fbcon *con, uint32_t row)
{
	return (uint8_t *) con->back + (row * (unsigned long) con->pitch);
}

static inline void
putpixel32 (uint8_t *ptr, uint32_t x)
{
	*(uint32_t *) ptr = x;
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
	(void) con;
	(void) level;
	(void) msg_time;
	(void) msg;
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

	uint32_t c = get_color (&fbcon->fmt, 0, 0, 255);
	for (uint32_t i = 0; i < fbcon->height; i++) {
		uint8_t *p = fbcon_row (fbcon, i);
		for (uint32_t j = 0; j < fbcon->width; j++) {
			putpixel32 (p, c);
			p += 4;
		}
	}
	fbcon_flush (fbcon);

	console_register (&fbcon->con);
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

