/**
 * Kernel console.
 * Copyright (C) 2024  dbstream
 */
#ifndef _DAVIX_CONSOLE_H
#define _DAVIX_CONSOLE_H 1

#include <davix/list.h>
#include <davix/time.h>

struct console {
	struct list list;
	void (*emit_message) (struct console *con,
			int level, usecs_t msg_time, const char *msg);
};

extern void
console_register (struct console *con);

extern void
console_unregister (struct console *con);

#endif /* _DAVIX_CONSOLE_H */
