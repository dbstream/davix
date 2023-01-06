/* SPDX-License-Identifier: MIT */
#include <davix/mm.h>
#include <davix/page_table.h>

/*
 * A ``struct mm`` used for purely kernel-mode tasks and to manage page tables
 * for kernel mappings.
 */
struct mm kernelmode_mm = {
	.mm_lock = SPINLOCK_INIT(kernelmode_mm.mm_lock),
	.page_tables = NULL
};
