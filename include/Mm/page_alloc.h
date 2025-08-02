// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/Mm/page_alloc.h
 * Physical page allocation interfaces.
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

#include <Mm/pfn.h>

void MmFreePage(MMPFN *pfn);

MMPFN *MmAllocatePage(void);

