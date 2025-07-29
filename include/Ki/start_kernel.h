// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/Ki/start_kernel.h
 * Kernel-mode startup entry points.
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

void KiInitializeEarlySubsystems(void);

void KiStartKernel(void);

