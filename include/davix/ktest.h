/**
 * ktest - Kernel unit testing system.
 * Copyright (C) 2024  dbstream
 */
#ifndef __DAVIX_KTEST_H
#define __DAVIX_KTEST_H 1

#if CONFIG_KTESTS

extern void
run_ktests (void);

#else

static inline void run_ktests (void) {}

#endif

#endif /* __DAVIX_KTEST_H */
