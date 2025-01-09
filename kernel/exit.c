/**
 * Task termination and exit() syscall.
 * Copyright (C) 2025-present  dbstream
 */
#include <davix/printk.h>
#include <davix/sched.h>
#include <davix/syscall.h>
#include <asm/irq.h>

SYSCALL1(void, exit, int, status)
{
	status &= 0xff;

	struct task *self = get_current_task ();

	printk (PR_INFO "task %.16s is exiting with status=%d\n", self->comm, status);

	// TODO: proper exiting...

	irq_disable ();
	self->state = TASK_ZOMBIE;
	schedule ();
	syscall_return_void;
}
