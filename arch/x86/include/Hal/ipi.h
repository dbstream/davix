// SPDX-License-Identifier: GPL-3.0
/*
 * File: Hal/ipi.h
 * Issuing interprocessor interrupts.
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

void HalSendPanicIPI(unsigned int cpu);

void HalSendRescheduleIPI(unsigned int cpu);

void HalSendSchedTimerRecalcIPI(unsigned int cpu);

