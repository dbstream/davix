// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/Hal/irq_vectors.h
 * IRQ vector numbers and reserved vectors.
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

/*
 * Vectors 0x00 ... 0x1f (0 ... 31) are architecturally reserved and have
 * special uses such as CPU exceptions.
 *
 * Vectors 0x20 ... 0x2f (32 ... 47) are reserved for the 8259 PIC interrupts.
 *
 * Vector 0x80 is reserved as a compat syscall vector.
 *
 * Vectors IRQ_VECTOR_SYSTEM_FIRST ... 0xff (... 255) are reserved for system
 * use.
 *
 * The rest of the vectors are managed dynamically.
 *
 * System vectors:
 *	0xfa	250	Reschedule IPI
 *	0xfb	251	Scheduler timer dirty IPI
 *	0xfc	252	KePanic request vector
 *	0xfd	253	TLB invalidation
 *	0xfe	254	Local APIC Timer
 *	0xff	255	Spurious APIC Interrupt
 */

#define IRQ_VECTOR_8259_OFFSET		0x20

#define IRQ_VECTOR_INT80h		0x80

#define IRQ_VECTOR_SYSTEM_FIRST		0xfa
#define IRQ_VECTOR_RESCHEDULE		0xfa
#define IRQ_VECTOR_SCHED_TIMER_DIRTY	0xfb
#define IRQ_VECTOR_KERNEL_PANIC		0xfc
#define IRQ_VECTOR_TLBFLUSH		0xfd
#define IRQ_VECTOR_APIC_TIMER		0xfe
#define IRQ_VECTOR_APIC_SPURIOUS	0xff

#define IRQ_VECTOR_DYNAMIC_FIRST	0x30
#define IRQ_VECTOR_DYNAMIC_LAST		(IRQ_VECTOR_SYSTEM_FIRST - 1)

