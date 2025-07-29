// SPDX-License-Identifier: GPL-3.0
/*
 * include/Ke/log.h
 * Debug logging facilities.
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

void KePuts(const char *str);

void KePrintf [[gnu::format(printf, 1, 2)]] (const char *fmt, ...);

void KePanic [[noreturn, gnu::format(printf, 1, 2)]] (const char *fmt, ...);

