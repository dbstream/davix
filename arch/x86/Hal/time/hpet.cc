// SPDX-License-Identifier: GPL-3.0
/*
 * File: Hal/time/hpet.cc
 * HPET for timekeeping
 *
 * Copyright (C) 2025  dbstream
 */
#include <Acpi/tables.h>
#include <Hal/fixed_mapping.h>
#include <Hal/mm.h>
#include <Hal/mmio.h>
#include <Hal/smp.h>
#include <Ke/context.h>
#include <Ke/log.h>
#include <davix/atomic.h>
#include "internal.h"

static constexpr unsigned long HPET_BASE = HalFixedMappingAddress(HAL_FIXED_MAP_HPET);

static constexpr uint64_t HPET_32BIT_MASK = 0xffffffffUL;

static inline uint64_t hpet_read(int reg)
{
	return HalMMIORead64((uint64_t *) (HPET_BASE + reg));
}

static inline void hpet_write(int reg, uint64_t value)
{
	HalMMIOWrite64((uint64_t *) (HPET_BASE + reg), value);
}

enum {
	HPET_CAP_ID		= 0x00,
	HPET_CONF		= 0x10,
	HPET_IRQ_STATUS		= 0x20,
	HPET_MAIN_COUNTER	= 0xf0,
};

static constexpr int HPET_TMR_CONF_CAP(int i)
{
	return 0x100 + 0x20 * i;
}

static bool hpet_is_32bit;

static uint32_t hpet_period;
static uint64_t hpet_frequency;

static uint32_t hpet_period_ns;
static uint32_t hpet_period_fraction;

/*
 * Value reuse and 64-bit HPET counter emulation.
 *
 * When the underlying HPET device is 32-bit, the timer counter value will wrap
 * around after 2^32 ticks.  At any point in time, we know that the unwrapped
 * value must be equal to:
 *
 *	2^32 * k + 32-bit counter value
 *
 * for some integer k.  We keep track of this value in the upper 32 bits of a
 * variable which stores the last observed unwrapped value.  Whenever we see
 * time go backwards, we increment these high 32 bits thereby achieveing a
 * monotonically increasing 64-bit counter value.
 *
 * Regardless of HPET counter bitness, we keep track of the last unwrapped HPET
 * counter value that has been read by a CPU.  __hpet_read_counter is protected
 * by a simple lock which a CPU is allowed to grab nested.  If another CPU then
 * comes along and wants to read the HPET while it is locked, it will wait for
 * the lock to be released and then immediately return the value stored in
 * hpet_data.last_read instead of initiating its own HPET read.  By reusing the
 * previously read counter values in this way, CPUs don't queue up on reading
 * the HPET main counter, which is a _slow_ operation.
 */

static struct {
	uint64_t last_read = 0;
	uint32_t lock = -1U;
} hpet_data;

/**
 * __hpet_read_counter - _actually_ read the HPET main counter register.
 */
static inline uint64_t __hpet_read_counter(void)
{
	uint64_t last_read = atomic_load_relaxed(&hpet_data.last_read);
	uint64_t value = hpet_read(HPET_MAIN_COUNTER);

	if (hpet_is_32bit) {
		value &= HPET_32BIT_MASK;
		if (value < last_read)
			/* Time doesn't go backwards. */
			value += 1UL << 32;
	}

	atomic_cmpxchg(&hpet_data.last_read, &last_read, value,
			_MO_Relaxed, _MO_Relaxed);
	return value;
}

/**
 * hpet_read_counter - get a recent HPET counter value.
 */
