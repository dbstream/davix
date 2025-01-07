/**
 * Kernel main() function.
 * Copyright (C) 2024  dbstream
 */
#include <davix/context.h>
#include <davix/cpuset.h>
#include <davix/initmem.h>
#include <davix/ktest.h>
#include <davix/main.h>
#include <davix/mm.h>
#include <davix/panic.h>
#include <davix/printk.h>
#include <davix/sched.h>
#include <davix/smp.h>
#include <davix/task_api.h>
#include <davix/vmap.h>
#include <davix/workqueue.h>
#include <asm/irq.h>
#include <asm/mm.h>
#include <asm/page.h>
#include <asm/sections.h>

#include <davix/page.h>
#include <asm/usercopy.h>

static const char kernel_version[] = KERNELVERSION;

/**
 * Start the init task.
 */
static void
start_init (void *arg)
{
	(void) arg;

	sched_begin_task ();
	printk (PR_INFO "start_init()\n");

	initmem_exit ();
	arch_init_late ();

	smp_boot_cpus ();

	run_ktests ();

	mm_init ();

	struct task *self = get_current_task ();
	self->mm = mmnew ();
	if (!self->mm)
		panic ("Failed to create the process_mm of init.");
	switch_to_mm (self->mm);

	pgalloc_dump ();

	printk ("Ok, mapping some memory...\n");
	void *addr;
	errno_t e = ksys_mmap (NULL, 0x10000000UL, PROT_READ | PROT_WRITE,
			MAP_ANON | MAP_PRIVATE, NULL, NULL, &addr);
	if (e == ESUCCESS) {
		printk ("Got ESUCCESS from ksys_mmap!\n");
//		pgalloc_dump ();
//		printk ("Ok, unmapping it...\n");
//		ksys_munmap (addr, 0x10000000UL);
	} else {
		printk ("Got error %d from ksys_mmap!\n", e);
	}

	pgalloc_dump ();

	printk ("copying stuff to userspace...\n");
	e = memcpy_to_userspace (addr, "Hello, world!", 14);
	if (e == ESUCCESS)
		printk ("Got ESUCCESS from memcpy_to_userspace!\n");
	else
		printk ("Got error %d from memcpy_to_userspace!\n", e);

	if (1)
		for (;;)
			arch_wfi ();

	irq_disable ();

	get_current_task ()->state = TASK_ZOMBIE;
	schedule ();
}

/**
 * Kernel main function. Called in an architecture-specific way to startup
 * kernel subsystems and eventually transition into userspace.
 */
__INIT_TEXT
void
main (void)
{
	set_cpu_online (0);
	set_cpu_present (0);

	preempt_off ();
	printk (PR_NOTICE "Davix version %s\n", kernel_version);

	arch_init ();
	arch_init_pfn_entry ();
	smp_init ();

	vmap_init ();
	smpboot_init ();

	irq_enable ();

	tasks_init ();
	sched_init ();

	if (!create_kernel_task ("init", start_init, NULL, 0))
		panic ("Couldn't create init task.");

	workqueue_init_this_cpu ();

	printk (PR_NOTICE "Reached end of main()\n");
	sched_idle ();
}
