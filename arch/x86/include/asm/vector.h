/* SPDX-License-Identifier: MIT */
#ifndef __ASM_VECTOR_H
#define __ASM_VECTOR_H

/*
 * Vector numbers used by x86 interrupt handling.
 */

/*
 * Dynamically managed vectors.
 */
#define FIRST_DYNAMIC_VECTOR 32 /* The first dynamically allocated vector. */
#define LAST_DYNAMIC_VECTOR 252 /* The last dynamically allocated vector. */

#define SMP_NOTIFY_VECTOR 253	/* Notify about cross-CPU function calls */
#define TIMER_VECTOR 254	/* Local APIC timer */
#define SPURIOUS_VECTOR 255	/* Local APIC spurious */

#endif /* __ASM_VECTOR_H */
