/**
 * Processor bringup on SMP systems.
 * Copyright (C) 2024  dbstream
 */
#include <davix/cpuset.h>
#include <davix/initmem.h>
#include <davix/kmalloc.h>
#include <davix/page.h>
#include <davix/panic.h>
#include <davix/printk.h>
#include <davix/sched.h>
#include <davix/smp.h>
#include <davix/spinlock.h>
#include <davix/string.h>
#include <davix/time.h>
#include <davix/vmap.h>
#include <asm/apic.h>
#include <asm/cregs.h>
#include <asm/fence.h>
#include <asm/msr.h>
#include <asm/smp.h>
#include <asm/smpboot.h>
#include <asm/task.h>
#include <asm/tsc.h>
#include <asm/trap.h>

static inline void
udelay (uint64_t microseconds)
{
	if (!microseconds)
		return;

	uint64_t target = us_since_boot () + microseconds;
	do
		arch_relax ();
	while (us_since_boot () < target);
}

/**
 * The SMPBOOT page is targeted by the INIT-INIT-SIPI sequence. However startup
 * processors are only able to begin execution in the first 256 4KiB pages of
 * the system. To protect from BIOS weirdnesses and off-by-ones, we limit
 * ourselves to the page 1 to 254 range.
 */
static void *smpboot_page = NULL;

static bool disable_smpboot = true;

extern char x86_ap_boot_trampoline[], x86_ap_boot_trampoline_end[];

errno_t
arch_smpboot_init (void)
{
	/** use low memory for the SMPBOOT page */
	unsigned long addr = initmem_alloc_phys_range (PAGE_SIZE, PAGE_SIZE,
		0x1000UL, 0xff000UL);
	if (!addr)
		return ENOMEM;

	printk (PR_INFO "smpboot: allocated SMPBOOT page at 0x%lx\n", addr);

	smpboot_page = vmap_phys_explicit ("SMPBOOT", addr, PAGE_SIZE,
		__PG_PRESENT | __PG_WRITE, PAGE_SIZE, addr, addr + PAGE_SIZE - 1);
	if (!smpboot_page) {
		initmem_free_phys (addr, PAGE_SIZE);
		return ENOMEM;
	}

	if ((unsigned long) smpboot_page != addr)
		panic ("smpboot: smpboot_page mapped using vmap_phys_explicit is at incorrect location!");

	/** copy the trampoline to the SMPBOOT page */
	memcpy (smpboot_page, &x86_ap_boot_trampoline[0],
		&x86_ap_boot_trampoline_end[0] - &x86_ap_boot_trampoline[0]);

	/**
	 * The first few instructions on the SMPBOOT page are as follows:
	 *	0xfa						cli
	 *	0x90						nop
	 *	0x66	0xb8	0xDD	0xCC	0xBB	0xAA	movl $0xAABBCCDD, %eax
	 * This is required for the relocatable SMPBOOT trampoline, so it
	 * knows where it is. Overwrite 0xAABBCCDD with the address of the SMPBOOT page.
	 *
	 * Additionally, store the desired %cr4 at +2048 and the desired EFER at + 2052.
	 */
	*(volatile uint32_t *) ((unsigned long) smpboot_page + 4) = (uint32_t) addr;
	*(volatile uint32_t *) ((unsigned long) smpboot_page + 0x800) = __cr4_state;
	*(volatile uint32_t *) ((unsigned long) smpboot_page + 0x804) = __efer_state;
	*(volatile uint32_t *) ((unsigned long) smpboot_page + 0x808) = __cr0_state;
	*(volatile uint32_t *) ((unsigned long) smpboot_page + 0x80c) = read_cr3 ();

	disable_smpboot = false;
	return ESUCCESS;
}

extern void *__startup_rsp;
extern void *__startup_rip;

static int booting_cpu;

void
startup64_ap_initialize_cpu (void)
{
	write_msr (MSR_GSBASE, __cpulocal_offsets[booting_cpu]);
}

