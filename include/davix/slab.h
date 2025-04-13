/**
 * Slab allocator.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#include <stddef.h>
#include <davix/allocation_class.h>

struct SlabAllocator;

SlabAllocator *
slab_create (const char *name, size_t size, size_t align);

void *
slab_alloc (SlabAllocator *allocator, allocation_class aclass);

void
slab_free (void *ptr);

void
kmalloc_init (void);

bool
ptr_is_slab (const void *ptr);

void
slab_dump (void);
