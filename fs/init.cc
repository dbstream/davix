/**
 * Filesystem initialization.
 * Copyright (C) 2025-present  dbstream
 */
#include <davix/fs_types.h>
#include <davix/fs.h>
#include <davix/path.h>
#include <davix/printk.h>
#include <uapi/davix/fcntl.h>
#include <uapi/davix/stat.h>
#include "internal.h"

static inline void xrename(const char *from, const char *to)
{
	int ret = ksys_rename(AT_FDCWD, from, AT_FDCWD, to, 0);
	printk(PR_NOTICE "rename(\"%s\", \"%s\") = %d\n", from, to, ret);
}

static inline void xunlink(const char *path)
{
	int ret = ksys_unlink(AT_FDCWD, path);
	printk(PR_NOTICE "unlink(\"%s\") = %d\n", path, ret);
}

static inline void xmkdir(const char *path)
{
	int ret = ksys_mkdir(AT_FDCWD, path, 0755);
	printk(PR_NOTICE "mkdir(\"%s\") = %d\n", path, ret);
}

static inline void xmknod(const char *path, mode_t mode, dev_t dev)
{
	int ret = ksys_mknod(AT_FDCWD, path, mode, dev);
	printk(PR_NOTICE "mknod(\"%s\", %o, %lu) = %d\n", path, mode, dev, ret);
}

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

	xmkdir("/a");
	xmknod("/a/foo", __S_IFREG | 0755, 0);
	xmknod("/a/foo", __S_IFREG | 0755, 0);
	xrename("/a/foo", "/a/bar");
	xmknod("/a/foo", __S_IFREG | 0755, 0);
	xmknod("/a/bar", __S_IFREG | 0755, 0);
}

