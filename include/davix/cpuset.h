/**
 * cpuset
 * Copyright (C) 2025-present  dbstream
 *
 * A cpuset is a bitmask of CPUs.
 */
#pragma once

#include <stdint.h>

extern int nr_cpus;

static constexpr int cpuset_bitmap_size = (CONFIG_MAX_NR_CPUS + 63) / 64;

struct cpuset {
	uint64_t bitmap[cpuset_bitmap_size] {};

	constexpr void
	set (int cpu)
	{
		int word = cpu / 64;
		uint64_t bit = UINT64_C(1) << (cpu % 64);

		bitmap[word] |= bit;
	}

	constexpr void
	clear (int cpu)
	{
		int word = cpu / 64;
		uint64_t bit = UINT64_C(1) << (cpu % 64);

		bitmap[word] &= ~bit;
	}

	constexpr bool
	get (int cpu) const
	{
		int word = cpu / 64;
		uint64_t bit = UINT64_C(1) << (cpu % 64);

		return (bitmap[word] & bit) ? true : false;
	}

	constexpr bool
	operator() (int cpu) const
	{
		return get (cpu);
	}

	constexpr int
	next (int cpu) const
	{
		while (cpu < nr_cpus) {
			if (get (cpu))
				return cpu;
			cpu++;
		}
		return -1;
	}

	struct iterator {
		const cpuset *set;
		int cpu;

		constexpr int
		operator *(void) const
		{
			return cpu;
		}

		constexpr bool
		operator!= (const iterator &other) const
		{
			return cpu != other.cpu;
		}

		constexpr iterator &
		operator++ (void)
		{
			cpu = set->next (cpu + 1);
			return *this;
		}
	};

	constexpr iterator
	begin (void) const
	{
		return { this, next (0) };
	}

	constexpr iterator
	end (void) const
	{
		return { this, -1 };
	}
};

extern cpuset cpu_online;
extern cpuset cpu_present;

void
cpuset_init (void);

void
set_nr_cpus (int cpus);
