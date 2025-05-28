/**
 * Kernel virtual address space management.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#include <asm/pgtable.h>
#include <stddef.h>
#include <stdint.h>

void *
vmap_io_range (uintptr_t phys, size_t size, page_cache_mode pcm,
		uintptr_t low, uintptr_t high);

void *
vmap_io (uintptr_t phys, size_t size, page_cache_mode pcm);

void *
vmap_range (uintptr_t phys, size_t size, uintptr_t low, uintptr_t high);

void *
vmap (uintptr_t phys, size_t size);

void
vunmap (void *ptr);

void *
kmalloc_large (size_t size);

void
kfree_large (void *mem);
