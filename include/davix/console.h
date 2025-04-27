/**
 * Kernel message logging sinks.
 * Copyright (C) 2025-present  dbstream
 *
 * A kernel "console" is effectively a sink for printk() messages.
 */
#pragma once

#include <davix/time.h>

struct Console {
	Console *next;
	Console **link;

	void (*emit_message) (Console *con, int loglevel,
			usecs_t msg_time, const char *msg);
};

void
console_register (Console *con);
