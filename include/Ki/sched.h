// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/Ki/sched.h
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

struct KTHREAD;

void KiFinalizeTaskSwitch(KTHREAD *previous);

void KiSetupInitialThreadContext(void);

KTHREAD *HalSwitchThread(KTHREAD *previous, KTHREAD *next);

void KiInitializeScheduler(void);

void KiInitializeSchedulerOnSecondaryProcessor(void);

