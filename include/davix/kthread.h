/**
 * Kernel-mode threads.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

struct Task;

Task *
kthread_create (const char *name, void (*function)(void *), void *arg);

void
kthread_start (Task *task);

void
kthread_exit (void);

