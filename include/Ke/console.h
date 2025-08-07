// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/Ke/console.h
 * Kernel logging console.
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

enum : unsigned int {
	CONSOLE_PANIC_CAPABLE		= 1U << 0,
};

struct CONSOLE {
	unsigned int flags;
	CONSOLE *pNext;
	/**
	 * CONSOLE::putString - display a string on the console.
	 * @console: pointer to the CONSOLE itself
	 * @str: string to display
	 * @usec: microsecond message timestamp
	 */
	void (*putString)(CONSOLE *console, const char *str,
			unsigned long long usec);
	/**
	 * CONSOLE::beginPanicLogging - prepare for putString in panic context.
	 * @console: pointer to the CONSOLE itself
	 * Returns false if this console should be excluded from panic logging.
	 */
	bool (*beginPanicLogging)(CONSOLE *console);
	/**
	 * CONSOLE::endPanicLogging - finish panic logging.
	 * @console: pointer to the CONSOLE itself
	 */
	void (*endPanicLogging)(CONSOLE *console);
};

void KeRegisterConsole(CONSOLE *console);

