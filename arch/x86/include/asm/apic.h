/* SPDX-License-Identifier: MIT */
#ifndef __ASM_APIC_H
#define __ASM_APIC_H

#include <davix/types.h>

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

	u32 (*read)(unsigned long offset);
	void (*write)(unsigned long offset, u32 value);

	void (*write_icr)(u32 value, u32 dst);
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

#define APIC_TGM_LEVEL (1 << 15)
#define APIC_POLARITY_LOW (1 << 13)

#define APIC_DM_FIXED (0)
#define APIC_DM_SMI (2 << 8)
#define APIC_DM_NMI (4 << 8)
#define APIC_DM_INIT (5 << 8)
#define APIC_DM_ExtINT (7 << 8)

#define APIC_IRQ_MASKED (1 << 16) /* masked - i.e. disabled */

#define APIC_TIMER_MASKED (1 << 16) /* masked - i.e. disabled */

#define APIC_TIMER_ONESHOT (0)
#define APIC_TIMER_PERIODIC (1 << 17)
#define APIC_TIMER_DEADLINE (2 << 17)

extern struct apic xapic_impl;
extern struct apic x2apic_impl;

extern struct apic *active_apic_interface;
extern unsigned long apic_base;

struct acpi_table_madt;

void apic_init(struct acpi_table_madt *madt);

void apic_configure(void); /* configure the APIC and start the timer */

static inline unsigned apic_read_id(void)
{
	return active_apic_interface->read_id();
}

static inline u32 apic_read(unsigned long offset)
{
	return active_apic_interface->read(offset);
}

static inline void apic_write(unsigned long offset, u32 value)
{
	active_apic_interface->write(offset, value);
}

static inline void apic_write_icr(u32 value, u32 dst)
{
	active_apic_interface->write_icr(value, dst);
}

static inline void apic_send_eoi(void)
{
	active_apic_interface->write(APIC_EOI, 0);
}

/*
 * ICR::Delivery Mode
 */
#define APIC_ICR_FIXED (0 << 8)
#define APIC_ICR_SMI (2 << 8)
#define APIC_ICR_NMI (4 << 8)
#define APIC_ICR_INIT (5 << 8)
#define APIC_ICR_SIPI (6 << 8)

/*
 * ICR::Level and ICR::Trigger Mode
 */
#define APIC_ICR_ASSERT (1 << 14)
#define APIC_ICR_LEVEL (1 << 15)

/*
 * ICR::Destination Shorthand
 */
#define APIC_ICR_SELF (1 << 18)

#endif /* __ASM_APIC_H */
