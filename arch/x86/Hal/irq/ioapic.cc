// SPDX-License-Identifier: GPL-3.0
/*
 * File: Hal/irq/ioapic.cc
 *
 * Copyright (C) 2025  dbstream
 */
#include <Hal/mm.h>
#include <Hal/mmio.h>
#include <Ke/log.h>
#include <Ke/spinlock.h>
#include <Mm/pool.h>
#include <Mm/vmap.h>
#include "internal.h"

/*
 * Legacy IRQ mapping
 *
 * This table corresponds to the "interrupt source override" information present
 * in ACPI tables. If none of (force_active_hi, force_active_lo) are set, the
 * default device polarity is used. If none of (force_tgm_edge, force_tgm_level)
 * are set, the device default trigger mode is used.
 */
struct legacy_irq_mapping {
	unsigned int gsi;
	bool force_active_hi;
	bool force_active_lo;
	bool force_tgm_edge;
	bool force_tgm_level;
} legacy_irq_mapping[16] = {
	{ 0, 0, 0, 0, 0 },
	{ 1, 0, 0, 0, 0 },
	{ 2, 0, 0, 0, 0 },
	{ 3, 0, 0, 0, 0 },
	{ 4, 0, 0, 0, 0 },
	{ 5, 0, 0, 0, 0 },
	{ 6, 0, 0, 0, 0 },
	{ 7, 0, 0, 0, 0 },
	{ 8, 0, 0, 0, 0 },
	{ 9, 0, 0, 0, 0 },
	{ 10, 0, 0, 0, 0 },
	{ 11, 0, 0, 0, 0 },
	{ 12, 0, 0, 0, 0 },
	{ 13, 0, 0, 0, 0 },
	{ 14, 0, 0, 0, 0 },
	{ 15, 0, 0, 0, 0 },
};

void HalAddIRQOverride(unsigned int source, unsigned int dest,
		bool force_active_hi, bool force_active_lo,
		bool force_tgm_edge, bool force_tgm_level)
{
	if (source >= 16)
		return;

	legacy_irq_mapping[source] = (struct legacy_irq_mapping) {
		dest, force_active_hi, force_active_lo,
		force_tgm_edge, force_tgm_level
	};
}

void HalPrintIRQOverrideTable(void)
{
	KePrintf("IOAPIC: ISA IRQ mapping:\n");
	KePrintf(".. irq -> gsi  active_hi active_lo tgm_edge tgm_level\n");
	for (unsigned int i = 0; i < 16; i++) {
		KePrintf(".. %3u -> %3u %9s %9s %8s %9s\n",
				i, legacy_irq_mapping[i].gsi,
				legacy_irq_mapping[i].force_active_hi ? "yes" : "no",
				legacy_irq_mapping[i].force_active_lo ? "yes" : "no",
				legacy_irq_mapping[i].force_tgm_edge ? "yes" : "no",
				legacy_irq_mapping[i].force_tgm_level ? "yes" : "no");
	}
}

struct IOAPIC {
	unsigned long mapping;
	KeIRQSpinlock lock;
};

static inline uint32_t ioapic_read(IOAPIC *apic, uint32_t offset)
{
	HalMMIOWrite32((uint32_t *) apic->mapping, offset);
	return HalMMIORead32((uint32_t *) (apic->mapping + 0x10));
}

static inline void ioapic_write(IOAPIC *apic, uint32_t offset, uint32_t value)
{
	HalMMIOWrite32((uint32_t *) apic->mapping, offset);
	HalMMIOWrite32((uint32_t *) (apic->mapping + 0x10), value);
}

#define IOAPICID		0x00
#define IOAPICVER		0x01
#define IOAPICARB		0x02
#define IOAPICREDTBL(x)		(0x10 + 2 * x)

void HalAddIOAPIC(unsigned long address, unsigned int gsi_base)
{
	IOAPIC *apic = MmNew<IOAPIC>();
	if (!apic)
		KePanic("Out of memory!");

	apic->mapping = (unsigned long)
		MmMapVirtual(address, PAGE_SIZE, PTEFLAGS_IOUNCACHED);
	if (!apic->mapping)
		KePanic("Failed to map IOAPIC at address 0x%lx!\n", address);

	uint32_t ioapicver = ioapic_read(apic, IOAPICVER);
	unsigned int num_gsis = ((ioapicver >> 16) & 0xff) + 1;

	KePrintf("IOAPIC: address 0x%lx GSIs %u-%u\n",
			address,
			gsi_base,
			gsi_base + num_gsis - 1
	);
}

