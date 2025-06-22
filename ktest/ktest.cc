/**
 * Kernel self-testing.
 * Copyright (C) 2025-present  dbstream
 */
#if CONFIG_KTEST_VMATREE
void ktest_vmatree (void);
#else
static inline void ktest_vmatree (void) {}
#endif

#if CONFIG_KTEST_FIREWORKS
void ktest_fireworks (void);
#else
static inline void ktest_fireworks (void) {}
#endif

#if CONFIG_KTEST_MUTEX
void ktest_mutex (void);
#else
static inline void ktest_mutex (void) {}
#endif

void
run_ktests (void)
{
	ktest_fireworks ();
	ktest_mutex ();
	ktest_vmatree ();
}
