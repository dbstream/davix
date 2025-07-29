// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/Hal/pteflags.h
 * PTEFLAGS and PAGE_CACHE_MODE enumeration.
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

#include <asm/cpufeature.h>
#include <asm/pg_bits.h>
#include <davix/bug.h>

typedef unsigned long PTEFLAGS;

typedef enum {
	PAGE_WRITEBACK		= 0,
	PAGE_WRITETHROUGH	= 1,
	PAGE_WRITECOMBINE	= 2,
	PAGE_UNCACHED		= 3,
} PAGE_CACHE_MODE;

/**
 * HalMakePTEFlags - make a PTEFLAGS value from cache mode and RWX permissions.
 * @cache_mode: cache mode
 * @read: read permission
 * @write: write permission
 * @exec: exec permission
 */
static inline PTEFLAGS HalMakePTEFlags(
		PAGE_CACHE_MODE cache_mode,
		bool read, bool write, bool exec)
{
	BUG_ON(!read && !write && !exec);

	PTEFLAGS flags = __PG_PRESENT;
	switch(cache_mode) {
	case PAGE_WRITETHROUGH:
		flags |= __PG_PWT;
		break;
	case PAGE_WRITECOMBINE:
		if (CPUFeature(PAT))
			flags |= __PG_PAT;
		else
			flags |= __PG_PCD;
		break;
	case PAGE_UNCACHED:
		flags |= __PG_PCD;
		break;
	default:
		break;
	}

	if (write)
		flags |= __PG_WRITE;

	if (!exec && CPUFeature(NX))
		flags |= __PG_NX;

	return flags;
}

extern PTEFLAGS PTEFLAGS_READONLY;
extern PTEFLAGS PTEFLAGS_READWRITE;
extern PTEFLAGS PTEFLAGS_READEXEC;

