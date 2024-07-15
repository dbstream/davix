/**
 * High-Precision Event Timer.
 * Copyright (C) 2024  dbstream
 */
#ifndef _ASM_HPET_H
#define _ASM_HPET_H 1

#include <davix/stdbool.h>
#include <davix/stdint.h>
#include <davix/time.h>

extern bool
x86_init_hpet (void);

extern nsecs_t
x86_hpet_nsecs (void);

extern uint32_t hpet_period;
extern uint32_t hpet_period_ns, hpet_period_frac;

extern uint64_t
hpet_read_counter (void);

#endif /* _ASM_HPET_H */
