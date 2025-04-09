/**
 * Bits in PTEs (page table entries).
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#include <stdint.h>

#define __PG_BIT(n)	(UINT64_C(1) << n)
#define __PG_PRESENT	__PG_BIT(0)
#define __PG_WRITE	__PG_BIT(1)
#define __PG_USER	__PG_BIT(2)
#define __PG_PWT	__PG_BIT(3)
#define __PG_PCD	__PG_BIT(4)
#define __PG_ACCESSED	__PG_BIT(5)
#define __PG_DIRTY	__PG_BIT(6)
#define __PG_PAT	__PG_BIT(7)
#define __PG_HUGE	__PG_BIT(7)
#define __PG_GLOBAL	__PG_BIT(8)
#define __PG_AVL1	__PG_BIT(9)
#define __PG_AVL2	__PG_BIT(10)
#define __PG_AVL3	__PG_BIT(11)
#define __PG_PAT_HUGE	__PG_BIT(12)
#define __PG_NX		__PG_BIT(63)

#define __PG_ADDR_MASK UINT64_C(0x000ffffffffff000)
