// SPDX-License-Identifier: GPL-3.0
/*
 * File: Hal/irq/apic.cc
 * Integrated Local APIC (xAPIC) and Local x2APIC driver.
 *
 * Copyright (C) 2025  dbstream
 */
#include <Hal/fixed_mapping.h>
#include <Hal/interrupt.h>
#include <Hal/irq_vectors.h>
#include <Hal/mmio.h>
#include <Ke/log.h>
#include <Ke/time.h>
#include <asm/apic-def.h>
#include <asm/apic.h>
#include <asm/cpufeature.h>
#include <asm/msr_bits.h>
#include <asm/msraccess.h>
#include <davix/atomic.h>
#include "internal.h"

static constexpr unsigned long xAPIC_VIRTUAL_BASE = HalFixedMappingAddress(
		HAL_FIXED_MAP_LOCAL_APIC
);

static constexpr int APIC_MSR(int reg)
{
	return 0x800 + (reg >> 4);
}

static volatile uint32_t *APIC_REG(int reg)
{
	return (volatile uint32_t *) (xAPIC_VIRTUAL_BASE +  reg);
}

static inline uint32_t apic_read(int reg)
{
	if (CPUFeature(X2APIC))
		return read_msr(APIC_MSR(reg));
	else
		return HalMMIORead32(APIC_REG(reg));
}

static inline void apic_write(int reg, uint32_t value)
{
	if (CPUFeature(X2APIC))
		write_msr(APIC_MSR(reg), value);
	else
		HalMMIOWrite32(APIC_REG(reg), value);
}

uint32_t apic_read_id(void)
{
	if (CPUFeature(X2APIC))
		return read_msr(APIC_MSR(APIC_ID));
	else
		return HalMMIORead32(APIC_REG(APIC_ID)) >> 24;
}

static inline void apic_write_ICR(uint32_t value, uint32_t apicid)
{
	if (CPUFeature(X2APIC)) {
		write_msr(
				APIC_MSR(APIC_ICR_LOW),
				value | ((uint64_t) apicid << 32)
		);
	} else {
		HalMMIOWrite32(APIC_REG(APIC_ICR_HIGH), apicid << 24);
		HalMMIOWrite32(APIC_REG(APIC_ICR_LOW), value);

		while (HalMMIORead32(APIC_REG(APIC_ICR_LOW)) & APIC_IRQ_PENDING)
			smp_spinwait_hint();
	}
}

void apic_send_IPI(uint32_t value, uint32_t target_apicid)
{
	apic_write_ICR(value, target_apicid);
}

void HalAcknowledgeInterrupt(void)
{
	apic_write(APIC_EOI, 0);
}

static unsigned long xAPIC_base;

static void setup_apic_base_xAPIC(void)
{
	write_msr(MSR_APIC_BASE, xAPIC_base | _APIC_BASE_ENABLED);
}

static void setup_apic_base_x2APIC(void)
{
	/*
	 * Local APIC mode transitions from no APIC directly to x2APIC are
	 * forbidden, as are APIC mode transitions back to xAPIC mode.
	 * Therefore we need to look at what mode the BIOS and bootloader left
	 * us in.
	 */
	uint64_t state = read_msr(MSR_APIC_BASE);
	if (!(state & _APIC_BASE_ENABLED))
		write_msr(MSR_APIC_BASE, xAPIC_base | _APIC_BASE_ENABLED);

	write_msr(
			MSR_APIC_BASE,
			xAPIC_base | _APIC_BASE_ENABLED | _APIC_BASE_X2APIC
	);
}

static void setup_apic_base(void)
{
	if (CPUFeature(X2APIC))
		setup_apic_base_x2APIC();
	else
		setup_apic_base_xAPIC();	
}

static void reset_apic(void)
{
	/* Soft-disable and then soft-enable the Local APIC. */
	apic_write(APIC_SPIV, 0);
	apic_write(APIC_SPIV, IRQ_VECTOR_APIC_SPURIOUS | APIC_SPIV_ENABLE_APIC);
}

static uint64_t apic_khz = 0;

static void calibrate_apic(void)
{
	/*
	 * Disable the APIC timer.
	 */
	apic_write(APIC_TMR_ICR, 0);

	/*
	 * Setup the APIC clock divisor to 16.
	 *
	 * Mapping from value to timer timer divide:
	 *  0 - divide by 2
	 *  1 - divide by 4
	 *  2 - divide by 8
	 *  3 - divide by 16
	 *  8 - divide by 32
	 *  9 - divide by 64
	 * 10 - divide by 128
	 * 11 - divide by 1
	 */
	apic_write(APIC_TMR_DIV, 3);

	/*
	 * Configure the APIC Timer for oneshot mode and disable the interrupt.
	 */
	apic_write(APIC_LVTTMR, IRQ_VECTOR_APIC_TIMER | APIC_IRQ_MASK);

	nsec_t t0 = KeNanosSinceBoot();
	if (!t0) {
		[[unlikely]];
		KePrintf("warning: no reference timer for APIC calibration is available, guessing 1 GHz\n");

		apic_khz = 1000000UL;
		return;
	}

	/*
	 * Start the APIC timer countdown.
	 */
	apic_write(APIC_TMR_ICR, 0xffffffffU);

	/*
	 * Wait for 100ms.
	 */
	nsec_t t1, target = t0 + 100UL * 1000UL * 1000UL;
	do {
		smp_spinwait_hint();
		t1 = KeNanosSinceBoot();
	} while (t1 < target);
	uint32_t ccr = apic_read(APIC_TMR_CCR);

	/*
	 * Disable the APIC timer.
	 */
	apic_write(APIC_TMR_ICR, 0);

	uint64_t elapsed = 0xffffffffUL - ccr;
	nsec_t delta_ns = t1 - t0;

	elapsed *= 16; /* Compensate for APIC_TMR_DIV=3 (16) */

	apic_khz = (1000000UL * elapsed) / delta_ns;
	KePrintf("APIC: calibrated Local APIC Timer frequency to %lu.%03lu MHz\n",
			apic_khz / 1000, apic_khz % 1000);
}

static void setup_timer_periodic(void)
{
	apic_write(APIC_TMR_ICR, 0);
	apic_write(APIC_TMR_DIV, 3);
	apic_write(APIC_LVTTMR, IRQ_VECTOR_APIC_TIMER | APIC_TMR_PERIODIC);
	apic_write(APIC_TMR_ICR, apic_khz / 16);
}

void HalInitializeLocalAPIC(void)
{
	/*
	 * FIXME: is 0xfee00000 always a good value for MSR_APIC_BASE?
	 */
	xAPIC_base = 0xfee00000UL;

	KePrintf("xAPIC_base=0x%lx\n", xAPIC_base);
	HalSetFixedMapping(
			HAL_FIXED_MAP_LOCAL_APIC,
			xAPIC_base,
			PTEFLAGS_IOUNCACHED
	);

	if (CPUFeature(X2APIC))
		KePrintf("Using x2APIC\n");
	else
		KePrintf("Using integrated xAPIC\n");

	setup_apic_base();
	reset_apic();
	calibrate_apic();

	setup_timer_periodic();
}

void HalInitializeLocalAPICNonBSP(void)
{
	setup_apic_base();
	reset_apic();
	setup_timer_periodic();
}

