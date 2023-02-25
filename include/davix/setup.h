/* SPDX-License-Identifier: MIT */
#ifndef __DAVIX_SETUP_H
#define __DAVIX_SETUP_H

extern const char *kernel_cmdline;

void start_kernel(void);

/*
 * arch_early_init() should do stuff like find all memory regions, initialize
 * page_alloc() and kmalloc() and install an early exception handler for kernel
 * exceptions.
 */
void arch_early_init(void);

/*
 * Setup IRQ handling.
 */
void arch_setup_interrupts(void);

#endif /* __DAVIX_SETUP_H */
