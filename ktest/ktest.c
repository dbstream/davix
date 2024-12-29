/**
 * This is the main testrunner.
 */
#if CONFIG_KTESTS

#include <davix/printk.h>
#include "mutex.h"
#include "vmatree.h"

void
run_ktests (void)
{
	printk (PR_NOTICE "Running ktests...\n");
	ktest_mutex ();
	ktest_vmatree ();
}

#endif
