/**
 * Deferred Procedure Call (DPC)
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#include <dsl/list.h>

typedef struct DPC_T DPC;

struct DPC_T {
	dsl::ListHead m_listHead;

	void (*m_routine) (DPC *dpc, void *arg1, void *arg2);

	void *m_arg1, *m_arg2;
	bool m_is_enqueued;

	constexpr inline void
	init (void (*routine) (DPC *, void *, void *), void *arg1, void *arg2)
	{
		m_routine = routine;
		m_arg1 = arg1;
		m_arg2 = arg2;
		m_is_enqueued = false;
	}

	/**
	 * Schedule a DPC for execution on the currently-running processor.
	 *
	 * This function returns true if the DPC was already scheduled for
	 * execution.
	 */
	bool
	enqueue (void);
};

/**
 * dispatch_DPCs - dispatch scheduled DPCs.
 * NOTE: this function must be called at DPC level.
 */
void
dispatch_DPCs (void);
