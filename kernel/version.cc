/**
 * Kernel version banner.
 * Copyright (C) 2025-present  dbstream
 */

#define STR(x) STR_(x)
#define STR_(x) #x

extern const char davix_banner[] = "Davix version 0.0.1 ("
		STR(COMPILE_USER) "@" STR(COMPILE_HOST) ") ("
		STR(CC_VERSION) ")";
