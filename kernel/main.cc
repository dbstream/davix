/**
 * start_kernel and related infrastructure
 * Copyright (C) 2025-present  dbstream
 */
#include <asm/irql.h>
#include <davix/dpc.h>
#include <davix/early_alloc.h>
#include <davix/kmalloc.h>
#include <davix/page.h>
#include <davix/printk.h>
#include <davix/slab.h>
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

extern const char davix_banner[];

void
start_kernel (void)
{
	printk (PR_NOTICE "%s\n", davix_banner);

	arch_init ();

	hello_dpc.init (hello_dpc_routine, nullptr, nullptr);
	hello_dpc.enqueue ();

	pgalloc_init ();
	early_free_everything_to_pgalloc ();
	dump_pgalloc_stats ();

	kmalloc_init ();
	slab_dump ();
}
