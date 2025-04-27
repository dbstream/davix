/**
 * Early memory allocator.
 * Copyright (C) 2025-present  dbstream
 *
 * The early memory allocator is a simple first-fit freelist allocator, suitable
 * for early memory management before the normal page allocation mechanism has
 * been brought online.
 *
 * The early memory allocator uses inline storage for the list linkage, thus the
 * memory it manages must be accessible through the kernel virtual map.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

void
early_free_everything_to_pgalloc (void);

uintptr_t
early_alloc_phys_range (size_t size, size_t align, uintptr_t low, uintptr_t high);

void
early_free_phys (uintptr_t addr, size_t size);

void *
early_alloc_virt_zone (size_t size, size_t align, int zonenr);

void *
early_alloc_virt (size_t size, size_t align);

void
early_free_virt (void *ptr, size_t size);
