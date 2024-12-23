/**
 * Interrupt vectors.
 * Copyright (C) 2024  dbstream
 */
#ifndef ASM_INTERRUPT_H
#define ASM_INTERRUPT_H 1

#define IRQ_VECTORS_PER_CPU	64
#define IRQ_VECTORS_OFFSET	64	/* offset of IRQ vectors in the IDT */

#define VECTOR_PANIC		0xfd
#define VECTOR_APIC_TIMER	0xfe
#define VECTOR_SPURIOUS		0xff

#endif /* ASM_INTERRUPT_H */
