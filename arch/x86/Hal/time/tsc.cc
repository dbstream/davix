// SPDX-License-Identifier: GPL-3.0
/*
 * File: Hal/time/tsc.cc
 * Time Stamp Counter and HalNanosSinceBoot
 *
 * Copyright (C) 2025  dbstream
 */
#include <Hal/percpu.h>
#include <Hal/time.h>
#include <Ke/context.h>
#include <Ke/log.h>
#include <davix/atomic.h>
#include <davix/export.h>
#include <asm/cpufeature.h>
#include <asm/rdtsc.h>
#include "internal.h"

static unsigned long long read_tsc_ref(unsigned long long *ref)
{
	constexpr unsigned long long max_delta = 100000UL;
	constexpr unsigned long long good_delta = 1000UL;

	unsigned long long besttsc, bestref, bestdelta = -1UL;
	for (int i = 0; i < 10; i++) {
		uint64_t tsc1 = rdtsc_strong();
		uint64_t ref = HalHPETReadCounter();
		uint64_t tsc2 = rdtsc_strong();

		uint64_t delta = tsc2 - tsc1;
		if (delta < bestdelta) {
			besttsc = tsc2;
			bestref = ref;
			bestdelta = delta;
		}
		if (bestdelta < good_delta)
			break;
	}

	if (bestdelta > max_delta) {
		KePrintf("warning: read_tsc_ref: TSC calibration failed\n");
		return 0;
	}

	*ref = bestref;
	return besttsc;
}

static void tsc_ref_mdelay(unsigned long long ref, unsigned long long ms)
{
	unsigned long long delta = HalHPETNanosToCounter(ms * 1000UL * 1000UL);
	unsigned long long target = ref + delta;

	do {
		smp_spinwait_hint();
	} while(HalHPETReadCounter() < target);
}

static unsigned long long tsc_ref_to_ns(unsigned long long ref)
{
	return HalHPETCounterToNanos(ref);
}

static unsigned long long calculate_tsc_khz(
		unsigned long long tsc_delta,
		unsigned long long ns_delta
)
{
	tsc_delta *= 1000000UL;
	tsc_delta /= ns_delta;
	return tsc_delta;
}

struct tsc2ns_data {
	unsigned long long khz;
	unsigned long long offset;
};

static inline unsigned long long tsc2ns(unsigned long long tsc, tsc2ns_data conv)
{
	return (1000000ULL * (tsc + conv.offset)) / conv.khz;
}

struct tsc_cpu_data {
	tsc2ns_data conv[2];
	unsigned long generation;
};

static DEFINE_PERCPU(tsc_cpu_data, tsc_cpu);

static void set_local_tsc2ns(tsc2ns_data conv)
{
	unsigned long next = HalReadPerCPU(tsc_cpu.generation) + 1;
	HalWritePerCPU(tsc_cpu.conv[next & 1], conv);
	barrier();
	HalWritePerCPU(tsc_cpu.generation, next);
}

static unsigned long long read_tsc_conv(tsc2ns_data *conv)
{
	uint64_t tsc;
	uint64_t gen1, gen2 = HalReadPerCPU(tsc_cpu.generation);
	do {
		barrier();
		gen1 = gen2;
		tsc = rdtsc();
		*conv = HalReadPerCPU(tsc_cpu.conv[gen1 & 1]);
		barrier();
		gen2 = HalReadPerCPU(tsc_cpu.generation);
	} while (gen1 != gen2);
	return tsc;
}

static tsc2ns_data calculate_tsc2ns(
		unsigned long long khz,
		unsigned long long tsc,
		unsigned long long ns
)
{
	tsc2ns_data conv;
	conv.khz = khz;
	conv.offset = (ns / 1000UL) * (khz / 1000UL) - tsc;

	return conv;
}

static unsigned long long tsc_nsecs(void)
{
	unsigned long long tsc;
	tsc2ns_data conv;

	KeDisablePreemption();
	tsc = read_tsc_conv(&conv);
	KeEnablePreemption();

	return tsc2ns(tsc, conv);
}

static unsigned long long tsc_khz = 0;

static void calibrate_early(void)
{
	unsigned long long tsc1, tsc2, ref1, ref2;

	tsc1 = read_tsc_ref(&ref1);
	if (!tsc1)
		return;

	tsc_ref_mdelay(ref1, 50);

	tsc2 = read_tsc_ref(&ref2);
	if (!tsc2)
		return;

	unsigned long long tsc_delta = tsc2 - tsc1;
	unsigned long long ref_delta = ref2 - ref1;
	unsigned long long ns_delta = tsc_ref_to_ns(ref_delta);
	if (ns_delta < 50000000ULL) {
		KePrintf("warning: TSC: calibrate_early: 50ms wait failed (ns_delta=%llu)\n",
				ns_delta);
		return;
	}

	tsc_khz =  calculate_tsc_khz(tsc_delta, ns_delta);
	KePrintf("Early TSC calibration using HPET: %llu.%03lluMHz\n",
			tsc_khz / 1000, tsc_khz % 1000);
	if (tsc_khz < 1000ULL) {
		KePrintf("warning: TSC: unreasonably slow, disabling it.\n");
		tsc_khz = 0;
		return;
	}

	set_local_tsc2ns(calculate_tsc2ns(tsc_khz, tsc2, tsc_ref_to_ns(ref2)));
	HalUseTSC = true;
}

void HalInitializeTSC(void)
{
	if (!CPUFeature(TSC)) {
		if (!HalUseHPETTimer)
			KePanic("No supported system timer is available! (No TSC, HPET)");
		return;
	}

	if (!HalUseHPETTimer)
		KePanic("No supported timer for TSC calibration is available! (No HPET)");

	calibrate_early();
}

bool HalUseHPETTimer = false;
bool HalUseTSC = false;

unsigned long long HalNanosSinceBoot(void)
{
	if (HalUseTSC) {
		return tsc_nsecs();
	}

	if (HalUseHPETTimer) {
		return HalHPETNanosSinceBoot();
	}

	return 0; // fallback value
}
EXPORT_SYMBOL(HalNanosSinceBoot);

