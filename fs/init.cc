/**
 * Filesystem initialization.
 * Copyright (C) 2025-present  dbstream
 */
#include <davix/fs.h>
#include "internal.h"

void
init_fs_caches (void)
{
	init_dentry_cache ();
	init_mount_table ();
	register_tmpfs ();
}

