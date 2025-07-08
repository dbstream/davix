/**
 * Path lookup.
 * Copyright (C) 2025-present  dbstream
 */
#include <davix/fs.h>
#include <davix/kmalloc.h>
#include <davix/path.h>
#include <davix/task.h>

FSContext init_fs_context = {
	.refcount = 1,
	.fs_uid = 0,
	.fs_gid = 0,
};

Path
path_get (Path path)
{
	Path ret;
	ret.mount = mnt_get (path.mount);
	ret.dentry = dget (path.dentry);
	return ret;
}

void
path_put (Path path)
{
	dput (path.dentry);
	mnt_put (path.mount);
}

FSContext *
fsctx_get (FSContext *fsctx)
{
	refcount_inc (&fsctx->refcount);
	return fsctx;
}

void
fsctx_put (FSContext *fsctx)
{
	if (!fsctx)
		return;

	if (refcount_dec (&fsctx->refcount)) {
		path_put (fsctx->cwd);
		path_put (fsctx->root);
		kfree (fsctx);
	}
}

