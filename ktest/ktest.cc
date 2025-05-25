/**
 * Kernel self-testing.
 * Copyright (C) 2025-present  dbstream
 */
#if CONFIG_KTEST_VMATREE
void ktest_vmatree (void);
#else
static inline void ktest_vmatree (void) {}
#endif


void
run_ktests (void)
{
	ktest_vmatree ();
}
