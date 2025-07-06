/**
 * Davix tmpfs
 * Copyright (C) 2025-present  dbstream
 */
#include <davix/atomic.h>
#include <davix/fs_types.h>
#include <davix/fs.h>
#include <davix/refstr.h>
#include <uapi/davix/errno.h>
#include <uapi/davix/stat.h>

static ino_t
tmpfs_next_ino (Filesystem *fs)
{
	return atomic_fetch_inc (&fs->fs_private.ino, mo_relaxed);
}

static const INodeOps *
get_tmpfs_dir_ops (void);

static const INodeOps *
get_tmpfs_regular_ops (void);

static const INodeOps *
get_tmpfs_special_ops (void);

static const INodeOps *
get_tmpfs_symlink_ops (void);

static INode *
tmpfs_new_inode (Filesystem *fs, uid_t uid, gid_t gid, mode_t mode)
{
	INode *inode = new_inode (fs, nullptr);
	if (!inode)
		return nullptr;

	inode->uid = uid;
	inode->gid = gid;
	inode->mode = mode;
	inode->nlink = 0;
	inode->rdev = 0;
	inode->size = 0;
	inode->ino = tmpfs_next_ino (fs);
	if (__S_ISREG(mode))
		inode->i_ops = get_tmpfs_regular_ops ();
	else if (__S_ISDIR(mode))
		inode->i_ops = get_tmpfs_dir_ops ();
	else if (__S_ISLNK(mode))
		inode->i_ops = get_tmpfs_symlink_ops ();
	else
		inode->i_ops = get_tmpfs_special_ops ();
	return inode;
}

/*
 * Tmpfs common operations:
 */

static int
tmpfs_chmod (INode *inode, mode_t mode)
{
	inode->i_lock.lock_dpc ();
	inode->mode = mode;
	inode->i_lock.unlock_dpc ();
	return 0;
}

static int
tmpfs_chown (INode *inode, uid_t uid, gid_t gid)
{
	inode->i_lock.lock_dpc ();
	inode->uid = uid;
	inode->gid = gid;
	inode->i_lock.unlock_dpc ();
	return 0;
}

static int
tmpfs_stat (INode *inode, kstat *stat)
{
	/* TODO: */
	(void) inode;
	(void) stat;
	return EOPNOTSUPP;
}

/*
 * Tmpfs directory operations:
 */

static int
tmpfs_lookup (INode *dir, DEntry *de)
{
	(void) dir;

	d_set_nocache (de);
	return ENOENT;
}

static int
tmpfs_unlink (INode *dir, DEntry *de)
{
	INode *inode = d_inode (de);
	inode->i_lock.lock_dpc ();
	mode_t mode = inode->mode;
	inode->i_lock.unlock_dpc ();

	if (__S_ISDIR(mode) && inode->nlink != 2) {
		/*
		 * Not an empty directory.
		 */
		return ENOTEMPTY;
	}

	/*
	 * Refcount is biased.
	 */
	dput (de);
	d_unlink (de);

	if (__S_ISDIR(mode)) {
		i_set_nlink (inode, 0);
		i_decr_nlink (dir, 1);
	} else
		i_decr_nlink (inode, 1);

	return 0;
}

static int
tmpfs_mknod (INode *dir, DEntry *de,
		uid_t uid, gid_t gid, mode_t mode, dev_t device)
{
	INode *inode = tmpfs_new_inode (dir->fs, uid, gid, mode);
	if (!inode)
		return ENOMEM;

	inode->rdev = device;
	i_set_nlink (inode, 1);
	d_set_inode (de, inode);
	/*
	 * Bias the refcount.
	 */
	dget(de);
	return 0;
}

static int
tmpfs_mkdir (INode *dir, DEntry *de,
		uid_t uid, gid_t gid, mode_t mode)
{
	INode *inode = tmpfs_new_inode (dir->fs, uid, gid, mode);
	if (!inode)
		return ENOMEM;

	i_set_nlink (inode, 2);
	i_incr_nlink (dir, 1);
	d_set_inode (de, inode);
	/*
	 * Bias the refcount.
	 */
	dget (de);
	return 0;
}

static int
tmpfs_symlink (INode *dir, DEntry *de,
		uid_t uid, gid_t gid, mode_t mode, const char *path)
{
	RefStr *path_str = make_refstr (path);
	if (!path_str)
		return ENOMEM;

	INode *inode = tmpfs_new_inode (dir->fs, uid, gid, mode);
	if (!inode) {
		put_refstr (path_str);
		return ENOMEM;
	}

	inode->i_private = path_str;
	i_set_nlink (inode, 1);
	i_incr_nlink (dir, 1);
	d_set_inode (de, inode);
	/*
	 * Bias the refcount.
	 */
	dget (de);
	return 0;
}

static int
tmpfs_link (INode *dir, DEntry *de, INode *inode)
{
	(void) dir;

	i_incr_nlink (inode, 1);
	d_set_inode (de, inode);
	/*
	 * Bias the refcount.
	 */
	dget (de);
	return 0;
}

