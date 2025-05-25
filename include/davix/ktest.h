/**
 * Kernel self-testing.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#if CONFIG_KTEST
void run_ktests (void);
#else
static inline void run_ktests (void) {}
#endif
