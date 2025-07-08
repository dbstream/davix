/**
 * Filesystem initialization.
 * Copyright (C) 2025-present  dbstream
 */
#include <davix/fs_types.h>
#include <davix/fs.h>
#include <davix/path.h>
#include "internal.h"

void
init_fs_caches (void)
{
	init_dentry_cache ();
	init_vfs_inodes ();
	init_mount_table ();
	register_tmpfs ();

	Mount *mnt = do_mount_root ("tmpfs", "", 0, nullptr);
	DEntry *de = dget (mnt->root);

	Path path = { mnt, de };
	init_fs_context.root = path_get (path);
	init_fs_context.cwd = path_get (path);
}

