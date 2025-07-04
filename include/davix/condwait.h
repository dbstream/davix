/**
 * Wait-on-condition: wait for some condition on an object to become true.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#include <davix/time.h>

typedef void *condwait_key_t;

condwait_key_t
condwait_obj_key (const void *obj);

int
__cond_wait_on (condwait_key_t key, bool (*cond) (const void *), const void *arg,
		bool interruptible, nsecs_t timeout);

void
__condwait_touch (condwait_key_t key);

#define __condwait(object, arg, condition, interruptible, timeout)	\
[&]()->int{								\
	typedef decltype(object) __cw_object_type;			\
	const void *__cw_object = (object);				\
	auto __cw_condition = [](const void *__cw_arg)->bool {		\
		__cw_object_type arg = (__cw_object_type) __cw_arg;	\
		return condition;					\
	};								\
	condwait_key_t __cw_key = condwait_obj_key (object);		\
	bool __cw_interruptible = (interruptible);			\
	nsecs_t __cw_timeout = (timeout);				\
	return __cond_wait_on (__cw_key, __cw_condition, __cw_object,	\
			__cw_interruptible, __cw_timeout);		\
} ()

#define condwait(o, a, c) __condwait(o, a, c, false, NO_TIMEOUT)
#define condwait_interruptible(o, a, c) __condwait(o, a, c, true, NO_TIMEOUT)
#define condwait_timeout(o, a, c, t) __condwait(o, a, c, false, t)
#define condwait_interruptible_timeout(o, a, c, t) __condwait(o, a, c, true, t)

#define condwait_touch(o) __condwait_touch(condwait_obj_key(o))

