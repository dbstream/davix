// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/asm/apic.h
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

void apic_send_IPI(unsigned int value, unsigned int target_apicid);

