/* SPDX-License-Identifier: MIT */
#ifndef __ASM_APIC_H
#define __ASM_APIC_H

/*
 * APIC interface functions, only safe to call
 * from interrupt-disabled environments.
 */
struct apic {
	/*
	 * Initialize the local APIC. This should only be called by
	 * the BSP once at early boot.
	 */
	void (*init)(void);

	/*
	 * Initialize the local APIC on an application processor. This
	 * should be called when an application processor has just been
	 * brought online.
	 *
	 * NOTE: This includes the BSP if it is offlined and onlined by
	 * CPU hotplug support.
	 */
	void (*init_other);

	/*
	 * Read the APIC ID.
	 */
	unsigned (*read_id)(void);
};

#define APIC_ID 0x20
#define APIC_VERSION 0x30
#define APIC_EOI 0xb0
#define APIC_LOGICAL_DST 0xd0
#define APIC_DST_FMT 0xe0
#define APIC_SPR_VECTOR 0xf0
#define APIC_ESR 0x280
#define APIC_ICR_LOW 0x300
#define APIC_ICR_HIGH 0x310
#define APIC_TIMER 0x320
#define _APIC_TIMER_PERIODIC (1 << 17)
#define APIC_LINT0 0x350
#define APIC_LINT1 0x360
#define APIC_LVT_ERROR 0x370
#define APIC_TIMER_INITIAL_COUNT 0x380
#define APIC_TIMER_COUNT 0x390
#define APIC_TIMER_DIV 0x3e0

extern struct apic xapic_impl;
extern struct apic x2apic_impl;

extern struct apic *active_apic_interface;
extern unsigned long apic_base;

struct acpi_table_madt;

void apic_init(struct acpi_table_madt *madt);

static inline unsigned apic_read_id(void)
{
	return active_apic_interface->read_id();
}

#endif /* __ASM_APIC_H */
