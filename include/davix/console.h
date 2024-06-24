/**
 * Kernel console.
 * Copyright (C) 2024  dbstream
 */
#ifndef _DAVIX_CONSOLE_H
#define _DAVIX_CONSOLE_H 1

#include <davix/time.h>

extern void
arch_printk_emit (int level, usecs_t msg_time, const char *msg);

#endif /* _DAVIX_CONSOLE_H */
