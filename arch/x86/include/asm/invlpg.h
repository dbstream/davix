// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/Hal/invlpg.h
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

static inline void __invlpg(unsigned long addr)
{
	asm volatile("invlpg (%0)" :: "r"(addr) : "memory");
}

