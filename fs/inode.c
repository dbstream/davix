/**
 * inode cache
 * Copyright (C) 2025-present  dbstream
 */
#include <davix/fs.h>
#include <davix/panic.h>
#include <davix/slab.h>
#include <davix/string.h>

static struct slab *fs_slab;
static struct slab *inode_slab;

void
init_inode_cache (void)
{
	fs_slab = slab_create ("struct filesystem", sizeof (struct filesystem));
	if (!fs_slab)
		panic ("init_inode_cache: failed to create fs_slab!");

	inode_slab = slab_create ("struct inode", sizeof (struct inode));
	if (!inode_slab)
		panic ("init_inode_cache: failed to create inode_slab!");
}

void
__fs_put (struct filesystem *fs)
{
	fs->ops->close (fs);
	slab_free (fs);
}

void
__iput (struct inode *inode)
{
	spin_lock (&inode->fs->fs_inodes_lock);
	list_delete (&inode->fs_inodes);
	spin_unlock (&inode->fs->fs_inodes_lock);

	if (inode->ops->free_inode)
		inode->ops->free_inode (inode);

	slab_free (inode);
}

static const struct filesystem_ops default_filesystem_ops = {};

static errno_t
null_do_mount (struct filesystem **fs, struct vnode **root,
		struct fs_mount_args *args)
{
	(void) fs;
	(void) root;
	(void) args;

	return EINVAL;
}

static const struct filesystem_driver null_filesystem_driver = {
	.name = "(null driver)",
	.do_mount = null_do_mount
};

struct filesystem *
new_filesystem (void)
{
	struct filesystem *fs = slab_alloc (fs_slab);
	if (!fs)
		return NULL;

	refcount_set (&fs->refcount, 1);
	spinlock_init (&fs->fs_inodes_lock);
	list_init (&fs->fs_inodes);

	fs->ops = &default_filesystem_ops;
	fs->driver = &null_filesystem_driver;
	return fs;
}

struct inode *
alloc_inode (struct filesystem *fs)
{
	struct inode *inode = slab_alloc (inode_slab);
	if (!inode)
		return NULL;

	memset (inode, 0, sizeof (*inode));

	inode->i_nlink = 0;
	inode->i_mode = 0;
	inode->i_uid = 0;
	inode->i_gid = 0;
	spinlock_init (&inode->i_lock);
	refcount_set (&inode->refcount, 1);
	inode->fs = fs;

	spin_lock (&fs->fs_inodes_lock);
	list_insert (&fs->fs_inodes, &inode->fs_inodes);
	spin_unlock (&fs->fs_inodes_lock);
	return inode;
}
