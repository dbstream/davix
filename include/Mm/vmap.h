// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/Mm/vmap.h
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

#include <Hal/pteflags.h>

void *MmMapVirtual(unsigned long phys, unsigned long size, PTEFLAGS flags);

void MmUnmapVirtual(void *mem);

void *MmAllocateVirtual(unsigned long size, PTEFLAGS flags);

void MmFreeVirtual(void *mem);

