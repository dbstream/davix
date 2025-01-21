/**
 * Things internal to the VFS.
 * Copyright (C) 2025-present  dbstream
 */
#ifndef _FS_INTERNAL_H
#define _FS_INTERNAL_H 1

#include <davix/fs.h>
#include <davix/uapi/openat.h>

/**
 * Test if the current user has the required permissions to open an inode.
 *
 * @inode	inode to open.
 * @mode	permissions to check for.  bitmask of S_IRUSR, S_IWUSR, S_IXUSR.
 *		If S_IF* is set, require the inode to be of that type.
 */
extern errno_t
inode_permission_check (struct inode *inode, mode_t mode);

struct pathwalk_state {
	struct path root;
	struct path origin;
	int oflag;
	mode_t open_mode;
	int resolve;

	unsigned  int has_parent_component :1;
	unsigned int has_result_component :1;
	struct path parent_component;
	struct path result_component;
};

/**
 * Begin a pathwalk.
 *
 * @state	the pathwalk state.
 * @dirfd	path file descriptor or AT_*
 * @info	a (kernel-space) instance of struct pathwalk_info.
 */
extern errno_t
pathwalk_begin (struct pathwalk_state *state, int dirfd, const struct pathwalk_info *info);

/**
 * Perform a pathwalk.
 *
 * @state	the pathwalk state.
 * @pathname	pathname to lookup.  (kernel-space copy)
 *
 * This returns ESUCCESS for a successful walk.  Callers need to ensure that the
 * final component is present before using it.
 */
extern errno_t
do_pathwalk (struct pathwalk_state *state, const char *pathname);

/**
 * Finish a pathwalk and release pathwalk resources.
 *
 * This must be called, even if do_pathwalk failed with an error.
 */
extern void
pathwalk_finish (struct pathwalk_state *state);

#endif /* _FS_INTERNAL_H */

