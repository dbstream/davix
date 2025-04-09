/**
 * Deferred Procedure Calls (DPC).
 * Copyright (C) 2025-present  dbstream
 */
#include <asm/percpu.h>
#include <davix/dpc.h>
#include <davix/irql.h>
#include <dsl/list.h>

typedef dsl::TypedList<DPC, &DPC::m_listHead> DPCList;

static DEFINE_PERCPU(DPCList, globalDpcList);

PERCPU_CONSTRUCTOR(kernel_dpc)
{
	DPCList *list = percpu_ptr (globalDpcList).on (cpu);
	list->init ();
}

/**
 * DPC::enqueue - schedule a DPC for execution.
 * Returns true if the DPC was already enqueued.
 */
bool
DPC::enqueue (void)
{
	scoped_irql guard (IRQL_HIGH);

	if (m_is_enqueued)
		return true;

	DPCList *list = percpu_ptr (globalDpcList);
	list->push_back (this);
	irql_set_pending_dpc ();
	return false;
}

/**
 * dispatch_DPCs - dispatch scheduled DPCs.
 * NOTE: this function must be called at DPC level.
 */
void
dispatch_DPCs (void)
{
	DPCList *list = percpu_ptr (globalDpcList);
	irql_cookie_t cookie = raise_irql (IRQL_HIGH);
	while (!list->empty ()) {
		DPC *dpc = list->pop_front ();
		dpc->m_is_enqueued = false;

		void (*routine) (DPC *, void *, void *) = dpc->m_routine;
		void *arg1 = dpc->m_arg1;
		void *arg2 = dpc->m_arg2;

		lower_irql (cookie);
		routine (dpc, arg1, arg2);
		cookie = raise_irql (IRQL_HIGH);
	}
	lower_irql (cookie);
}
