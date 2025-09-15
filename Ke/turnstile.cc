// SPDX-License-Identifier: GPL-3.0
/*
 * File: Ke/turnstile.cc
 *
 * Copyright (C) 2025  dbstream
 */
#include <Ke/log.h>
#include <Ke/sched.h>
#include <Ke/spinlock.h>
#include <Ke/thread.h>
#include <Ke/turnstile.h>
#include <Ki/turnstile.h>
#include <Mm/pool.h>
#include <dsl/hlist.h>
#include <dsl/list.h>

struct KTURNSTILE {
	union {
		KTURNSTILE *next;
		dsl::HListEntry hlist_entry;
	};

	unsigned long count;

	void *address;

	dsl::TypedList<KTHREAD, &KTHREAD::wq_entry> queues[KTURNSTILE_NUM_Q];
};

static KeSpinlock turnstile_pool_lock;
static KTURNSTILE *turnstile_pool;

static KTURNSTILE *allocate_turnstile(void)
{
	turnstile_pool_lock.lock();
	if (!turnstile_pool)
		KePanic("allocate_turnstile: out of turnstiles");
	KTURNSTILE *tstl = turnstile_pool;
	turnstile_pool = tstl->next;
	turnstile_pool_lock.unlock();
	return tstl;
}

static void free_turnstile(KTURNSTILE *tstl)
{
	turnstile_pool_lock.lock();
	tstl->next = turnstile_pool;
	turnstile_pool = tstl;
	turnstile_pool_lock.unlock();
}

bool KiExpandTurnstilePool(void)
{
	KTURNSTILE *tstl = MmNew<KTURNSTILE>();
	if (!tstl)
		return false;

	free_turnstile(tstl);
	return true;
}

void KiShrinkTurnstilePool(void)
{
	MmDelete(allocate_turnstile());
}

struct turnstile_hash_table {
	dsl::TypedHList<KTURNSTILE, &KTURNSTILE::hlist_entry> list;
	KeSpinlock lock;
};

static constexpr unsigned long hash_table_size = 256;
static constexpr unsigned long hash_table_mask = hash_table_size - 1;

static inline unsigned long hash(void *address)
{
	unsigned long ulong = (unsigned long) address;

	ulong ^= ulong >> 8;
	ulong ^= ulong >> 16;
	ulong ^= ulong >> 32;
	return ulong & hash_table_mask;
}

static struct turnstile_hash_table hash_table[hash_table_size];

/**
 * KeTurnstileGet - acquire a turnstile for a synchronization object.
 * @address: pointer to synchronization object
 */
KTURNSTILE *KeTurnstileGet(void *address)
{
	unsigned long idx = hash(address);

	turnstile_hash_table *entry = hash_table + idx;

	entry->lock.lock();
	for (KTURNSTILE *tstl : entry->list) {
		if (tstl->address == address) {
			tstl->count++;
			return tstl;
		}
	}

	KTURNSTILE *tstl = allocate_turnstile();
	tstl->address = address;
	tstl->count = 1;
	for (int i = 0; i < KTURNSTILE_NUM_Q; i++)
		tstl->queues[i].init();
	entry->list.push(tstl);
	return tstl;
}

/**
 * KeTurnstilePut - release a turnstile.
 * @tstl: pointer to turnstile
 */
void KeTurnstilePut(KTURNSTILE *tstl)
{
	unsigned long idx = hash(tstl->address);

	turnstile_hash_table *entry = hash_table + idx;

	if (!--(tstl->count)) {
		tstl->hlist_entry.remove();
		free_turnstile(tstl);
	}

	entry->lock.unlock();
}

/** 
 * KeTurnstileBlock - block on a turnstile.
 * @tstl: pointer to turnstile
 * @queue: KTURNSTILE_Q_READER or KTURNSTILE_Q_WRITER
 */
void KeTurnstileBlock(KTURNSTILE *tstl, int queue)
{
	unsigned long idx = hash(tstl->address);

	turnstile_hash_table *entry = hash_table + idx;

	KTHREAD *me = HalCurrentThread();

	me->wq_removed = false;
	tstl->queues[queue].push_back(me);

	entry->lock.unlock();
	KeReschedule();
	entry->lock.lock();

	if (!me->wq_removed)
		me->wq_entry.remove();
}

/**
 * KeTurnstileWake - wake up turnstile waiters.
 * @tstl: pointer to turnstile
 * @queue: KTURNSTILE_Q_READER or KTURNSTILE_Q_WRITER
 *
 * This function wakes up all turnstile waiters in the given queue.
 */
void KeTurnstileWake(KTURNSTILE *tstl, int queue)
{
	KTHREAD *thread = tstl->queues[queue].pop_front();
	while (thread) {
		thread->wq_removed = true;
		KeWakeThread(thread);
		thread = tstl->queues[queue].pop_front();
	}
}

