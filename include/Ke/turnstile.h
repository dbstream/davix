// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/Ke/turnstile.h
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

struct KTURNSTILE;

enum : int {
	KTURNSTILE_Q_WRITER	= 0,
	KTURNSTILE_Q_READER	= 1,
	KTURNSTILE_NUM_Q	= 2,
};

KTURNSTILE *KeTurnstileGet(void *address);

void KeTurnstilePut(KTURNSTILE *tstl);

void KeTurnstileBlock(KTURNSTILE *tstl, int queue);

void KeTurnstileWake(KTURNSTILE *tstl, int queue);

