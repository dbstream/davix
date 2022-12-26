/* SPDX-License-Identifier: MIT */
#ifndef __DAVIX_MM_H
#define __DAVIX_MM_H

#include <davix/atomic.h>
#include <davix/mm_types.h>

#define _set_page_bit(page, bit, memorder) \
	atomic_fetch_or(&(page)->flags, (bit), (memorder))

#define _clear_page_bit(page, bit, memorder) \
	atomic_fetch_and(&(page)->flags, ~(bit), (memorder))

#define set_page_bit(page, bit) \
	_set_page_bit((page), (bit), memory_order_relaxed)

#define clear_page_bit(page, bit) \
	_clear_page_bit((page), (bit), memory_order_relaxed)

#define _test_page_bit(page, bit, memorder) \
	(atomic_load(&(page)->flags, (memorder)) & (bit))

#define test_page_bit(page, bit) \
	_test_page_bit((page), (bit), memory_order_relaxed)

#endif /* __DAVIX_MM_H */
