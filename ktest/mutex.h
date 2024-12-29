/**
 * Ktest suite for mutexes.
 * Copyright (C) 2024  dbstream
 */

#if CONFIG_KTEST_MUTEX

extern void
ktest_mutex (void);

#else

static inline void ktest_mutex (void) {}

#endif
