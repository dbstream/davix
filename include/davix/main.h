/**
 * Kernel main() function and subsystem initialization functions.
 * Copyright (C) 2024  dbstream
 */
#ifndef _DAVIX_MAIN_H
#define _DAVIX_MAIN_H 1

extern void
main (void);

extern void
arch_init (void);

extern void
arch_init_late (void);

extern unsigned long boot_module_start, boot_module_end;

#endif /* _DAVIX_MAIN_H */