static inline uint64_t hpet_read_counter(void)
{
	KeDisableIRQs();
	unsigned int me = HalCurrentProcessor();
	unsigned int lockval = -1U;
	if (!atomic_cmpxchg_for_lock(&hpet_data.lock, &lockval, me)) {
		if ((lockval & ~0x80000000U) == me) {
			/*
			 * The lock is currently held by this processor so
			 * consider this a nested grab.
			 */
			uint64_t counter = __hpet_read_counter();
			/*
			 * After reading the HPET counter value, set the highest
			 * bit of the lock word to signal to other CPUs that the
			 * value in hpet_data.last_read is "recent".
			 */
			if (!(lockval & 0x80000000U)) {
				lockval |= 0x80000000;
				atomic_store_release(&hpet_data.lock, lockval);
			}
			KeEnableIRQs();
			return counter;
		}
		KeEnableIRQs();
		while (!(lockval & 0x80000000)) {
			/*
			 * We don't need to grab the lock here.  Only wait for
			 * the highest bit in the lock word to be set, which
			 * signals that the value in hpet_data.last_read is
			 * "recent".
			 */
			smp_spinwait_hint();
			lockval = atomic_load_relaxed(&hpet_data.lock);
		}
		smp_rmb();
		return atomic_load_relaxed(&hpet_data.last_read);
	}
	uint64_t counter = __hpet_read_counter();
	atomic_store_release(&hpet_data.lock, -1U);
	KeEnableIRQs();
	return counter;
}

void HalInitializeHPET(void)
{
	void *hpet_table_ptr;
	size_t hpet_table_id;
	if (!AcpiGetTable(ACPI_HPET_SIGNATURE, &hpet_table_ptr, &hpet_table_id))
		return;

	acpi_hpet *hpet_table = (acpi_hpet *) hpet_table_ptr;
	uint8_t asid = hpet_table->address.address_space_id;
	unsigned long address = hpet_table->address.address;
	AcpiPutTable(hpet_table_ptr, hpet_table_id);

	if (asid != ACPI_AS_ID_SYS_MEM) {
		KePrintf("warning: HPET is not in System Memory space\n");
		return;
	}

	if (address & (PAGE_SIZE - 1)) {
		KePrintf("warning: HPET address is 0x%lx, which is unaligned\n", address);
		return;
	}

	HalSetFixedMapping(HAL_FIXED_MAP_HPET, address, PTEFLAGS_IOUNCACHED);

	uint64_t cap_id = hpet_read(HPET_CAP_ID);
	hpet_is_32bit = !(cap_id & (1UL << 13));

	hpet_period = cap_id >> 32;
	int num_comparators = ((cap_id >> 8) & 31) + 1;
	/*
	 * Reset the HPET configuration register and main counter register.
	 */
	hpet_write(HPET_CONF, 0);
	hpet_write(HPET_MAIN_COUNTER, 0);
	/*
	 * Mask the HPET comparator registers.
	 */
	for (int i = 0; i < num_comparators; i++)
		hpet_write(HPET_TMR_CONF_CAP(i), 0);

	hpet_period_ns = hpet_period / 1000000U;
	hpet_period_fraction = hpet_period % 1000000U;
	hpet_frequency = 1000000000000000UL / hpet_period;

	KePrintf("HPET: period=%u.%03uns bits=%u frequency=%luMHz\n",
			hpet_period_ns, hpet_period_fraction / 1000,
			hpet_is_32bit ? 32 : 64,
			hpet_frequency / 1000000);
	/*
	 * Require the HPET counter period to be at least 0.1ns and at most
	 * 100ns.  Anything outside of this range is not sane.
	 *
	 * For a 32-bit HPET with a period smaller than 0.1ns, the main counter
	 * register would overflow in ~0.429 seconds, which is not acceptable.
	 */
	if(hpet_period < 100000U || hpet_period > 100000000U) {
		KePrintf("warning: HPET: period is unsane\n");
		return;
	}
	/*
	 * Start the HPET timer.
	 */
	hpet_write(HPET_CONF, 1);
	HalUseHPETTimer = true;
}

static inline unsigned long long hpet2nsec(uint64_t value)
{
	uint64_t ns = hpet_period_ns * value;
	ns += (hpet_period_fraction * value) / 1000000;
	return ns;
}

unsigned long long HalHPETNanosSinceBoot(void)
{
	return hpet2nsec(hpet_read_counter());
}

unsigned long long HalHPETReadCounter(void)
{
	return hpet_read_counter();
}

unsigned long long HalHPETNanosToCounter(unsigned long long nsecs)
{
	return (nsecs * 1000UL * 1000UL) / hpet_period;
}

unsigned long long HalHPETCounterToNanos(unsigned long long counter)
{
	return hpet2nsec(counter);
}

