// SPDX-License-Identifier: GPL-3.0
/*
 * File: Hal/irq/internal.h
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

void HalInitializeIRQSubsystem(void);

void HalInitializeLocalAPIC(void);

void HalInitializeLocalAPICNonBSP(void);

void HalAddIOAPIC(unsigned long address, unsigned int gsi_base);

void HalAddIRQOverride(unsigned int source, unsigned int dest,
		bool force_active_hi, bool force_active_lo,
		bool force_tgm_edge, bool force_tgm_level);

void HalPrintIRQOverrideTable(void);

