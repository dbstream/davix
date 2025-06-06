/**
 * Low-level timer initialization functions.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

void
x86_init_time (void);

void
x86_synchronize_tsc_control (void);

void
x86_synchronize_tsc_victim (void);

void
tsc_sync_dump (void);
