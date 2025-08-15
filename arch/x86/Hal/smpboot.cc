// SPDX-License-Identifier: GPL-3.0
/*
 * File: Hal/smpboot.cc
 *
 * Copyright (C) 2025  dbstream
 */
#include <Hal/interrupt.h>
#include <Hal/mm.h>
#include <Hal/page_tables.h>
#include <Hal/percpu.h>
#include <Hal/smp.h>
#include <Hal/smpboot.h>
#include <Ke/context.h>
#include <Ke/idle.h>
#include <Ke/log.h>
#include <Ke/smp.h>
#include <Ke/time.h>
#include <Mm/page_alloc.h>
#include <Mm/vmap.h>
#include <asm/apic-def.h>
#include <asm/apic.h>
#include <asm/clear_page.h>
#include <asm/cpufeature.h>
#include <asm/creg_access.h>
#include <asm/idt.h>
#include <asm/msraccess.h>
#include <asm/msr_bits.h>
#include <asm/pg_bits.h>
#include <davix/bug.h>
#include <string.h>
#include "irq/internal.h"
#include "time/internal.h"

extern "C" char trampoline_page[];
extern "C" char trampoline_page_end[];

static inline MMPTE alloc_page_table(int level)
{
	MMPFN *pfn = MmAllocatePage();
	if (!pfn)
		KePanic("HalPrepareSMPBringup: out of memory!");

	unsigned long phys = MmGetPhysForPFN(pfn);
	clear_page((void *) MiPhysToVirt(phys));
	return HalMakeTableKPTE(level, phys);
}

/**
 * HalPrepareSMPBringup - prepare for bringing an SMP system online.
 */
void HalPrepareSMPBringup(void)
{
	if (keProcessorCount != 1 && HalTrampolineAddress == 0)
		KePanic("HalPrepareSMPBringup: no trampoline was allocated!");
	/*
	 * Allocate percpu areas, copy the trampoline contents, etc....
	 */
	for (unsigned int cpu = 1; cpu < keProcessorCount; cpu++) {
		HalAllocateAndInitializePerCPUVariables(cpu);
	}

	unsigned long start = (unsigned long) trampoline_page;
	unsigned long end = (unsigned long) trampoline_page_end;
	asm("" : "+r"(start), "+r"(end));

	unsigned long size = end - start;
	BUG_ON(size > 0x800);

	unsigned long virt = MiPhysToVirt(HalTrampolineAddress);
	memcpy((void *) virt, trampoline_page, size);

	*(uint32_t *) (virt + 4) = HalTrampolineAddress;

	unsigned long trampoline_cr3 = read_cr3();
	BUG_ON((unsigned long) (unsigned int) trampoline_cr3 != trampoline_cr3);

	*((unsigned int *) (virt + 0x800)) = __cr4_state;
	*((unsigned int *) (virt + 0x804)) = __efer_state;
	*((unsigned int *) (virt + 0x808)) = __cr0_state;
	*((unsigned int *) (virt + 0x80c)) = trampoline_cr3;

	/*
	 * Setup a true identity mapping for the trampoline page.
	 */
	for (int i = HalNumPageTableLevels(); i >= 1; i--) {
		MMPTEP ptep = MmGetPteForAddressLevel(HalTrampolineAddress, i);
		if (i == 1) {
			MMPTE pte = HalMakeKPTE(1, HalTrampolineAddress, PTEFLAGS_READWRITE);
			pte.value &= ~(__PG_GLOBAL | __PG_NX);
			HalWritePTE(ptep, pte);
			break;
		}

		MMPTE pte = HalReadPTE(ptep);
		if (HalPTEEmpty(pte)) {
			pte = alloc_page_table(i);
			HalWritePTE(ptep, pte);
		}
	}
}

static bool sync_point_0;
static bool sync_point_1;
static bool sync_point_2;
static bool sync_point_3;
static bool sync_point_4;

static void udelay(usec_t us)
{
	usec_t now = KeMicrosSinceBoot();
	usec_t target = now + us;
	do {
		smp_spinwait_hint();
		now = KeMicrosSinceBoot();
	} while (now < target);
}

static bool startup_processor_via_init_sipi(unsigned int cpu, unsigned int vector)
{
	unsigned int apicid = halCpuToApic[cpu];
	KePrintf("startup_processor_via_init_sipi: apicid=%u vector=%u\n", apicid, vector);

	apic_send_IPI(APIC_DM_INIT | APIC_LEVEL_TRIGGERED | APIC_LEVEL_ASSERT, apicid);
	udelay(10000);
	apic_send_IPI(APIC_DM_INIT | APIC_LEVEL_TRIGGERED, apicid);
	smp_mb();

	for (int i = 0; i < 2; i++) {
		apic_send_IPI(APIC_DM_SIPI | vector, apicid);
		udelay(i ? 10000 : 300);

		smp_mb();
		if (atomic_load_relaxed(&sync_point_0))
			return true;
	}

	return false;
}

