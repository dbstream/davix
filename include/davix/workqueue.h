/**
 * Kernel workqueues
 * Copyright (C) 2024  dbstream
 *
 * Workqueues are jobs represented by 'struct work'. They are executed FIFO, in
 * a high-priority kernel thread on each CPU. If a workload can sleep, it is not
 * suitable for workqueues.
 */
#ifndef _DAVIX_WORKQUEUE_H
#define _DAVIX_WORKQUEUE_H 1

#include <davix/list.h>

struct work {
	struct list list;
	void (*execute) (struct work *);
};

extern void
workqueue_init_this_cpu (void);

extern void
wq_enqueue_work (struct work *work);

#endif /* _DAVIX_WORKQUEUE_H */
