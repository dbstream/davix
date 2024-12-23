/**
 * x86 timekeeping
 * Copyright (C) 2024  dbstream
 */
#include <davix/printk.h>
#include <davix/stdbool.h>
#include <davix/time.h>
#include <asm/hpet.h>
#include <asm/sections.h>
#include <asm/tsc.h>

static bool use_hpet;
static bool use_tsc;

bool
x86_time_is_tsc (void)
{
	return use_tsc;
}

__INIT_TEXT
void
x86_time_init_early (void)
{
	bool can_use_hpet = x86_init_hpet ();
	bool can_use_tsc = false;

	can_use_tsc = x86_init_tsc (can_use_hpet);

	if (can_use_tsc)
		use_tsc = true;
	else if (can_use_hpet)
		use_hpet = true;
	else
		printk (PR_WARN "x86/time: No supported time sources were found.\n");
}

nsecs_t
ns_since_boot (void)
{
	if (use_tsc)
		return x86_tsc_nsecs ();

	if (use_hpet)
		return x86_hpet_nsecs ();

	return 0;
}
