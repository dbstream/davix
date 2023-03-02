/* SPDX-License-Identifier: MIT */
#include <davix/sched.h>
#include <davix/time.h>
#include <davix/printk.h>

void timer_interrupt(void)
{
	/*
	 * This is literally how we decide when tasks need to switch.
	 */
	set_task_flag(current_task(), TASK_NEED_RESCHED);
}
