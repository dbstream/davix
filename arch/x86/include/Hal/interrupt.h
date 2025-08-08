// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/Hal/interrupt.h
 * Raw interrupt state management.
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

static inline void HalDisableRawIRQs(void)
{
	asm volatile("cli" ::: "memory");
}

static inline void HalEnableRawIRQs(void)
{
	asm volatile("sti" ::: "memory");
}

static inline void HalWaitForInterrupt(void)
{
	asm volatile("nop; hlt" ::: "memory");
}

static inline void HalEnableRawIRQsAndWaitForInterrupt(void)
{
	asm volatile("sti; hlt" ::: "memory");
}

void HalAcknowledgeInterrupt(void);

static inline void HalAcknowledgeLocalTimerInterrupt(void)
{
	/* already done */
}

