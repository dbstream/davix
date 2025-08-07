// SPDX-License-Identifier: GPL-3.0
/*
 * File: Hal/time/internal.h
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

extern bool HalUseHPETTimer;

void HalInitializeHPET(void);

unsigned long long HalHPETNanosSinceBoot(void);

