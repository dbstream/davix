// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/Ke/irq.h
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

void KeHandleInterruptVector(unsigned int vector);

void KeHandleLocalTimerInterrupt(void);

