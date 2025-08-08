// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/Ke/time.h
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

typedef unsigned long long nsec_t;
typedef unsigned long long usec_t;
typedef unsigned long long msec_t;

#include <Hal/time.h>

static inline nsec_t KeNanosSinceBoot(void)
{
	return HalNanosSinceBoot();
}

static inline usec_t KeMicrosSinceBoot(void)
{
	return HalMicrosSinceBoot();
}

static inline msec_t KeMillisSinceBoot(void)
{
	return HalMillisSinceBoot();
}

