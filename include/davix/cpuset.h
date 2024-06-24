/**
 * cpusets
 * Copyright (C) 2024  dbstream
 *
 * A cpuset is a bitmask of CPUs. cpusets are used to represent the topology on
 * SMP systems. Other common uses are affinity masks (sched_affinity, etc...).
 */
#ifndef _DAVIX_CPUSET_H
#define _DAVIX_CPUSET_H 1

#include <davix/bitmap.h>

typedef struct cpuset {
	bitmap_t bitmap[BITMAP_SIZE(CONFIG_MAX_NR_CPUS)];
} cpuset_t;

extern unsigned int nr_cpus;

extern cpuset_t cpus_online;
extern cpuset_t cpus_present;

#define cpuset_get(cpuset, cpu)		bitmap_get ((cpuset)->bitmap, (cpu))
#define cpuset_set(cpuset, cpu)		bitmap_set ((cpuset)->bitmap, (cpu))
#define cpuset_clear(cpuset, cpu)	bitmap_clear ((cpuset)->bitmap, (cpu))

#define cpu_online(cpu)			cpuset_get(&cpus_online, (cpu))
#define cpu_present(cpu)		cpuset_get(&cpus_present, (cpu))

#define set_cpu_online(cpu)		cpuset_set(&cpus_online, (cpu))
#define set_cpu_present(cpu)		cpuset_set(&cpus_present, (cpu))

#define clear_cpu_online(cpu)		cpuset_clear(&cpus_online, (cpu))
#define clear_cpu_present(cpu)		cpuset_clear(&cpus_present, (cpu))

/**
 * Bitmaps work with unsigned longs, while we prefer unsigned ints. Define this
 * wrapper around bitmap_next_set_bit to convert from unsigned longs to unsigned
 * ints.
 */
static inline int
cpuset_next_set_bit (cpuset_t *cpuset, unsigned int *curr)
{
	unsigned long it = *curr;
	if (*curr == -1U)
		it = -1UL;
	if (!bitmap_next_set_bit (cpuset->bitmap, nr_cpus, &it))
		return 0;
	*curr = it;
	return 1;
}

#define cpuset_for_each(cpuset, cpu)	\
	for (unsigned int cpu = -1U; cpuset_next_set_bit ((cpuset), &cpu); )

#define for_each_online_cpu(cpu)	cpuset_for_each(&cpus_online, cpu)
#define for_each_present_cpu(cpu)	cpuset_for_each(&cpus_present, cpu)

#endif /* _DAVIX_CPUSET_H */
