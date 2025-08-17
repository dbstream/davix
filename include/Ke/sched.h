// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/Ke/sched.h
 * Scheduler interface.
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

struct KTHREAD;

void KeReschedule(void);

bool KeWakeThread(KTHREAD *thread);

