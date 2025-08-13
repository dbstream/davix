// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/Hal/smpboot.h
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

extern unsigned long HalTrampolineAddress;

void HalPrepareSMPBringup(void);

bool HalTryToStartProcessor(unsigned int cpu);

void HalSmpStartProcessors(void);

