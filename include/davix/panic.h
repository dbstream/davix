/**
 * panic() - the kernel's "bug check".
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#ifdef __cplusplus
extern "C"
#endif
void
__attribute__ ((format (printf, 1, 2)))
panic (const char *fmt, ...);
