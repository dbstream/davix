/**
 * start_kernel and related infrastructure
 * Copyright (C) 2025-present  dbstream
 */
#include <asm/irql.h>
#include <asm/smp.h>
#include <davix/cmdline.h>
#include <davix/cpuset.h>
#include <davix/dpc.h>
#include <davix/early_alloc.h>
#include <davix/kmalloc.h>
#include <davix/ktest.h>
#include <davix/page.h>
#include <davix/printk.h>
#include <davix/sched.h>
#include <davix/slab.h>
#include <davix/smp.h>
#include <davix/start_kernel.h>
#include <string.h>

#include <davix/kthread.h>
#include <davix/ktimer.h>

#include <vsnprintf.h>

static DPC hello_dpc;

static void
hello_dpc_routine (DPC *dpc, void *arg1, void *arg2)
{
	(void) dpc;
	(void) arg1;
	(void) arg2;
	printk (PR_INFO "Hello, DPCs!\n");
}

const char *kernel_cmdline = "";

/**
 * set_command_line - set the kernel command line.
 * @cmdline: pointer to kernel command line
 *
 * This function may be called early in boot.  It must be called prior to
 * start_kernel or not at all.  It is assumed that the memory pointed to by
 * cmdline will remain accessible.
 */
void
set_command_line (const char *cmdline)
{
	kernel_cmdline = cmdline;
}

static const char *
match_param (const char *param, const char *begin, const char *end)
{
	if (begin == end)
		return nullptr;

	bool squote = false;
	bool dquote = false;
	bool backslash = false;
	for (; begin != end; begin++) {
		if (backslash)
			backslash = false;
		else if (*begin == '\\' && !squote) {
			backslash = true;
			continue;
		} else if (*begin == '\'' && !dquote) {
			squote = !squote;
			continue;
		} else if (*begin == '"' && !squote) {
			dquote = !dquote;
			continue;
		} else if (*begin == '=' && !squote && !dquote)
			break;
		if (*param == *begin)
			param++;
		else
			return nullptr;
	}
	return *param ? nullptr : begin;
}

static inline bool
isspace (char c)
{
	return c == ' ' || c == '\t' || c == '\n' || c == '\v' || c == '\r';
}

/**
 * get_early_param - find a param in the kernel command line.
 * @param: parameter name
 * @idx: how many matching params to skip (for params that occur multiple times)
 * Returns a pointer to the character just after the param.
 *
 * Example: kernel command line is "foo=bar baz"
 *	get_early_param("foo", 0) returns a pointer to "=bar baz"
 *	get_early_param("baz", 0) returns a pointer to "baz"
 *	get_early_param("bar", 0) returns NULL
 *	get_early_param("foo", 1) returns NULL
 */
const char *
get_early_param (const char *param, int idx)
{
	if (!param || !*param)
		return nullptr;
	if (idx < 0)
		return nullptr;

	const char *begin = kernel_cmdline;
	const char *end = begin;
	const char *matching;
	bool squote = false;
	bool dquote = false;

	for (;; end++) {
		if (isspace (*end) || !*end) {
			if (*end && (squote || dquote))
				continue;
do_match:
			matching = match_param (param, begin, end);
			if (matching && !idx--)
				return matching;
			if (!*end)
				return nullptr;
			begin = ++end;
		} else if (*end == '\'' && !dquote) {
			squote = !squote;
		} else if (*end == '"' && !squote) {
			dquote = !dquote;
		} else if (*end == '\\' && !squote) {
			end++;
			if (!*end)
				goto do_match;
		}
	}
}

/**
 * early_param_matches - test if a command-line parameter has a given value
 * @expected: value to test for
 * @value: pointer to string returned by get_early_param
 * Returns true if the parameter matches the given value.
 */
bool
early_param_matches (const char *expected, const char *value)
{
	bool squote = false;
	bool dquote = false;
	bool backslash = false;

	if (!expected)
		expected = "";
	if (*value != '=')
		return !*expected;

	value++;
	for (; *value; value++) {
		if (backslash)
			backslash = false;
		else if (*value == '\\' && !squote) {
			backslash = true;
			continue;
		} else if (*value == '\'' && !dquote) {
			squote = !squote;
			continue;
		} else if (*value == '"' && !squote) {
			dquote = !dquote;
			continue;
		}
		if (*expected == *value)
			expected++;
		else
			return false;
	}
	return !*expected;
}

extern const char davix_banner[];

static KTimer hello_ktimer;

static void
hello_ktimer_routine (KTimer *ktimer, void *arg)
{
	(void) ktimer;
	(void) arg;

	hello_ktimer.enqueue (ns_since_boot () + 1000000000UL);
	printk (PR_INFO "Hello, KTimers!\n");
}

static void
hello_kthread (void *arg)
{
	int i = (int) (uintptr_t) arg;

	for (int j = 0; j < 2; j++) {
		printk (PR_INFO "CPU%u: hello kthreads!  [%2d] [%d]\n",
				this_cpu_id (), i, j);
		schedule ();
	}

	disable_dpc ();
	set_current_state (TASK_ZOMBIE);
	schedule ();
}

void
start_kernel (void)
{
	printk (PR_NOTICE "%s\n", davix_banner);
	printk (PR_INFO "Kernel command line: %s\n", kernel_cmdline);

	arch_init ();
	printk (PR_INFO "CPUs: %d\n", nr_cpus);

	hello_dpc.init (hello_dpc_routine, nullptr, nullptr);
	hello_dpc.enqueue ();

	hello_ktimer.init (hello_ktimer_routine, nullptr);
	hello_ktimer.enqueue (ns_since_boot () + 10000000000ULL);

	pgalloc_init ();
	early_free_everything_to_pgalloc ();
	dump_pgalloc_stats ();

	kmalloc_init ();
//	slab_dump ();

	sched_init ();
	smp_boot_all_cpus ();

	disable_dpc ();
	for (int i = 0; i < 5; i++) {
		char buf[10];
		snprintf (buf, sizeof (buf), "foo-%d", i);
		Task *task = kthread_create (buf, hello_kthread, (void *) (uintptr_t) i);
		if (task)
			kthread_start (task);
	}
	enable_dpc ();

	run_ktests ();
	sched_idle ();
}
