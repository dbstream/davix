/**
 * RWMutex - Reader-writer mutual exclusion.
 * Copyright (C) 2025-present  dbstream
 *
 * This is currently a very poor implementation of the two-mutexes reader-writer
 * lock.  It can be improved in the future when the need arises.
 */
#include <davix/atomic.h>
#include <davix/rwmutex.h>

void
RWMutex::write_unlock (void)
{
	common_mutex.unlock ();
}

void
RWMutex::read_unlock (void)
{
	if (atomic_dec_fetch (&reader_count, mo_release) == 0)
		common_mutex.unlock ();
}

void
RWMutex::write_lock (void)
{
	common_mutex.lock ();
}

int
RWMutex::write_lock_interruptible (void)
{
	return common_mutex.lock_interruptible ();
}

int
RWMutex::write_lock_timeout (nsecs_t ns)
{
	return common_mutex.lock_timeout (ns);
}

int
RWMutex::write_lock_timeout_interruptible (nsecs_t ns)
{
	return common_mutex.lock_timeout_interruptible (ns);
}

static inline bool
inc_unless_zero (size_t *ptr)
{
	size_t val = atomic_load_relaxed (ptr);
	while (val != 0) {
		bool ret = atomic_cmpxchg_weak (ptr, &val, val + 1,
				mo_acquire, mo_relaxed);
		if (ret)
			return true;
	}
	return false;
}

void
RWMutex::read_lock (void)
{
	if (inc_unless_zero (&reader_count))
		return;

	reader_mutex.lock ();

	if (inc_unless_zero (&reader_count)) {
		reader_mutex.unlock ();
		return;
	}

	common_mutex.lock ();
	atomic_store_release (&reader_count, 1);
	reader_mutex.unlock ();
}

int
RWMutex::read_lock_interruptible (void)
{
	if (inc_unless_zero (&reader_count))
		return 0;

	int error = reader_mutex.lock_interruptible ();
	if (error)
		return error;

	if (inc_unless_zero (&reader_count)) {
		reader_mutex.unlock ();
		return 0;
	}

	error = common_mutex.lock_interruptible ();
	if (!error)
		atomic_store_release (&reader_count, 1);
	reader_mutex.unlock ();
	return error;
}

int
RWMutex::read_lock_timeout (nsecs_t ns)
{
	if (inc_unless_zero (&reader_count))
		return 0;

	int error = reader_mutex.lock_timeout (ns);
	if (error)
		return error;

	if (inc_unless_zero (&reader_count)) {
		reader_mutex.unlock ();
		return 0;
	}

	error = common_mutex.lock_timeout (ns);
	if (!error)
		atomic_store_release (&reader_count, 1);
	reader_mutex.unlock ();
	return error;
}

int
RWMutex::read_lock_timeout_interruptible (nsecs_t ns)
{
	if (inc_unless_zero (&reader_count))
		return 0;

	int error = reader_mutex.lock_timeout_interruptible (ns);
	if (error)
		return error;

	if (inc_unless_zero (&reader_count)) {
		reader_mutex.unlock ();
		return 0;
	}

	error = common_mutex.lock_timeout_interruptible (ns);
	if (!error)
		atomic_store_release (&reader_count, 1);
	reader_mutex.unlock ();
	return error;
}
