/**
 * Processor bringup on SMP systems.
 * Copyright (C) 2024  dbstream
 */
#include <davix/initmem.h>
#include <davix/page.h>
#include <davix/panic.h>
#include <davix/printk.h>
#include <davix/smp.h>
#include <davix/spinlock.h>
#include <davix/string.h>
#include <davix/time.h>
#include <davix/vmap.h>
#include <asm/apic.h>
#include <asm/cregs.h>
#include <asm/msr.h>
#include <asm/pgtable.h>
#include <asm/smpboot.h>
#include <asm/task.h>

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

static void
startup_cpu_using_APIC (unsigned int target, unsigned int vector)
{
	apic_send_IPI (APIC_DM_INIT | APIC_TRIGGER_MODE_LEVEL | APIC_LEVEL_ASSERT, target);
	udelay (10000);
	apic_send_IPI (APIC_DM_INIT | APIC_TRIGGER_MODE_LEVEL, target);
	asm volatile ("mfence" ::: "memory");
	apic_send_IPI (APIC_DM_SIPI | vector, target);
	udelay (300);
}

extern void *__startup_rsp;
extern void *__startup_rip;

static void
start_additional_processor (void)
{
	asm volatile ("mfence" ::: "memory");

	for (;;)
		arch_relax ();
}

errno_t
arch_smp_boot_cpu (unsigned int cpu)
{
	(void) cpu;

	if (disable_smpboot)
		return ENOTSUP;

	unsigned long stack_top = 0;
	if (mm_is_early)
		stack_top = (unsigned long) initmem_alloc_virt (TASK_STK_SIZE, TASK_STK_SIZE);
	else {
		_Static_assert (TASK_STK_SIZE == PAGE_SIZE << 2, "TASK_STK_SIZE");
		struct pfn_entry *stack_page = alloc_page (ALLOC_KERNEL, 2);
		if (stack_page)
			stack_top = pfn_entry_to_virt (stack_page);
	}

	if (!stack_top)
		return ENOMEM;

	stack_top += TASK_STK_SIZE;
	__startup_rsp = (void *) stack_top;
	__startup_rip = start_additional_processor;

	printk ("smp: booting CPU%u\n", cpu);
	startup_cpu_using_APIC (cpu_to_apicid[cpu], (unsigned long) smpboot_page >> 12);

	return ESUCCESS;
}
