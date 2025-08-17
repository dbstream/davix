// SPDX-License-Identifier: GPL-3.0
/*
 * File: OS/status.h
 * enum OSSTATUS - kernel status codes.
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

typedef int OSSTATUS;

static inline bool OS_SUCCESS(OSSTATUS status)
{
	return status == 0;
}

enum : OSSTATUS {
	OS_STATUS_SUCCESS				= 0,
	OS_STATUS_KERNEL_ALLOCATION_FAILED		= 1,
};

