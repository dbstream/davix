/**
 * Kernel printk.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#define PR_INFO "\x01\x31\x02"
#define PR_NOTICE "\x01\x32\x02"
#define PR_WARN "\x01\x33\x02"
#define PR_ERROR "\x01\x34\x02"

#ifdef __cplusplus
extern "C"
#endif
void
__attribute__ ((format (printf, 1, 2)))
printk (const char *fmt, ...);
