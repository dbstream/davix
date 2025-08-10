// SPDX-License-Identifier: GPL-3.0
/*
 * File: Ke/timer.cc
 *
 * Copyright (C) 2025  dbstream
 */
#include <Hal/interrupt.h>
#include <Ke/irq.h>
#include <Ke/log.h>
#include <Ke/timer.h>

/**
 * KeHandleLocalTimerInterrupt - handle the Local APIC Timer interrupt.
 */
void KeHandleLocalTimerInterrupt(void)
{
	KePrintf("TIMER INTERRUPT WERKS!!!1!1!\n");
}

