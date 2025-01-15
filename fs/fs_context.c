/**
 * Process fs_context tracking.
 * Copyright (C) 2025-present  dbstream
 *
 * Keep track of the process working directory (CWD) and the current filesystem
 * root.
 */
#include <davix/fs.h>
#include <davix/printk.h>
#include <davix/kmalloc.h>
#include <davix/sched_types.h>

errno_t
fsctx_create (struct path root)
{
	if (get_current_task ()->fs != NULL)
		return EINVAL;

	// TODO: convert this to using 'struct slab'.

	struct fs_context *ctx = kmalloc (sizeof (*ctx));
	if (!ctx)
		return ENOMEM;

	refcount_set (&ctx->refcount, 1);
	spinlock_init (&ctx->cwd_root_lock);
	mutex_init (&ctx->fsctx_mutex);
	ctx->root = path_get (root);
	ctx->cwd = path_get (root);
	get_current_task ()->fs = ctx;
	return ESUCCESS;
}

void
__fsctx_put (struct fs_context *ctx)
{
	path_put (ctx->cwd);
	path_put (ctx->root);
	kfree (ctx);
}
