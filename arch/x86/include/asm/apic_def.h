/**
 * Local APIC definitions.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

/**
 * Local APIC registers.
 */
enum {
	APIC_ID			= 0x20,
	APIC_VERSION		= 0x30,
	APIC_TPR		= 0x80,
	APIC_APR		= 0x90,
	APIC_PPR		= 0xa0,
	APIC_EOI		= 0xb0,
	APIC_RRD		= 0xc0,
	APIC_LDR		= 0xd0,
	APIC_DFR		= 0xe0,
	APIC_SPI		= 0xf0,
	APIC_ESR		= 0x280,
	APIC_LVTCMCI		= 0x2f0,
	APIC_ICR_LOW		= 0x300,
	APIC_ICR_HIGH		= 0x310,
	APIC_LVTTMR		= 0x320,
	APIC_LVTTHRM		= 0x330,
	APIC_LVTPMC		= 0x340,
	APIC_LVTLINT0		= 0x350,
	APIC_LVTLINT1		= 0x360,
	APIC_LVTERR		= 0x370,
	APIC_TMR_ICR		= 0x380,
	APIC_TMR_CCR		= 0x390,
	APIC_TMR_DIV		= 0x3e0
};

/**
 * Local APIC interrupt delivery modes.
 */
enum : unsigned int {
	APIC_DM_FIXED		= 0U << 8,
	APIC_DM_LOWPRI		= 1U << 8,
	APIC_DM_SMI		= 2U << 8,
	APIC_DM_NMI		= 4U << 8,
	APIC_DM_INIT		= 5U << 8,
	APIC_DM_SIPI		= 6U << 8
};

/**
 * Various local APIC flags.
 */
enum : unsigned int {
	APIC_DST_LOGICAL	= 1U << 11,
	APIC_IRQ_PENDING	= 1U << 12,
	APIC_POLARITY_LOW	= 1U << 13,
	APIC_LEVEL_ASSERT	= 1U << 14,
	APIC_LEVEL_TRIGGERED	= 1U << 15,
	APIC_IRQ_MASK		= 1U << 16,
	APIC_TMR_PERIODIC	= 1U << 17
};

/**
 * Local APIC destination shorthands.
 */
enum : unsigned int {
	APIC_DST_SELF		= 1U << 18,
	APIC_DST_ALL		= 2U << 18,
	APIC_DST_OTHERS		= 3U << 18
};

/**
 * APIC_SPI flags.
 */
enum : unsigned int {
	APIC_SPI_ENABLE		= 1U << 8,
	APIC_SPI_FCC_ENABLE	= 1U << 9
};
