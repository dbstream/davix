/**
 * cpuset
 * Copyright (C) 2025-present  dbstream
 *
 * A cpuset is a bitmask of CPUs.
 */
#pragma once

#include <davix/atomic.h>
#include <stdint.h>

extern unsigned int nr_cpus;

static constexpr unsigned int cpuset_bitmap_size = (CONFIG_MAX_NR_CPUS + 63) / 64;

struct cpuset {
	uint64_t bitmap[cpuset_bitmap_size] {};

	constexpr void
	set (unsigned int cpu)
	{
		unsigned int word = cpu / 64;
		uint64_t bit = UINT64_C(1) << (cpu % 64);

		atomic_fetch_or (&bitmap[word], bit, mo_relaxed);
	}

	constexpr void
	clear (unsigned int cpu)
	{
		unsigned int word = cpu / 64;
		uint64_t bit = UINT64_C(1) << (cpu % 64);

		atomic_fetch_and (&bitmap[word], ~bit, mo_relaxed);
	}

	constexpr bool
	get (unsigned int cpu) const
	{
		unsigned int word = cpu / 64;
		uint64_t bit = UINT64_C(1) << (cpu % 64);

		uint64_t data = atomic_load_relaxed (&bitmap[word]);
		return (data & bit) ? true : false;
	}

	constexpr bool
	operator() (unsigned int cpu) const
	{
		return get (cpu);
	}

	constexpr unsigned int
	next (unsigned int cpu) const
	{
		while (cpu < nr_cpus) {
			if (get (cpu))
				return cpu;
			cpu++;
		}
		return -1U;
	}

	struct iterator {
		const cpuset *set;
		unsigned int cpu;

		constexpr unsigned int
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
		return { this, -1U };
	}
};

extern cpuset cpu_online;
extern cpuset cpu_present;

void
cpuset_init (void);

void
set_nr_cpus (unsigned int cpus);
