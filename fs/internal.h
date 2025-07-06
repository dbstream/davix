/**
 * VFS internal interfaces.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

void
init_dentry_cache (void);

void
init_mount_table (void);

void
init_vfs_inodes (void);

void
register_tmpfs (void);

