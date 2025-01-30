/**
 * tmpfs filesystem.
 * Copyright (C) 2025-present  dbstream
 */
#include <davix/fs.h>
#include <davix/slab.h>

static const struct inode_ops tmpfs_inode_operations;

static errno_t
tmpfs_mknod (struct inode *parent, struct vnode *vnode,
		uid_t uid, gid_t gid, mode_t mode, dev_t dev)
{
	(void) dev;

	struct inode *inode = alloc_inode (parent->fs);
	if (!inode)
		return ENOMEM;

	inode->ops = &tmpfs_inode_operations;
	inode->i_uid = uid;
	inode->i_gid = gid;
	inode->i_mode = mode;
	if (S_ISDIR (mode))
		inode->i_nlink = 2;
	else
		inode->i_nlink = 1;

	if (S_ISDIR (mode)) {
		spin_lock (&parent->i_lock);
		parent->i_nlink++;
		spin_unlock (&parent->i_lock);
	} else if (S_ISBLK (mode) || S_ISCHR (mode))
		inode->i_dev = dev;

	vnode_install (vnode, inode);
	vn_get (vnode);
	return ESUCCESS;
}

static errno_t
tmpfs_unlink (struct inode *parent, struct vnode *vnode)
{
	struct inode *child = vnode->vn_inode;

	spin_lock (&child->i_lock);
	mode_t mode = child->i_mode;
	nlink_t nlink = child->i_nlink;

	if (S_ISDIR (mode) && nlink != 2) {
		spin_unlock (&child->i_lock);
		return EEXIST;
	}

	if (S_ISDIR (mode))
		child->i_nlink = 0;
	else
		child->i_nlink--;

	spin_unlock (&child->i_lock);
	vn_put (vnode);
	vnode_unlink (vnode);

	if (S_ISDIR (mode)) {
		spin_lock (&parent->i_lock);
		parent->i_nlink--;
		spin_unlock (&parent->i_lock);
	}

	return ESUCCESS;
}

static const struct filesystem_ops tmpfs_operations = {};

static const struct inode_ops tmpfs_inode_operations = {
	.mknod = tmpfs_mknod,
	.unlink = tmpfs_unlink
};

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
	inode->i_nlink = 2;

	vnode->vn_inode = inode;
	*out_fs = fs;
	*out_root = vnode;
	return ESUCCESS;
}

const struct filesystem_driver tmpfs_filesystem_driver = {
	.name = "tmpfs",
	.do_mount = tmpfs_mount
};

