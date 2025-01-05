/**
 * x86-specific memory manager functionality.
 * Copyright (C) 2025-present  dbstream
 */
#ifndef _ASM_MM_H
#define _ASM_MM_H 1

struct process_mm;

extern void
switch_to_mm (struct process_mm *mm);

#endif /** _ASM_MM_H  */
