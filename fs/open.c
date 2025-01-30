/**
 * stat() system call.
 * Copyright (C) 2025  dbstream
 */
#include <davix/printk.h>
#include <davix/string.h>
#include <davix/syscall.h>
#include "internal.h"

#include <asm/usercopy.h>

/**
 * NOTE:  in the future, it is desirable to do some other form of path
 * copying from userspace to the kernel, because this is just horrible.
 *
 * On x86, where the stack size is 16KiB, this buffer uses 1/4th of the
 * available stack space.  Luckily, syscalls don't nest.
 */

#define PATH_MAX	4096

#include <asm/task.h>
_Static_assert (4 * PATH_MAX <= TASK_STK_SIZE, "rework filesystem syscalls!");

/**
 * Lookup a path.
 */
static errno_t
lookup_path (struct path *out, const char *path,
		int dirfd, struct pathwalk_info *info)
{
	struct pathwalk_state state;
	errno_t e = pathwalk_begin (&state, dirfd, info);
	if (e != ESUCCESS)
		return e;

	e = do_pathwalk (&state, path);
	if (e == ESUCCESS) {
		if (state.has_result_component)
			*out = path_get (state.result_component);
		else {
			printk (PR_ERR "lookup_path: do_pathwalk() returned ESUCCESS, but has_result_component is false\n");
			e = EINVAL;
		}
	}

	pathwalk_finish (&state);
	return e;
}

static errno_t
do_stat_inode (struct stat *buf, struct inode *inode)
{
	if (inode->ops->stat)
		return inode->ops->stat (buf, inode);

	buf->st_valid = STAT_ATTR_NLINK | STAT_ATTR_MODE |
			STAT_ATTR_UID | STAT_ATTR_GID | STAT_ATTR_DEV;
	spin_lock (&inode->i_lock);
	buf->st_nlink = inode->i_nlink;
	buf->st_mode = inode->i_mode;
	buf->st_uid = inode->i_uid;
	buf->st_gid = inode->i_gid;
	buf->st_dev = inode->i_dev;
	spin_unlock (&inode->i_lock);
	return ESUCCESS;
}

static errno_t
do_stat (struct stat *buf, const char *pathname,
		int dirfd, struct pathwalk_info *info)
{
	struct path path;
	errno_t e = lookup_path (&path, pathname, dirfd, info);
	if (e != ESUCCESS)
		return e;

	spin_lock (&path.vnode->vn_lock);
	if (path.vnode->vn_flags & VN_NEEDLOOKUP)
		printk (PR_WARN "do_stat: lookup_path was called but VN_NEEDLOOKUP is still set\n");
	struct inode *inode = path.vnode->vn_inode;
	spin_unlock (&path.vnode->vn_lock);

	if (inode)
		e = do_stat_inode (buf, inode);
	else
		e = ENOENT;

	path_put (path);
	return e;
}

static errno_t
do_mknod (const char *pathname, int dirfd, struct pathwalk_info *info,
		mode_t mode, dev_t dev)
{
	struct path parent, target;
	struct inode *inode;

	switch (mode & S_IFMT) {
	case S_IFDIR:
	case S_IFCHR:
	case S_IFBLK:
	case S_IFREG:
	case S_IFIFO:
	case S_IFSOCK:
		break;
	default:
		return EINVAL;
	}

	struct pathwalk_state state;
	errno_t e = pathwalk_begin (&state, dirfd, info);
	if (e != ESUCCESS)
		return e;

	e = do_pathwalk (&state, pathname);
	if (e == ESUCCESS) {
		if (!state.has_parent_component || !state.has_result_component)
			e = EINVAL;
		else {
			parent = path_get (state.parent_component);
			target = path_get (state.result_component);
		}
	}

	pathwalk_finish (&state);
	if (e != ESUCCESS)
		return e;

	if (parent.mount != target.mount)
		e = EINVAL;

	if (e == ESUCCESS) {
		inode = parent.vnode->vn_inode;
		e = inode_permission_check (inode, S_IFDIR | S_IWUSR);
	}

	if (e == ESUCCESS) {
		i_lock_exclusive (inode);
		spin_lock (&target.vnode->vn_lock);
		if (target.vnode->vn_parent != parent.vnode)
			/** we raced against rename()  */
			e = EAGAIN;
		else if (target.vnode->vn_inode)
			/** don't create it if it already exists.  */
			e = EEXIST;
		spin_unlock (&target.vnode->vn_lock);

		if (e == ESUCCESS) {
			if (inode->ops->mknod)
				/**
				 * TODO: use process effective uid and gid.
				 */
				e = inode->ops->mknod (inode, target.vnode,
						0, 0, mode, dev);
			else
				e = ENOTSUP;
		}

		i_unlock_exclusive (inode);
	}

	path_put (target);
	path_put (parent);
	return e;
}

