/* SPDX-License-Identifier: MIT */
#ifndef __DAVIX_SETUP_H
#define __DAVIX_SETUP_H

void start_kernel(const char *cmdline);

/*
 * arch_early_init() should do stuff like find all memory regions, initialize
 * page_alloc() and kmalloc() and install an early exception handler for kernel
 * exceptions.
 */
void arch_early_init(void);

#endif /* __DAVIX_SETUP_H */
