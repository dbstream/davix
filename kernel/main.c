/**
 * Kernel main() function.
 * Copyright (C) 2024  dbstream
 */
#include <davix/context.h>
#include <davix/cpuset.h>
#include <davix/exec.h>
#include <davix/file.h>
#include <davix/fs.h>
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
#include <asm/usercopy.h>

#if CONFIG_UACPI
#include <uacpi/event.h>
#include <uacpi/uacpi.h>
#endif

static const char kernel_version[] = KERNELVERSION;

unsigned long boot_module_start = 0, boot_module_end = 0;

static errno_t
boot_module_pread (struct file *file, void *buf,
		size_t size, off_t offset, ssize_t *out)
{
	(void) file;

	if (offset < 0)
		return EINVAL;
	unsigned long uoffset = (unsigned long) offset;
	if (uoffset + size < uoffset)
		return EINVAL;

	if (uoffset + size > boot_module_end - boot_module_start)
		return EINVAL;

	void *src = (void *) phys_to_virt (boot_module_start + offset);
	errno_t e = __memcpy_to_userspace (buf, src, size);
	if (e == ESUCCESS)
		*out = size;
	return e;
}

/**
 * Implement everything needed for kernel_fexecve().
 */
static const struct file_ops boot_module_ops = {
	.pread = boot_module_pread
};

static struct file boot_module_file = {
	.refcount = 1,
	.ops = &boot_module_ops
};

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
	init_vnode_cache ();
	init_inode_cache ();
	init_mounts ();

#if CONFIG_UACPI
	uacpi_initialize (0);
	uacpi_namespace_load ();
	uacpi_namespace_initialize ();
	uacpi_finalize_gpe_initialization ();
#endif

	if (boot_module_start == boot_module_end)
		panic ("No boot module was provided!");

	errno_t e = kernel_fexecve (fget (&boot_module_file), NULL, NULL);
	panic ("kernel_fexecve returned error %d!", e);
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
