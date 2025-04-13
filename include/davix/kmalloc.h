/**
 * kmalloc()
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#include <stddef.h>
#include <davix/allocation_class.h>

void
kmalloc_init (void);

void *
kmalloc (size_t size, allocation_class aclass);

void
kfree (void *ptr);
