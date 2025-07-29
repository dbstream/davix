// SPDX-License-Identifier: GPL-3.0
/*
 * File: Ke/log.cc
 * Debug logging facilities.
 *
 * Copyright (C) 2025  dbstream
 */
#include <Ke/console.h>
#include <Ke/log.h>
#include <Ke/rcu.h>
#include <Ki/log.h>
#include <davix/atomic.h>
#include <davix/export.h>
#include <davix/vsnprintf.h>

static CONSOLE *console_list;

/**
 * KeRegisterConsole - register a console output device for KePuts and KePrintf.
 * @console: pointer to a CONSOLE structure describing the console to register
 */
void KeRegisterConsole(CONSOLE *console)
{
	console->pNext = atomic_load_acquire(&console_list);

	while (!atomic_cmpxchg_weak(&console_list, &console->pNext, console,
			_MO_AcqRel, _MO_Acquire))
		;
}

/**
 * KePuts - put a message on the kernel console.
 * @str: message
 */
void KePuts(const char *str)
{
	KeRcuLock();
	CONSOLE *con = atomic_load_acquire(&console_list);

	while(con) {
		con->putString(con, str);
		con = con->pNext;
	}

	KeRcuUnlock();
}
EXPORT_SYMBOL(KePuts)

/**
 * KePrintf - put a message on the kernel console.
 * @fmt: the kernel's supported vsnprintf subset format string.
 */
void KePrintf(const char *fmt, ...)
{
	char buf[512];

	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

	KePuts(buf);
}
EXPORT_SYMBOL(KePrintf)

/**
 * KiBeginPanicLogging - prepare KePuts and KePrintf for panic message logging.
 */
void KiBeginPanicLogging(void)
{
	CONSOLE **pThis = &console_list;
	CONSOLE *con = *pThis;
	while(con) {
		if(!(con->flags & CONSOLE_PANIC_CAPABLE)) {
			con = con->pNext;
			*pThis = con;
			continue;
		}

		if(con->beginPanicLogging) {
			if (!con->beginPanicLogging(con)) {
				con = con->pNext;
				*pThis = con;
				continue;
			}
		}

		pThis = &con->pNext;
		con = con->pNext;
	}
}

/**
 * KiEndPanicLogging - finish panic message logging.
 */
void KiEndPanicLogging(void)
{
	for (CONSOLE *con = console_list; con; con = con->pNext) {
		if (con->endPanicLogging)
			con->endPanicLogging(con);
	}
}

