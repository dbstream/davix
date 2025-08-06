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

#define UACPI_OVERRIDE_LIBC 1
#define UACPI_OVERRIDE_TYPES 1

#endif

#endif /* __davix_predef_h_included */

