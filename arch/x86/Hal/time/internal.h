// SPDX-License-Identifier: GPL-3.0
/*
 * File: Hal/time/internal.h
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

extern bool HalUseHPETTimer;
extern bool HalUseTSC;

void HalInitializeHPET(void);

void HalInitializeTSC(void);

unsigned long long HalHPETNanosSinceBoot(void);

unsigned long long HalHPETReadCounter(void);

unsigned long long HalHPETNanosToCounter(unsigned long long nsecs);

unsigned long long HalHPETCounterToNanos(unsigned long long counter);

void HalSynchronizeTSC(bool control);

