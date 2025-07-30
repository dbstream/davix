// SPDX-License-Identifier: GPL-3.0
/*
 * File: Hal/internal.h
 * Hal-internal interfaces used during initialization.
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

void HalInitializeVirtualAddressSpace(void);

void HalSetEarlyPageAllocationFunction(unsigned long (*function)(void));

void HalMapRangeHHDM(unsigned long addr, unsigned long end);

void HalMapRangePFN(unsigned long addr, unsigned long end);

