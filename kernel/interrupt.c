/**
 * Interrupt handling - Generic code.
 * Copyright (C) 2025-present  dbstream
 */
#include <davix/interrupt.h>
#include <davix/printk.h>
#include <davix/spinlock.h>
#include <asm/interrupt.h>

static struct vector {
	spinlock_t vector_lock;
	struct list handler_list;
	bool exclusive;
} vectors[IRQ_VECTORS_PER_CPU];

static bool has_inited_vectors;

static void
init_vectors (void)
{
	bool flag = irq_save ();
	for (unsigned int i = 0; i < IRQ_VECTORS_PER_CPU; i++) {
		__spin_lock (&vectors[i].vector_lock);
		list_init (&vectors[i].handler_list);
		__spin_unlock (&vectors[i].vector_lock);
	}
	atomic_store_release (&has_inited_vectors, true);
	irq_restore (flag);
}

void
handle_interrupt (unsigned int vector)
{
	if (vector >= IRQ_VECTORS_PER_CPU) {
		printk (PR_WARN "IRQ: handle_interrupt was called for vector %u, which is >= IRQ_VECTORS_PER_CPU\n",
				vector);
		return;
	}

	// ok... technically this can be UB.  but in practice, never.
	if (!atomic_load_relaxed (&has_inited_vectors))
		return;

	irqstatus_t status = INTERRUPT_NOT_HANDLED;
	unsigned int count = 0;
	__spin_lock (&vectors[vector].vector_lock);
	list_for_each (entry, &vectors[vector].handler_list) {
		count++;
		struct interrupt_handler *h = struct_parent (
				struct interrupt_handler, irq_handler_list, entry);

		irqstatus_t dev_status = h->handler (h);
		if (dev_status == INTERRUPT_HANDLED)
			status = INTERRUPT_HANDLED;
	}

	__spin_unlock (&vectors[vector].vector_lock);
	if (status == INTERRUPT_NOT_HANDLED)
		printk (PR_WARN "IRQ: unrecognized interrupt on vector %u  (tried %u handlers)\n",
			vector, count);
}

void
sync_interrupts (unsigned int vector)
{
	irq_disable ();
	while (!spin_trylock (&vectors[vector].vector_lock)) {
		irq_enable ();
		arch_relax ();
		arch_relax ();
		irq_disable ();
	}
	__spin_unlock (&vectors[vector].vector_lock);
	irq_enable ();
}

static bool inuse_vector_map[IRQ_VECTORS_PER_CPU];
static spinlock_t vector_alloc_lock;

unsigned int
alloc_irq_vector (void)
{
	unsigned int vector = -1U;
	spin_lock (&vector_alloc_lock);
	for (unsigned int i = 0; i < IRQ_VECTORS_PER_CPU; i++) {
		if (inuse_vector_map[i])
			continue;

		inuse_vector_map[i] = true;
		vector = i;
		break;
	}
	spin_unlock (&vector_alloc_lock);
	return vector;
}

void
free_irq_vector (unsigned int vector)
{
	spin_lock (&vector_alloc_lock);
	inuse_vector_map[vector] = false;
	spin_unlock (&vector_alloc_lock);
}

// "intercontinental ballistic irq installer"  --   :P
static errno_t
icbirq (unsigned int vector, void *p)
{
	errno_t e = ESUCCESS;
	struct interrupt_handler *h = p;
	h->vector_number = vector;

	bool flag = spin_lock_irq (&vectors[vector].vector_lock);
	if (vectors[vector].exclusive)
		e = EEXIST;
	else {
		if (h->flags & INTERRUPT_EXCLUSIVE)
			vectors[vector].exclusive = true;
		list_insert_back (&vectors[vector].handler_list, &h->irq_handler_list);
	}
	spin_unlock_irq (&vectors[vector].vector_lock, flag);
	return e;
}

static spinlock_t install_irq_lock;

errno_t
install_interrupt_handler (struct interrupt_handler *handler,
		unsigned int irq_number, irqhandler_flags_t flags)
{
	handler->flags = flags;
	handler->irq_number = irq_number;
	spin_lock (&install_irq_lock);
	if (!has_inited_vectors)
		init_vectors ();

	errno_t e = arch_configure_interrupt_pin (&irq_number, flags,
			icbirq, handler);
	spin_unlock (&install_irq_lock);
	handler->irq_number = irq_number;
	return e;
}

void
uninstall_interrupt_handler (struct interrupt_handler *handler)
{
	spin_lock (&install_irq_lock);
	bool flag = spin_lock_irq (&vectors[handler->vector_number].vector_lock);
	list_delete (&handler->irq_handler_list);
	bool flag2 = list_empty (&vectors[handler->vector_number].handler_list);
	spin_unlock_irq (&vectors[handler->vector_number].vector_lock, flag);
	if (flag2)
		arch_clear_and_free_interrupt_pin (handler->irq_number,
				handler->vector_number);
	spin_unlock (&install_irq_lock);
}


