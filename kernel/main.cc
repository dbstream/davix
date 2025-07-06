/**
 * start_kernel and related infrastructure
 * Copyright (C) 2025-present  dbstream
 */
#include <asm/irql.h>
#include <davix/cmdline.h>
#include <davix/cpuset.h>
#include <davix/early_alloc.h>
#include <davix/fs.h>
#include <davix/kmalloc.h>
#include <davix/ktest.h>
#include <davix/kthread.h>
#include <davix/page.h>
#include <davix/panic.h>
#include <davix/printk.h>
#include <davix/rcu.h>
#include <davix/sched.h>
#include <davix/smp.h>
#include <davix/start_kernel.h>

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

static RCUHead hello_rcu_head;

static void
hello_rcu (RCUHead *rh)
{
	(void) rh;
	printk (PR_INFO "Hello, RCU!\n");
}

static void
setup_and_exec_init (void *arg);

void
start_kernel (void)
{
	rcu_enable ();

	printk (PR_NOTICE "%s\n", davix_banner);
	printk (PR_INFO "Kernel command line: %s\n", kernel_cmdline);

	arch_init ();
	printk (PR_INFO "CPUs: %d\n", nr_cpus);

	pgalloc_init ();
	early_free_everything_to_pgalloc ();
	dump_pgalloc_stats ();

	kmalloc_init ();
//	slab_dump ();

	sched_init ();
	smp_boot_all_cpus ();

	rcu_call (&hello_rcu_head, hello_rcu);

	Task *init_task = kthread_create ("init", setup_and_exec_init, nullptr);
	if (!init_task)
		panic ("Failed to create init_task!");

	kthread_start (init_task);
	sched_idle ();
}

/**
 * setup_and_exec_init - Mount the root filesystem and pass control to init.
 */
static void
setup_and_exec_init (void *arg)
{
	(void) arg;

	printk (PR_INFO "Hello from init!\n");

	init_fs_caches ();

	run_ktests ();

	printk (PR_WARN "TODO: execve(/sbin/init)\n");
	kthread_exit();
}

