// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/Ke/irq.h
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

#include <OS/status.h>

struct KIRQ;

void KeHandleInterruptVector(unsigned int vector);

void KeHandleLocalTimerInterrupt(void);

void KeHandleRescheduleIPI(void);

void KeHandleSchedTimerRecalcIPI(void);

/* Interrupt vector management */

void KeReserveInterruptVector(unsigned int vector);

// TODO: figure out how we want to do per CPU interrupt vector things...

OSSTATUS KeAllocateInterruptVector(KIRQ *irq, unsigned int *vector);

void KeFreeInterruptVector(KIRQ *irq);