static int
tmpfs_rename (INode *fromdir, DEntry *from, INode *todir, DEntry *to,
		unsigned int flags)
{
	bool exchange = (flags & RENAME_EXCHANGE) ? true : false;

	INode *from_inode = d_inode (from);
	INode *to_inode = d_inode (to);

	from_inode->i_lock.lock_dpc ();
	mode_t from_mode = from_inode->mode;
	from_inode->i_lock.unlock_dpc ();

	mode_t to_mode = __S_IFREG /* dummy value which is not __S_IFDIR */;
	if (to_inode) {
		to_inode->i_lock.lock_dpc ();
		to_mode = to_inode->mode;
		to_inode->i_lock.unlock_dpc ();
	}

	if (__S_ISDIR(from_mode) && !__S_ISDIR(to_mode))
		/*
		 * Replacing a nondirectory or ENOENT with a directory: increase
		 * link count of target directory.
		 */
		i_incr_nlink (todir, 1);
	else if (exchange && !__S_ISDIR(from_mode) && __S_ISDIR(to_mode))
		/*
		 * Exchanging a nondirectory with a directory: increase link
		 * count of source directory.
		 */
		i_incr_nlink (fromdir, 1);

	d_rename (from, to, flags);

	if (__S_ISDIR(from_mode) && (!exchange || !__S_ISDIR(to_mode)))
		/*
		 * Moved a directory out: decrease link count of source
		 * directory.
		 */
		i_decr_nlink (fromdir, 1);
	if (!__S_ISDIR(from_mode) && __S_ISDIR(to_mode))
		/*
		 * Replaced a directory with a nondirectory: decrease link count
		 * of target directory.
		 */
		i_decr_nlink (todir, 1);

	if (!exchange && to_inode) {
		i_decr_nlink (to_inode, 1);
		/*
		 * We effectively unlinked the target path, whose refcount was
		 * biased.
		 */
		dput (to);
	}

	return 0;
}

static int
tmpfs_tmpfile (INode *dir, INode **inode,
		uid_t uid, gid_t gid, mode_t mode)
{
	*inode = tmpfs_new_inode (dir->fs, uid, gid, mode);
	return (*inode) ? 0 : ENOMEM;
}

static int
tmpfs_open_dir (INode *dir, File *file)
{
	/* TODO: */
	(void) dir;
	(void) file;
	return EOPNOTSUPP;
}

/*
 * Tmpfs regular file operations:
 */

static int
tmpfs_open_regular (INode *inode, File *file)
{
	/* TODO: */
	(void) inode;
	(void) file;
	return EOPNOTSUPP;
}

static int
tmpfs_truncate (INode *inode, off_t length)
{
	/* TODO: */
	(void) inode;
	(void) length;
	return EOPNOTSUPP;
}

/*
 * Tmpfs symlink operations:
 */

static int
tmpfs_readlink (INode *inode, RefStr **out)
{
	*out = get_refstr ((RefStr *) inode->i_private);
	return 0;
}

static const INodeOps tmpfs_dir_ops = {
	.i_lookup = tmpfs_lookup,
	.i_unlink = tmpfs_unlink,
	.i_mknod = tmpfs_mknod,
	.i_mkdir = tmpfs_mkdir,
	.i_symlink = tmpfs_symlink,
	.i_link = tmpfs_link,
	.i_rename = tmpfs_rename,
	.i_chmod = tmpfs_chmod,
	.i_chown = tmpfs_chown,
	.i_stat = tmpfs_stat,
	.i_tmpfile = tmpfs_tmpfile,
	.i_open = tmpfs_open_dir,
};

static const INodeOps tmpfs_regular_ops = {
	.i_chmod = tmpfs_chmod,
	.i_chown = tmpfs_chown,
	.i_stat = tmpfs_stat,
	.i_open = tmpfs_open_regular,
	.i_truncate = tmpfs_truncate,
};

static const INodeOps tmpfs_special_ops = {
	.i_chmod = tmpfs_chmod,
	.i_chown = tmpfs_chown,
	.i_stat = tmpfs_stat,
};

static const INodeOps tmpfs_symlink_ops = {
	.i_chmod = tmpfs_chmod,
	.i_chown = tmpfs_chown,
	.i_stat = tmpfs_stat,
	.i_readlink = tmpfs_readlink,
};

static const INodeOps *
get_tmpfs_dir_ops (void)
{
	return &tmpfs_dir_ops;
}

static const INodeOps *
get_tmpfs_regular_ops (void)
{
	return &tmpfs_regular_ops;
}

static const INodeOps *
get_tmpfs_special_ops (void)
{
	return &tmpfs_special_ops;
}

static const INodeOps *
get_tmpfs_symlink_ops (void)
{
	return &tmpfs_symlink_ops;
}

static int
mount_tmpfs (const char *source, unsigned long mount_flags, FilesystemType *typ,
		const void *data, Filesystem **out_fs, DEntry **root)
{
	(void) source;
	(void) data;

	Filesystem *fs = new_filesystem (typ, nullptr, mount_flags);
	if (!fs)
		return ENOMEM;

	INode *root_inode = new_inode (fs, nullptr);
	if (!root_inode) {
		fs_put (fs);
		return ENOMEM;
	}

	root_inode->ino = 1;
	fs->fs_private.ino = 2;

	root_inode->i_ops = &tmpfs_dir_ops;
	root_inode->uid = 0;
	root_inode->gid = 0;
	root_inode->mode = __S_IFDIR | 0755 /* drwxr-xr-x */;
	root_inode->nlink = 2;
	root_inode->rdev = 0;
	root_inode->size = 0;

	DEntry *de = allocate_root_dentry (fs);
	if (!de) {
		iput (root_inode);
		fs_put (fs);
		return ENOMEM;
	}

	d_set_inode (de, root_inode);
	iput (root_inode);
	*out_fs = fs;
	*root = de;
	return 0;
}

static void
trim_tmpfs (Filesystem *fs)
{
	(void) fs;
}

static FilesystemType tmpfs_type = {
	.name = "tmpfs",
	.mount_fs = mount_tmpfs,
	.trim_fs = trim_tmpfs,
};

void
register_tmpfs (void)
{
	register_filesystem (&tmpfs_type);
}

