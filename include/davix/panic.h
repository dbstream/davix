/**
 * Kernel panic
 * Copyright (C) 2024  dbstream
 */
#ifndef _DAVIX_PANIC_H
#define _DAVIX_PANIC_H 1

/**
 * panic keeps a list of callbacks to call after acquiring the panic context.
 * Callbacks run in the reverse order in which they were registered.
 *
 * Rules for panic callbacks:
 *   - panic callbacks must be safe to call in an NMI context (because a kernel
 *     panic can happen anywhere).
 *   - panic callbacks cannot depend on anything but the simplest kernel
 *     functionality (because kernel panics usually imply compromised
 *     reliability).
 */

struct panic_callback {
	struct panic_callback *next;
	void (*callback) (void);
	struct panic_callback **link;
};

/**
 * Register a panic callback. cb->callback must be filled in with a function
 * pointer.
 */
extern void
register_panic_callback (struct panic_callback *cb);

/**
 * Deregister a panic callback. cb must have been previously passed to
 * register_panic_callback().
 */
extern void
deregister_panic_callback (struct panic_callback *cb);

/**
 * Check if a panic is currently in progress.
 */
extern int
in_panic (void);

/**
 * If in_panic() returns true, this function should be called on non-maskable
 * interrupts such as NMI or #MCE.
 */
extern void
panic_nmi_handler (void);

extern void
panic_stop_self (void);

/**
 * Trigger a kernel panic.
 */
__attribute__ ((format (printf, 1, 2)))
extern void
panic (const char *fmt, ...);

#endif /* _DAVIX_PANIC_H */
