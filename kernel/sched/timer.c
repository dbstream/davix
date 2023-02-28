/* SPDX-License-Identifier: MIT */
#include <davix/sched.h>
#include <davix/time.h>
#include <davix/printk.h>

void timer_interrupt(void)
{
	debug("timer interrupt\n");
	if(preempt_enabled())
		schedule();
}
