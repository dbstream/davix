/**
 * Unit testing for the VMA tree.
 * Copyright (C) 2024  dbstream
 */

#if CONFIG_KTEST_VMATREE

extern void
ktest_vmatree (void);

#else

static inline void ktest_vmatree (void) {}

#endif
