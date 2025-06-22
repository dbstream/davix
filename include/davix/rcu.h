/**
 * Read-Copy-Update
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

void
rcu_read_lock (void);

void
rcu_read_unlock (void);

void
rcu_enable (void);

void
rcu_disable (void);

void
rcu_quiesce (void);

struct RCUHead;

typedef void (*RCUCallback) (RCUHead *);

struct RCUHead {
	RCUHead *next;
	RCUCallback function;
};

void
rcu_call (RCUHead *head, RCUCallback function);