extern "C" void HalStartupAdditionalProcessor(void)
{
	smp_mb();
	/*
	 * Signal to startup_processor_via_init_sipi that we are alive.
	 */
	atomic_store_relaxed(&sync_point_0, true);
	do	/*
		 * Wait for HalTryToStartProcessor to set sync_point_1 to true.
		 */
		smp_spinwait_hint();
	while (!atomic_load_relaxed(&sync_point_1));
	/*
	 * Synchronize the TSC.
	 */
	HalSynchronizeTSC(false);

	load_idt_table();
	HalInitializeLocalAPICNonBSP();

	/*
	 * Signal to HalTryToStartProcessor that it can proceed to mark us
	 * online.
	 */
	atomic_store_release(&sync_point_2, true);
	do	/*
		 * Wait for HalTryToStartProcessor to let us continue.
		 */
		smp_spinwait_hint();
	while (!atomic_load_acquire(&sync_point_3));
	atomic_store_release(&sync_point_4, true);
	/*
	 * Enter the CPU idle loop now.
	 */
	HalEnableRawIRQs();
	KePrintf("Hello from CPU%u!\n", HalCurrentProcessor());
	KeCPUIdleLoop();
}

extern "C" { unsigned long __ap_startup_rsp; }

static unsigned int currently_booting;

static bool smpboot_killed = false;

extern "C" void HalSmpBootInitializePerCPU(void)
{
	write_msr(MSR_GSBASE, Hal::percpu_offsets[currently_booting]);
}

/**
 * HalTryToStartProcessor - try to start an additional processor.
 * @cpu: CPU number of the processor to start
 */
bool HalTryToStartProcessor(unsigned int cpu)
{
	if (smpboot_killed)
		return false;

	void *stack = MmAllocateVirtual(0x4000, PTEFLAGS_READWRITE);
	if (!stack) {
		KePrintf("HalTryToStartProcessor: couldn't allocate memory\n");
		smpboot_killed = true;
		return false;
	}

	__ap_startup_rsp = (unsigned long) stack + 0x4000;

	currently_booting = cpu;
	sync_point_0 = false;
	sync_point_1 = false;
	sync_point_2 = false;
	sync_point_3 = false;
	sync_point_4 = false;

	/*
	 * Startup the processor via INIT-SIPI.  Returns false if startup fails.
	 */
	if (!startup_processor_via_init_sipi(cpu, HalTrampolineAddress >> 12)) {
		KePrintf("HalTryToStartProcessor: failed to startup CPU%u via INIT-SIPI sequence!\n", cpu);
		smpboot_killed = true;
		return false;
	}

	/*
	 * If startup_processor_via_init_sipi returns true we have observed
	 * sync_point_0 being true, which means that the other CPU is waiting
	 * for us to tell it to continue.
	 */
	atomic_store_relaxed(&sync_point_1, true);

	/*
	 * Synchronize the TSC now.
	 */
	HalSynchronizeTSC(true);

	/*
	 * Wait for the target CPU to be ready for onlining.
	 */
	while (!atomic_load_acquire(&sync_point_2))
		smp_spinwait_hint();

	/*
	 * Mark the CPU online.  Processor bringup is now done.
	 */
	KeSetCPUOnline(cpu);
	atomic_store_release(&sync_point_3, true);

	do	/*
		 * Make sure the target CPU sees sync_point_3 == true.
		 */
		smp_spinwait_hint();
	while (!atomic_load_relaxed(&sync_point_4));

	return true;
}

/**
 * HalSmpStartProcessors - start additional processors in an SMP system.
 */
void HalSmpStartProcessors(void)
{
	if (keProcessorCount > 1) {
		KePrintf("Bringing up to %u additional processor(s) online\n", keProcessorCount - 1);

		unsigned int num_onlined = 0;
		for (unsigned int cpu = 1; cpu < keProcessorCount; cpu++) {
			if (!KeCPUPresent(cpu))
				continue;

			if (KeCPUOnline (cpu))
				continue;

			KeDisableDPCs();
			if (HalTryToStartProcessor(cpu))
				num_onlined++;
			KeEnableDPCs();
		}

		KePrintf("Brought %u additional processors online\n", num_onlined);
	}
}

