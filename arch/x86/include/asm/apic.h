// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/asm/apic.h
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

#include <stdint.h>

void apic_send_IPI(uint32_t value, uint32_t target_apicid);

uint32_t apic_read_id(void);

extern unsigned int halCpuToApic[];

