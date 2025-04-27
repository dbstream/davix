/**
 * Definitions for the zoned page allocator.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#include <stdint.h>
#include <davix/allocation_class.h>
#include <dsl/interval.h>

enum : allocation_class {
	__ALLOC_LOW4G	= 1U << 30,
	__ALLOC_LOW1M	= 1U << 31,
};

constexpr int num_page_zones = 3;

/**
 * allocation_zone - get the primary zone to be used for a given allocation.
 * @aclass: allocation class
 * Returns the primary zone to be used.
 */
constexpr int
allocation_zone (allocation_class aclass)
{
	if (aclass & __ALLOC_LOW1M)
		return 2;
	else if (aclass & __ALLOC_LOW4G)
		return 1;
	else
		return 0;
}

/**
 * phys_to_zone - get the zone that corresponds to a given physical address.
 * @phys: physical address
 * Returns the corresponding zone.
 */
constexpr int
phys_to_zone (uintptr_t phys)
{
	if (phys < 1024UL * 1024UL)
		return 2;
	else if (phys < 4UL * 1024UL * 1024UL * 1024UL)
		return 1;
	else
		return 0;
}

/**
 * zone_minaddr - get the lowest address that belongs to a given zone.
 * @zone: index of the zone
 * Returns the lowest physical address that belongs to the zone.
 */
constexpr uintptr_t
zone_minaddr (int zone)
{
	switch (zone) {
	case 0: return 4UL * 1024UL * 1024UL * 1024UL;
	case 1: return 1024UL * 1024UL;
	default: return 0;
	}
}

/**
 * zone_maxaddr - get the highest address that belongs to a given zone.
 * @zone: index of the zone
 * Returns the highest physical address that belongs to the zone.
 */
constexpr uintptr_t
zone_maxaddr (int zone)
{
	switch (zone) {
	case 0: return -1UL;
	case 1: return 4UL * 1024UL * 1024UL * 1024UL - 1UL;
	default: return 1024UL * 1024UL - 1UL;
	}
}

/**
 * zone_has_fallback - test if an allocation zone has a fallback zone.
 * @zone: index of the zone to test
 * Returns true if there is a fallback zone.
 */
constexpr bool
zone_has_fallback (int zone)
{
	return zone != 2;
}

/**
 * fallback_zone - get the fallback zone for a zone with a fallback.
 * @zone: index of the zone to get the fallback for
 * Returns the index of the fallback zone.
 */
constexpr int
fallback_zone (int zone)
{
	if (zone == 0)
		return 1;
	else
		return 2;
}
