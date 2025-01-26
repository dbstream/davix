/**
 * Fixed kernel configuration.
 * Copyright (C) 2024  dbstream
 *
 * This file is included automatically by the build system, via gcc -include. It
 * can be viewed as a primitive Kconfig-like system. The goal is to parameterise
 * build-time constants, such as the maximum number of CPUs the kernel is able
 * to support.
 */

/* <config.h> is a way for the user provides their own configuration values. */
#if __has_include(<config.h>)
#include <config.h>
#endif

/* <builtin_config.h> is used by architectures to augment this file. */
#if __has_include(<asm/builtin_config.h>)
#include <asm/builtin_config.h>
#endif

/* The maximum number of CPUs that we support. Other CPUs will be ignored. */
#ifndef CONFIG_MAX_NR_CPUS
#define CONFIG_MAX_NR_CPUS	256
#endif

/* Whether to run ktests or not. */
#ifndef CONFIG_KTESTS
#define CONFIG_KTESTS		0
#endif

/* Run ktests for the VMA tree. */
#ifndef CONFIG_KTEST_VMATREE
#define CONFIG_KTEST_VMATREE	0
#endif

/* Run ktests for the mutex implementation.  */
#ifndef CONFIG_KTEST_MUTEX
#define CONFIG_KTEST_MUTEX	0
#endif

/* Run the uACPI kernel benchmark.  */
/* WARNING!!! NOT SAFE ON REAL HARDWARE!!!  */
#ifndef CONFIG_UACPI
#define CONFIG_UACPI 0
#endif

/** TODO: find a better place for uACPI config stuff.  */
/**
 * Ok, turns out we need UACPI_OVERRIDE_TYPES even when !CONFIG_UACPI.  This is
 * because the build system doesn't integrate with config.h.
 */
#define UACPI_OVERRIDE_TYPES 1
