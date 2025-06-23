/**
 * RWMutex - Reader-writer mutual exclusion.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#include <davix/mutex.h>
#include <stddef.h>

struct RWMutex {
	Mutex common_mutex;
	size_t reader_count = 0;

	Mutex reader_mutex;

	void
	init (void)
	{
		common_mutex.init ();
		reader_mutex.init ();
		reader_count = 0;
	}

	void
	write_unlock (void);

	void
	read_unlock (void);

	void
	write_lock (void);

	int
	write_lock_interruptible (void);

	int
	write_lock_timeout (nsecs_t ns);

	int
	write_lock_timeout_interruptible (nsecs_t ns);

	void
	read_lock (void);

	int
	read_lock_interruptible (void);

	int
	read_lock_timeout (nsecs_t ns);

	int
	read_lock_timeout_interruptible (nsecs_t ns);
};

