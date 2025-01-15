/**
 * 'struct timespec'
 * Copyright (C) 2025-present  dbstream
 */
#ifndef _DAVIX_UAPI_TIMESPEC_H
#define _DAVIX_UAPI_TIMESPEC_H 1

#include <davix/uapi/types.h>

struct timespec {
	time_t	tv_sec;
	long	tv_nsec;
};

#endif /** _DAVIX_UAPI_TIMESPEC_H  */
