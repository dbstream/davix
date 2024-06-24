/**
 * Architecture support for pfn_entry
 * Copyright (C) 2024  dbstream
 */
#ifndef ASM_PAGE_H
#define ASM_PAGE_H 1

#include <asm/page_defs.h>

#define ARCH_PFN_ENTRY_SIZE	64

struct pfn_entry;

extern unsigned long PFN_OFFSET;

static inline struct pfn_entry *
pfn_to_pfn_entry (unsigned long pfn)
{
	return (struct pfn_entry *) (PFN_OFFSET + ARCH_PFN_ENTRY_SIZE * pfn);
}

static inline unsigned long
pfn_entry_to_pfn (struct pfn_entry *entry)
{
	return ((unsigned long) entry - PFN_OFFSET) / ARCH_PFN_ENTRY_SIZE;
}

extern void
arch_init_pfn_entry (void);

#endif /* ASM_PAGE_H */
