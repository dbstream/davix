/**
 * Layout of the __pcpu_fixed.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

struct x86_pcpu_fixed {
	void *pcpu_offset;
	int cpu_id;
	uint8_t irql_level[3];
	char reserved1[25];
	void *canary;
	char reserved2[16];
};

static_assert (sizeof (x86_pcpu_fixed) == 64);
static_assert (offsetof (x86_pcpu_fixed, pcpu_offset) == 0);
static_assert (offsetof (x86_pcpu_fixed, cpu_id) == 8);
static_assert (offsetof (x86_pcpu_fixed, irql_level) == 12);
static_assert (offsetof (x86_pcpu_fixed, canary) == 40);
