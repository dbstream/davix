// SPDX-License-Identifier: GPL-3.0
/*
 * File: Ke/irq/vector.cc
 * Vector allocation and management.
 *
 * Copyright (C) 2025  dbstream
 */
#include <Hal/irq_vectors.h>
#include <Ke/irq.h>
#include <Ke/log.h>
#include <Ke/spinlock.h>

static constexpr unsigned int NUM_DYNAMIC_VECTORS =
	IRQ_VECTOR_DYNAMIC_LAST - IRQ_VECTOR_DYNAMIC_FIRST + 1;

static bool global_inuse_map[NUM_DYNAMIC_VECTORS];

static KeDPCSpinlock global_vector_lock;

KIRQ *kiVectorContext[NUM_DYNAMIC_VECTORS];

/*
 * KeReserveInterruptVector - reserve an interrupt vector globally.
 *
 * Note: This is intended for use by the architecture bringup code early during
 * boot. It is a bug to call this function during normal system operation.
 */
void KeReserveInterruptVector(unsigned int vector)
{
	if (vector < IRQ_VECTOR_DYNAMIC_FIRST)
		return;
	if (vector > IRQ_VECTOR_DYNAMIC_LAST)
		return;

	global_vector_lock.lock();
	if (global_inuse_map[vector - IRQ_VECTOR_DYNAMIC_FIRST])
		KePanic("KeReserveInterruptVector: vector %u is already in use!", vector);
	global_inuse_map[vector - IRQ_VECTOR_DYNAMIC_FIRST] = true;
	global_vector_lock.unlock();
}

OSSTATUS KeAllocateInterruptVector(KIRQ *irq, unsigned int *vector)
{
	global_vector_lock.lock();

	for (unsigned int i = 0; i < NUM_DYNAMIC_VECTORS; i++) {
		if (global_inuse_map[i])
			continue;

		global_inuse_map[i] = true;
		kiVectorContext[i] = irq;
		global_vector_lock.unlock();
		*vector = i + IRQ_VECTOR_DYNAMIC_FIRST;
		return OS_STATUS_SUCCESS;
	}

	global_vector_lock.unlock();
	return OS_STATUS_RESOURCE_EXHAUSTED;
}

void KeFreeInterruptVector(KIRQ *irq, unsigned int vector)
{
	global_vector_lock.lock();

	if (!global_inuse_map[vector - IRQ_VECTOR_DYNAMIC_FIRST])
		KePanic("KeFreeInterruptVector: vector %u is not in use!", vector);
	if (kiVectorContext[vector - IRQ_VECTOR_DYNAMIC_FIRST] != irq)
		KePanic("KeFreeInterruptVector: KIRQ structure mismatch!");

	kiVectorContext[vector - IRQ_VECTOR_DYNAMIC_FIRST] = nullptr;
	global_inuse_map[vector - IRQ_VECTOR_DYNAMIC_FIRST] = false;
	global_vector_lock.unlock();
}

