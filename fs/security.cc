/**
 * Filesystem security checks.
 * Copyright (C) 2025-present  dbstream
 */
#include <davix/fs_types.h>
#include <davix/fs.h>
#include <davix/task.h>
#include <uapi/davix/stat.h>

static bool
i_check_permission (INode *inode, unsigned int requested, bool root_capable)
{
	FSContext *ctx = get_current_task ()->ctx_fs;
	if (!ctx)
		return false;
	ctx->lock.lock_dpc ();
	uid_t uid = ctx->fs_uid;
	gid_t gid = ctx->fs_gid;
	ctx->lock.unlock_dpc ();

	inode->i_lock.lock_dpc ();
	mode_t mode = inode->mode;
	uid_t i_uid = inode->uid;
	gid_t i_gid = inode->gid;
	inode->i_lock.unlock_dpc ();

	mode_t permissions;
	if (uid == i_uid)
		permissions = mode >> 6;
	else if (gid == i_gid)
		permissions = mode >> 3;
	else
		permissions = mode;

	requested &= ~permissions;
	return !requested || (root_capable && !uid);
}

bool
i_may_read (INode *inode)
{
	return i_check_permission (inode, __S_IROTH, true);
}

bool
i_may_write (INode *inode)
{
	return i_check_permission (inode, __S_IWOTH, true);
}

bool
i_may_exec (INode *inode)
{
	return i_check_permission (inode, __S_IXOTH, false);
}

bool
i_may_search_dir (INode *inode)
{
	return i_check_permission (inode, __S_IXOTH, true);
}

