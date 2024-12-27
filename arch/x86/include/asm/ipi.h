/**
 * Interprocessor interrupts. (calling functions on other CPUs)
 * Copyright (C) 2024  dbstream
 */
#ifndef __ASM_IPI_H
#define __ASM_IPI_H 1

/**
 * Send a call_on_cpu IPI to the target CPU.
 */
extern void
arch_send_call_on_cpu_IPI (unsigned int cpu);

/**
 * Architecture-specific function to stop all other CPUs on panic.
 */
extern void
arch_panic_stop_others (void);

#endif /* __ASM_IPI_H */
