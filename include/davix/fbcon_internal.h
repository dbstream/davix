/**
 * Functions internal to fbcon.
 * Copyright (C) 2025-present  dbstream
 *
 * The reason this header exists is so it can be used by the fireworks test.
 */
#pragma once

#include <stdint.h>

struct fbcon;

struct fbcon *
fbcon_find_one (void);

void
fbcon_flush (struct fbcon *fbcon);

uint32_t
fbcon_get_color (struct fbcon *fbcon, uint32_t r, uint32_t g, uint32_t b);

uint8_t *
fbcon_get_pixel (struct fbcon *fbcon, uint32_t x, uint32_t y);

void
fbcon_put_pixel (struct fbcon *fbcon, uint8_t *pixel, uint32_t color);
