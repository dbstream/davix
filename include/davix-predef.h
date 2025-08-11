// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/davix-predef.h
 * Predefined macros.
 *
 * Copyright (C) 2025  dbstream
 *
 * davix-predef.h is automatically included by _everything_ that uses the C
 * preprocessor, as if by '#include <davix-predef.h>' at the start of every
 * translation unit.  It should be kept as minimal as possible.
 */
#ifndef __davix_predef_h_included
#define __davix_predef_h_included 1

#ifdef __DAVIX_KERNEL__

/*
 * CONFIG_MAX_NR_CPUS: the maximum number of logical CPUs that are supported in
 * a Shared Memory Processor (SMP) system.
 */
#ifndef CONFIG_MAX_NR_CPUS
#define CONFIG_MAX_NR_CPUS		256
#endif /* CONFIG_MAX_NR_CPUS */

#define UACPI_OVERRIDE_LIBC 1
#define UACPI_OVERRIDE_TYPES 1

#endif /* __DAVIX_KERNEL__ */

#endif /* __davix_predef_h_included */

