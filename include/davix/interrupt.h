/**
 * Interrupt handling.
 * Copyright (C) 2025-present  dbstream
 */
#ifndef _DAVIX_INTERRUPT_H
#define _DAVIX_INTERRUPT_H

struct interrupt_handler;

#include <davix/errno.h>
#include <davix/list.h>

/**
 * Values for irqstatus_t.
 *	INTERRUPT_HANDLED	This device had pending work.
 *	INTERRUPT_NOT_HANDLED	This device did not have pending work.
 */
#define INTERRUPT_HANDLED	0
#define INTERRUPT_NOT_HANDLED	1

typedef int irqstatus_t;
typedef irqstatus_t (*irqhandler_fn_t) (struct interrupt_handler *handler);

/**
 * Bits in irqhandler_flags_t.
 *	INTERRUPT_EXCLUSIVE	An interrupt vector must be allocated for use
 *				exclusively by this device.
 *	INTERRUPT_LEGACY	If the architecture has "legacy" interrupt
 *				remapping, apply it.  (This is used on x86 for
 *				example, to remap according to the interrupt
 *				source overrides).
 *	INTERRUPT_ACTIVE_LOW	The interrupt is active-low.
 *	INTERRUPT_LEVEL_TRIGGERED	The interrupt is level-triggered.
 */
#define INTERRUPT_EXCLUSIVE		1
#define INTERRUPT_LEGACY		2
#define INTERRUPT_ACTIVE_LOW		4
#define INTERRUPT_LEVEL_TRIGGERED	8

typedef int irqhandler_flags_t;

struct interrupt_handler {
	irqhandler_fn_t handler;	/** IRQ handler function.  */
	struct list irq_handler_list;	/** linkage for the vector handler list.  */
	unsigned int irq_number;	/** IRQ number.  */
	unsigned int vector_number;	/** vector number.  */
	irqhandler_flags_t flags;	/** flags passed to install_interrupt_handler.  */
	void *ptr1;			/** pointers private to the handler.  */
	void *ptr2;
};

extern errno_t
install_interrupt_handler (struct interrupt_handler *handler,
		unsigned int irq_number, irqhandler_flags_t flags);

extern void
uninstall_interrupt_handler (struct interrupt_handler *handler);

extern void
handle_interrupt (unsigned int vector);

/**
 * Wait for all in-flight interrupt handlers on @vector to finish.
 */
extern void
sync_interrupts (unsigned int vector);

/**
 * alloc_irq_vector and free_irq_vector are used by architecture-specific code
 * to allocate and free IRQ vectors in arch_configure_interrupt_pin.
 */

extern unsigned int
alloc_irq_vector (void);

extern void
free_irq_vector (unsigned int vector);

extern errno_t
arch_configure_interrupt_pin (unsigned int *irq_number, irqhandler_flags_t flags,
		errno_t (*callback) (unsigned int vector, void *), void *arg);

extern void
arch_clear_and_free_interrupt_pin (unsigned int irq_number, unsigned int vector);

#endif /** _DAVIX_INTERRUPT_H  */