static errno_t
do_unlink (const char *path, int dirfd, struct pathwalk_info *info)
{
	struct path parent, target;
	struct inode *inode, *ichild;

	struct pathwalk_state state;
	errno_t e = pathwalk_begin (&state, dirfd, info);
	if (e != ESUCCESS)
		return e;

	e = do_pathwalk (&state, path);
	if (e == ESUCCESS) {
		if (!state.has_parent_component || !state.has_result_component)
			e = EINVAL;
		else {
			parent = path_get (state.parent_component);
			target = path_get (state.result_component);
		}
	}

	pathwalk_finish (&state);
	if (e != ESUCCESS)
		return e;

	if (parent.mount != target.mount)
		e = EINVAL;

	if (e == ESUCCESS) {
		inode = parent.vnode->vn_inode;
		ichild = target.vnode->vn_inode;

		if (!inode || !ichild)
			e = ENOENT;
		else
			e = inode_permission_check (inode, S_IWUSR | S_IXUSR);

		if (info->flags & O_FILETYPE) {
			spin_lock (&ichild->i_lock);
			mode_t mode = ichild->i_mode;
			spin_unlock (&ichild->i_lock);

			if ((mode & S_IFMT) != (info->mode & S_IFMT))
				e = S_ISDIR (info->mode) ? ENOTDIR : EISDIR;
		}
	}

	if (e == ESUCCESS) {
		i_lock_exclusive (inode);
		spin_lock (&target.vnode->vn_lock);
		if (target.vnode->vn_flags & VN_UNHOOKED)
			e = ENOENT;
		else if (target.vnode->vn_parent != parent.vnode)
			/* we raced against rename()  */
			e = ENOENT;
		spin_unlock (&target.vnode->vn_lock);

		if (e == ESUCCESS && inode->ops->unlink)
			e = inode->ops->unlink (inode, target.vnode);
		else if (e == ESUCCESS)
			e = ENOTSUP;

		i_unlock_exclusive (inode);
	}

	path_put (target);
	path_put (parent);
	return e;
}

static errno_t
get_path_from_userspace (char *kbuf, const char *user)
{
	kbuf[PATH_MAX - 1] = 0;
	errno_t e = strncpy_from_userspace (kbuf, user, PATH_MAX + 1);
	if (e == ESUCCESS && kbuf[PATH_MAX - 1])
		e = ENAMETOOLONG;
	return e;
}

SYSCALL6 (void, stat, struct stat *, buf, size_t, buf_size, const char *, path,
		int, dirfd, struct pathwalk_info *, info, size_t, info_size)
{
	char kpath[PATH_MAX];
	struct stat kstat;
	struct pathwalk_info kpathwalk_info;

	if (info_size > sizeof (kpathwalk_info))
		syscall_return_error (E2BIG);

	memset (&kpathwalk_info, 0, sizeof (kpathwalk_info));
	errno_t e = ESUCCESS;
	if (info_size)
		e = memcpy_from_userspace (&kpathwalk_info, info, info_size);
	if (e != ESUCCESS)
		syscall_return_error (e);

	e = get_path_from_userspace (kpath, path);
	if (e != ESUCCESS)
		syscall_return_error (e);

	memset (&kstat, 0, sizeof (kstat));
	kstat.st_valid = 0;
	e = do_stat (&kstat, kpath, dirfd, &kpathwalk_info);
	if (e != ESUCCESS)
		syscall_return_error (e);

	e = memcpy_to_userspace (buf, &kstat,
			min (size_t, buf_size, sizeof (kstat)));
	if (e == ESUCCESS)
		syscall_return_void;
	syscall_return_error (e);
}

SYSCALL6 (void, mknod, const char *, path, int, dirfd,
		struct pathwalk_info *, info, size_t, info_size,
		mode_t, mode, dev_t, dev)
{
	char kpath[PATH_MAX];
	struct pathwalk_info kpathwalk_info;

	if (info_size > sizeof (kpathwalk_info))
		syscall_return_error (E2BIG);

	memset (&kpathwalk_info, 0, sizeof (kpathwalk_info));
	errno_t e = ESUCCESS;
	if (info_size)
		e = memcpy_from_userspace (&kpathwalk_info, info, info_size);
	if (e != ESUCCESS)
		syscall_return_error (e);

	kpathwalk_info.flags = O_NOFOLLOW;

	e = get_path_from_userspace (kpath, path);
	if (e != ESUCCESS)
		syscall_return_error (e);

	e = do_mknod (kpath, dirfd, &kpathwalk_info, mode, dev);
	if (e == ESUCCESS)
		syscall_return_void;
	syscall_return_error (e);
}

SYSCALL4 (void, unlink, const char *, path, int, dirfd,
		struct pathwalk_info *, info, size_t, info_size)
{
	char kpath[PATH_MAX];
	struct pathwalk_info kpathwalk_info;

	if (info_size > sizeof (kpathwalk_info))
		syscall_return_error (E2BIG);

	memset (&kpathwalk_info, 0, sizeof (kpathwalk_info));
	errno_t e = ESUCCESS;
	if (info_size)
		e = memcpy_from_userspace (&kpathwalk_info, info, info_size);
	if (e != ESUCCESS)
		syscall_return_error (e);

	kpathwalk_info.flags = O_NOFOLLOW | (kpathwalk_info.flags & O_FILETYPE);

	e = get_path_from_userspace (kpath, path);
	if (e != ESUCCESS)
		syscall_return_error (e);

	e = do_unlink (kpath, dirfd, &kpathwalk_info);
	if (e == ESUCCESS)
		syscall_return_void;
	syscall_return_error (e);
}
