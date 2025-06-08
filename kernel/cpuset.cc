/**
 * cpusets
 * Copyright (C) 2025-present  dbstream
 */
#include <davix/cpuset.h>

unsigned int nr_cpus = 1;

cpuset cpu_online;
cpuset cpu_present;

void
cpuset_init (void)
{
	cpu_present.set (0);
	cpu_online.set (0);
}

void
set_nr_cpus (unsigned int cpus)
{
	nr_cpus = cpus;
}
