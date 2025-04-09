/**
 * Predefined macros.
 * Copyright (C) 2025-present  dbstream
 *
 * davix-predef.h is included by _everything_ that uses the C preprocessor.  It
 * should be kept entirely self-contained.
 */
#ifndef __davix_predef_h_included
#define __davix_predef_h_included 1

#define UACPI_BAREBONES_MODE 1
#define UACPI_OVERRIDE_LIBC 1
#define UACPI_OVERRIDE_TYPES 1

#ifndef CONFIG_MAX_NR_CPUS
#define CONFIG_MAX_NR_CPUS 1
#endif

#endif /** __davix_predef_h_included */
