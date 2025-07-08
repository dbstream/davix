/**
 * struct Path
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

struct DEntry;
struct Mount;

#include <davix/refcount.h>
#include <davix/spinlock.h>
#include <davix/types.h>

struct Path {
	Mount *mount;
	DEntry *dentry;
};

Path
path_get (Path path);

void
path_put (Path path);

struct FSContext {
	refcount_t refcount;
	spinlock_t lock;
	uid_t fs_uid;
	gid_t fs_gid;
	Path root;
	Path cwd;
};

FSContext *
fsctx_get (FSContext *ctx);

void
fsctx_put (FSContext *ctx);

extern FSContext init_fs_context;

