/* SPDX-License-Identifier: MIT */
#include <davix/sched.h>
#include <davix/page_alloc.h>
#include <davix/panic.h>
#include <davix/printk.h>
#include <asm/segment.h>

extern struct x86_tss x86_tss cpulocal;
struct x86_tss x86_tss cpulocal;

void x86_after_switch_from(struct task *from, struct task *to);
void x86_after_switch_from(struct task *from, struct task *to)
{
	info("task switch (from %p to %p)\n", from, to);
}

/*
 * Switch the stack and registers to those in 'to', saving the old in 'from',
 * and call 'after_switch_from()' on the new stack. Then, return to 'to'.
 */
void __switch_to(struct task *from, struct task *to);

static void x86_idle_task(void)
{
	/*
	 * A really simple idle task which just loops indefinitely.
	 */
	for(;;)
		relax();
}

void arch_init_idle_task(struct logical_cpu *cpu, struct task *task)
{
	task->arch_task_info.kernel_stack = alloc_page(ALLOC_KERNEL, 0);
	if(!task->arch_task_info.kernel_stack)
		panic("arch_init_idle_task: Failed to allocate memory for the kernel stack.");

	task->arch_task_info.timer_stack = alloc_page(ALLOC_KERNEL, 0);
	if(!task->arch_task_info.timer_stack)
		panic("arch_init_idle_task: Failed to allocate memory for the timer stack.");

	if(!cpu->online)
		return;

	struct idle_task_frame *frame = (struct idle_task_frame *)
		(page_to_virt(task->arch_task_info.kernel_stack)
			+ PAGE_SIZE - sizeof(struct idle_task_frame));

	frame->rbp = &frame->null_frame;
	frame->null_frame = (struct stack_frame) { NULL, 0 };
	frame->rip = (unsigned long) &x86_idle_task;
	task->arch_task_info.kernel_sp = frame;
}
