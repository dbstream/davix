/**
 * SMP (Simultaneous Multiprocessing) bringup.
 * Copyright (C) 2025-present  dbstream
 */
#include <asm/apic.h>
#include <asm/apic_def.h>
#include <asm/asm.h>
#include <asm/cpufeature.h>
#include <asm/idt.h>
#include <asm/msr_bits.h>
#include <asm/percpu.h>
#include <asm/smp.h>
#include <asm/time.h>
#include <davix/cpuset.h>
#include <davix/printk.h>
#include <davix/sched.h>
#include <davix/smp.h>
#include <davix/spinlock.h>
#include <davix/time.h>
#include <davix/vmap.h>
#include <stdint.h>

#include <davix/ktimer.h>

static unsigned int currently_booting_cpu;
extern "C" uintptr_t __startup_rip;
extern "C" uintptr_t __ap_startup_rsp;

extern "C" void
x86_smpboot_setup_percpu (void)
{
	write_msr (MSR_GSBASE, pcpu_detail::offsets[currently_booting_cpu]);
}

static bool sync_point_0;
static bool sync_point_1;
static bool sync_point_2;
static bool sync_point_3;
static bool sync_point_4;

static DEFINE_PERCPU(KTimer, ap_ktimer);

static void
ap_ktimer_fn (KTimer *t, void *arg)
{
	(void) t;
	(void) arg;

	printk (PR_INFO "CPU%u KTimer says hello!\n", this_cpu_id ());
}

static void
start_additional_processor (void)
{
	if (has_feature (FEATURE_PAT))
		write_msr (MSR_PAT, 0x100070406UL);

	smp_mb ();
	atomic_store_relaxed (&sync_point_0, true);

	do
		__builtin_ia32_pause ();
	while (atomic_load_relaxed (&sync_point_1) != true);

	x86_synchronize_tsc_victim ();
	x86_ap_setup_idt ();
	apic_init_ap ();

	atomic_store_release (&sync_point_2, true);
	do
		__builtin_ia32_pause ();
	while (atomic_load_relaxed (&sync_point_3) != true);
	atomic_store_relaxed (&sync_point_4, true);
//	printk (PR_NOTICE "CPU%u says Hello!\n", this_cpu_id ());
//	tsc_sync_dump ();
	apic_start_timer ();
	raw_irq_enable ();
	KTimer *t = percpu_ptr (ap_ktimer);
	t->init (ap_ktimer_fn, nullptr);
	t->enqueue (ns_since_boot () + 1000000000);
	sched_idle ();
}

static spinlock_t smpboot_lock;

static bool
startup_via_apic (uint32_t target, uint32_t vector)
{
	apic_send_IPI (APIC_DM_INIT | APIC_LEVEL_TRIGGERED | APIC_LEVEL_ASSERT, target);
	mdelay (10);
	apic_send_IPI (APIC_DM_INIT | APIC_LEVEL_TRIGGERED, target);
	smp_mb ();

	for (int i = 0; i < 2; i++) {
		apic_send_IPI (APIC_DM_SIPI | vector, target);
		udelay (i ? 10000 : 300);

		if (atomic_load_relaxed (&sync_point_0))
			return true;
	}

	return false;
}

static bool
do_boot_cpu (unsigned int cpu, void *stack)
{
	sync_point_0 = false;
	sync_point_1 = false;
	sync_point_2 = false;
	sync_point_3 = false;
	sync_point_4 = false;
	currently_booting_cpu = cpu;
	__startup_rip = (uintptr_t) start_additional_processor;
	__ap_startup_rsp = (uintptr_t) stack + 4 * PAGE_SIZE;

	if (!startup_via_apic (cpu_to_apic_id (cpu), trampoline_addr >> 12))
		return false;

	atomic_store_relaxed (&sync_point_1, true);

	x86_synchronize_tsc_control ();

	do
		__builtin_ia32_pause ();
	while (!atomic_load_acquire (&sync_point_2));
	cpu_online.set (cpu);
	atomic_store_relaxed (&sync_point_3, true);
	do
		__builtin_ia32_pause ();
	while (!atomic_load_relaxed (&sync_point_4));

	return true;
}

bool
arch_smp_boot_cpu (unsigned int cpu)
{
	smpboot_lock.lock_irq ();
	if (cpu_online (cpu)) {
		smpboot_lock.unlock_irq ();
		return true;
	}

	smpboot_lock.unlock_irq ();
	void *stack = kmalloc_large (4 * PAGE_SIZE);
	if (!stack) {
		printk (PR_ERROR "smp_boot_cpu: failed to allocate stack for CPU%u\n", cpu);
		return false;
	}

	smpboot_lock.lock_irq ();
	if (cpu_online (cpu)) {
		smpboot_lock.unlock_irq ();
		kfree_large (stack);
		return true;
	}

	bool ret = do_boot_cpu (cpu, stack);
	smpboot_lock.unlock_irq ();
	return ret;
}
