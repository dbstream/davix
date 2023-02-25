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
	sched_after_task_switch(from, to);
}

/*
 * Switch the stack and registers to those in 'to', saving the old in 'from',
 * and call 'after_switch_from()' on the new stack. Then, return to 'to'.
 */
void __switch_to(struct task *from, struct task *to);

void switch_to(struct task *from, struct task *to)
{
	__switch_to(from, to);
}

static void x86_idle_task(void)
{
	enable_interrupts();
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

	task->arch_task_info.irq_stack = alloc_page(ALLOC_KERNEL, 0);
	if(!task->arch_task_info.irq_stack)
		panic("arch_init_idle_task: Failed to allocate memory for the IRQ stack.");

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

void arch_setup_init_task(struct task *task)
{
	/*
	 * NOTE: This wastes memory.
	 *
	 * It'd be better to just use the kernel_stack in main.S than to
	 * allocate an entirely new one here, but I won't bother now because
	 * of all the special handling that will need to occur for that to work.
	 */
	task->arch_task_info.kernel_stack = alloc_page(ALLOC_KERNEL, 0);
	if(!task->arch_task_info.kernel_stack)
		panic("arch_setup_init_task: Failed to allocate memory for the kernel stack.");

	task->arch_task_info.irq_stack = alloc_page(ALLOC_KERNEL, 0);
	if(!task->arch_task_info.irq_stack)
		panic("arch_setup_init_task: Failed to allocate memory for the IRQ stack.");

	struct x86_tss *tss = cpulocal_address(smp_self(), &x86_tss);
	tss->rsp0 = page_to_virt(task->arch_task_info.kernel_stack) + PAGE_SIZE;
	tss->ist1 = page_to_virt(task->arch_task_info.irq_stack) + PAGE_SIZE;
}
