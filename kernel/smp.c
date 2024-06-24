/**
 * SMP support
 * Copyright (C) 2024  dbstream
 */
#include <davix/cpuset.h>
#include <davix/printk.h>
#include <asm/sections.h>

unsigned int nr_cpus = 1;

cpuset_t cpus_online;
cpuset_t cpus_present;

__INIT_TEXT
void
smp_init (void)
{
	printk (PR_INFO "CPUs: %u\n", nr_cpus);
}
