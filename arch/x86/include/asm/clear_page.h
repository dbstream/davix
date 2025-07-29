// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/asm/clear_page.h
 * Fast full page zeroing.
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

extern "C" void clear_page(void *addr);

