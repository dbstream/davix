/**
 * Mountpoint management.
 * Copyright (C) 2025-present  dbstream
 */
#include <davix/fs.h>
#include <davix/panic.h>
#include <davix/printk.h>
#include <davix/slab.h>
#include <davix/string.h>

extern const struct filesystem_driver tmpfs_filesystem_driver;

#if 0
static const struct filesystem_driver *builtin_filesystem_drivers[] = {
	&tmpfs_filesystem_driver,
	NULL
};
#endif

static struct slab *mnt_slab;

void
__mnt_put (struct mountpoint *mnt)
{
	printk (PR_INFO "unmounting a mount...\n");
	vn_put (mnt->root);
	fs_put (mnt->fs);
	slab_free (mnt);
}

static struct path
mount_root (void)
{
	struct fs_mount_args args = {
		.mode = S_IFDIR | 0700
	};

	struct filesystem *fs;
	struct vnode *root;
	errno_t e = tmpfs_filesystem_driver.do_mount (&fs, &root, &args);
	if (e != ESUCCESS)
		panic ("mount_root: do_mount() failed with error %d", e);

	struct mountpoint *mnt = slab_alloc (mnt_slab);
	if (!mnt)
		panic ("mount_root: failed to allocate memory for struct mountpoint");

	memset (mnt, 0, sizeof (*mnt));
	refcount_set (&mnt->refcount, 2);
	mnt->fs = fs;
	mnt->root = vn_get (root);

	printk (PR_INFO "mounted the root tmpfs\n");
	return (struct path) { .mount = mnt, .vnode = root };
}

void
init_mounts (void)
{
	mnt_slab = slab_create ("struct mountpoint", sizeof (struct mountpoint));
	if (!mnt_slab)
		panic ("init_mounts: failed to create struct mountpoint slab!");


	struct path root = mount_root ();
	errno_t e = fsctx_create (root);
	if (e != ESUCCESS)
		panic ("init_mounts: failed to create root filesystem context object!");

	path_put (root);
}

