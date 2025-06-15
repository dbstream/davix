/**
 * Architecture-specific details of the interrupt model.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

/**
 * The first 32 IDT entries are used by the CPU for exception handling.  As a
 * result, each CPU has IRQ_VECTOR_NUM (224) vectors, numbered IRQ_VECTOR_OFFSET
 * through IRQ_VECTOR_OFFSET+IRQ_VECTOR_NUM-1 (32 through 255).
 *
 * Note that this area overlaps with vectors that the architecture reserves for
 * purposes such as the APIC timer, handling interprocessor function calls, and
 * more.  The architecture reserves all such vectors early at boot.
 */

#define IRQ_VECTOR_OFFSET 32
#define IRQ_VECTOR_NUM 224

/**
 * The following vectors are reserved by the architecture:
 */
#define VECTOR_SMP_RESCHEDULE		251
#define VECTOR_SMP_PANIC		252
#define VECTOR_SMP_CALL_ON_ONE		253
#define VECTOR_APIC_TIMER		254
#define VECTOR_SPURIOUS			255
