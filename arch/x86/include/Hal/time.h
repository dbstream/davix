// SPDX-License-Identifier: GPL-3.0
/*
 * File: Hal/time.h
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

unsigned long long HalNanosSinceBoot(void);

static inline unsigned long long HalMicrosSinceBoot(void)
{
	return HalNanosSinceBoot() / 1000;
}

static inline unsigned long long HalMillisSinceBoot(void)
{
	return HalNanosSinceBoot() / 1000000UL;
}

