/* SPDX-License-Identifier: MIT */
#include <davix/page_table.h>
#include <davix/printk.h>
#include <asm/cpuid.h>
#include <asm/msr.h>
#include <asm/page.h>

static int use_pat;
static unsigned long msrpat_value;

static pte_t pgcachemode_to_pteflags_table[NUM_PG_CACHEMODES] = {
	[PG_WRITEBACK] =	0		| 0		| 0,
	[PG_WRITETHROUGH] =	0		| 0		| __PG_PWT,
	[PG_WRITECOMBINE] =	__P1E_PAT	| 0		| 0,
	[PG_UNCACHED_MINUS] =	0		| __PG_PCD	| 0,
	[PG_UNCACHED] =		0		| __PG_PCD	| __PG_PWT
};

static inline pte_t pgcachemode_to_pteflags(pgcachemode_t cachemode, unsigned lvl)
{
	pte_t pteflags = pgcachemode_to_pteflags_table[cachemode];
	if((pteflags & __P1E_PAT) && lvl > 1) {
		pteflags &= ~__P1E_PAT;
		pteflags |= __P2E_PAT;
	}
	return pteflags;
}

static_assert(__P2E_PAT == __P3E_PAT, "page flags");

void x86_setup_pat(void);
void x86_setup_pat(void)
{
	unsigned long a, b, c, d;
	do_cpuid(1, a, b, c, d);

	if(!(d & __ID_00000001_EDX_PAT)) {
		warn("x86/mm: no PAT mechanism, all write-combine mappings will be uncached.\n");
		use_pat = 0;
		pgcachemode_to_pteflags_table[PG_WRITECOMBINE] =
			pgcachemode_to_pteflags_table[PG_UNCACHED];
		return;
	}

	use_pat = 1;
	msrpat_value = 0x100070406; /* UC  UC  UC  WC  UC  UC-  WT  WB */

	info("x86/mm: enable PAT mechanism.\n");
	write_msr(MSR_PAT, msrpat_value);
}
