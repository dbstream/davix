/**
 * start_kernel and related infrastructure
 * Copyright (C) 2025-present  dbstream
 */
#include <asm/irql.h>
#include <davix/dpc.h>
#include <davix/page.h>
#include <davix/printk.h>
#include <davix/start_kernel.h>

static DPC hello_dpc;

static void
hello_dpc_routine (DPC *dpc, void *arg1, void *arg2)
{
	(void) dpc;
	(void) arg1;
	(void) arg2;
	printk (PR_INFO "Hello, DPCs!\n");
}

void
start_kernel (void)
{
	printk (PR_NOTICE "Davix version 0.0.1\n");

	hello_dpc.init (hello_dpc_routine, nullptr, nullptr);
	hello_dpc.enqueue ();

	dump_pgalloc_stats ();
}