static bool sync_point_0;
static bool sync_point_1;
static bool sync_point_2;

static void
start_additional_processor (void)
{
	mb ();

	atomic_store_relaxed (&sync_point_0, true);

	do
		arch_relax ();
	while (atomic_load_relaxed (&sync_point_1) != true);

	x86_synchronize_tsc_victim ();
	x86_setup_local_traps ();
	apic_init_ap ();

//	printk ("Hello from another CPU!\n");
	smp_start_additional_cpu ();
	atomic_store_release (&sync_point_2, true);

	sched_idle ();
}

static spinlock_t smpboot_lock;

static bool
startup_cpu_using_APIC (unsigned int target, unsigned int vector)
{
	apic_send_IPI (APIC_DM_INIT | APIC_TRIGGER_MODE_LEVEL | APIC_LEVEL_ASSERT, target);
	udelay (10000);
	apic_send_IPI (APIC_DM_INIT | APIC_TRIGGER_MODE_LEVEL, target);
	mb();

	for (int i = 0; i < 2; i++) {
		apic_send_IPI (APIC_DM_SIPI | vector, target);
		udelay (300);

		if (atomic_load_relaxed (&sync_point_0))
			return true;
	}

	return false;
}

errno_t
arch_smp_boot_cpu (unsigned int cpu)
{
	(void) cpu;

	if (disable_smpboot)
		return ENOTSUP;

	bool flag = irq_save ();
	if (!spin_trylock (&smpboot_lock)) {
		irq_restore (flag);
		return EAGAIN;
	}

	if (disable_smpboot) {
		spin_unlock (&smpboot_lock);
		irq_restore (flag);
		return ENOTSUP;
	}

	// mfence in startup_cpu_using_APIC will make these visible
	booting_cpu = cpu;
	sync_point_0 = false;
	sync_point_1 = false;
	sync_point_2 = false;

	unsigned long stack_top = (unsigned long) kmalloc (TASK_STK_SIZE);
	if (!stack_top) {
		spin_unlock (&smpboot_lock);
		irq_restore (flag);
		return ENOMEM;
	}

	stack_top += TASK_STK_SIZE;
	__startup_rsp = (void *) stack_top;
	__startup_rip = start_additional_processor;

	if (!startup_cpu_using_APIC (cpu_to_apicid[cpu], (unsigned long) smpboot_page >> 12)) {
		printk (PR_ERR "smp: failed to boot CPU%u; disabling smpboot...\n", cpu);
		disable_smpboot = true;
		spin_unlock (&smpboot_lock);
		irq_restore (flag);
		return EIO;
	}

	atomic_store_relaxed (&sync_point_1, true);

	x86_synchronize_tsc_control ();

	do
		arch_relax ();
	while (!atomic_load_acquire (&sync_point_2));

	spin_unlock (&smpboot_lock);
	irq_restore (flag);
	return ESUCCESS;
}

static void
resynchronize_tsc_smp_work (void *arg)
{
	(void) arg;
	x86_synchronize_tsc_victim ();
}

/**
 * Synchronize all other CPU's TSCs against the current CPU's TSC.
 *
 * This must be done here because we need to hold the smpboot_lock while we
 * do it.
 */
void
x86_smp_resynchronize_tsc (void)
{
	struct smp_call_on_cpu_work work = {
		.func = resynchronize_tsc_smp_work,
		.arg = NULL
	};

	spin_lock (&smpboot_lock);

	for_each_online_cpu (cpu) {
		if (cpu == this_cpu_id ())
			continue;

		smp_call_on_cpu_async (cpu, &work);
		x86_synchronize_tsc_control ();
		smp_wait_for_call_on_cpu (&work);
	}

	spin_unlock (&smpboot_lock);
	printk (PR_INFO "Resynchronized TSC on all CPUs to reference CPU%u\n", this_cpu_id ());
}
