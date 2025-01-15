/**
 * tmpfs filesystem.
 * Copyright (C) 2025-present  dbstream
 */
#include <davix/fs.h>
#include <davix/slab.h>

static const struct filesystem_ops tmpfs_operations = {};
static const struct inode_ops tmpfs_inode_operations = {};

static errno_t
tmpfs_mount (struct filesystem **out_fs, struct vnode **out_root,
		struct fs_mount_args *args)
{
	(void) args;

	/**
	 * TODO: make filesystem mounting easier and better.
	 */

	struct filesystem *fs = new_filesystem ();
	if (!fs)
		return ENOMEM;

	struct inode *inode = alloc_inode (fs);
	if (!inode) {
		slab_free (fs);
		return ENOMEM;
	}

	struct vnode *vnode = alloc_root_vnode (fs);
	if (!vnode) {
		slab_free (inode);
		slab_free (fs);
	}

	fs->ops = &tmpfs_operations;
	inode->ops = &tmpfs_inode_operations;
	inode->i_mode = args->mode;

	vnode->vn_inode = inode;
	*out_fs = fs;
	*out_root = vnode;
	return ESUCCESS;
}

const struct filesystem_driver tmpfs_filesystem_driver = {
	.name = "tmpfs",
	.do_mount = tmpfs_mount
};

