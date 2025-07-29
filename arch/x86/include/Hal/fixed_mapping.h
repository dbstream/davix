// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/Hal/fixed_mapping.h
 * Fixed address kernel memory mappings.
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

#include <Hal/pteflags.h>

enum : int {
	HAL_FIXED_MAP_LOCAL_APIC		= 0,
};

void HalSetFixedMapping(int idx, unsigned long address, PTEFLAGS flags);
void HalClearFixedMapping(int idx);

static constexpr unsigned long HAL_FIXED_MAPPING_BASE = 0xffffffffff000000UL;

constexpr unsigned long HalFixedMappingAddress(int idx)
{
	return HAL_FIXED_MAPPING_BASE + 0x1000UL * idx;
}

