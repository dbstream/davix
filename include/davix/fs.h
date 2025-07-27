/**
 * Filesystem API.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

struct DEntry;
struct Filesystem;
struct INode;
struct Mount;

struct FilesystemType;

struct File;

#include <davix/path.h>
#include <davix/types.h>
#include <stddef.h>

/*
 * Reference counting operations:
 */

DEntry *dget (DEntry *);
void dput (DEntry *);
INode *iget (INode *);
INode *iget_maybe_zero (INode *);
void iput (INode *);
Mount *mnt_get (Mount *);
void mnt_put (Mount *);
Filesystem *fs_get (Filesystem *);
void fs_put (Filesystem *);

/*
 * File system initialization:
 */

void
init_fs_caches (void);

DEntry *
allocate_root_dentry (Filesystem *fs);

/*
 * DEntry operations:
 */

bool
d_cond_unlink (DEntry *dentry);

DEntry *
d_lookup (DEntry *parent, const char *name, size_t name_len);

int
d_ensure_inode (DEntry *dentry);

void
d_set_inode (DEntry *dentry, INode *inode);

void
d_set_inode_nocache (DEntry *dentry, INode *inode);

void
d_set_nocache (DEntry *dentry);

void
d_unlink (DEntry *dentry);

void
d_rename (DEntry *from, DEntry *to, unsigned int rename_flags);

void
d_set_overmounted (DEntry *dentry);

void
d_clear_overmounted (DEntry *dentry);

void
d_trim_lru_full (void);

void
d_trim_lru_partial (void);

void *
d_path_seqbegin (void);

bool
d_path_seqretry (void **);

INode *
d_inode (DEntry *dentry);

bool
d_detached (DEntry *dentry);

DEntry *
dget_parent (DEntry *de);

/*
 * Mount table operations:
 */

Mount *
mnt_get_at (Mount *mnt, DEntry *de);

void *
mnt_path_seqbegin (void);

bool
mnt_path_seqretry (void **);

/*
 * Filesystem operations:
 */

void
register_filesystem (FilesystemType *fstype);

FilesystemType *
get_filesystem_type (const char *name);

Filesystem *
new_filesystem (FilesystemType *type, void *fs_private,
		unsigned long mount_flags);

/*
 * INode operations:
 */

INode *
new_inode (Filesystem *fs, void *i_private);

void
i_set_nlink (INode *inode, nlink_t count);

void
i_incr_nlink (INode *inode, nlink_t count);

void
i_decr_nlink (INode *inode, nlink_t count);

Mount *
do_mount_root (const char *fstype, const char *source,
		unsigned long mount_flags, const void *data);

enum : unsigned int {
	/*
	 * LOOKUP_NO_ENOENT: do not fail with ENOENT if the final component of
	 * the path does not exist.
	 */
	LOOKUP_NO_ENOENT		= 1U << 0,
	/*
	 * LOOKUP_PARENT_DIR: perform parent directory lookup.
	 */
	LOOKUP_PARENT_DIR		= 1U << 1,
	/*
	 * LOOKUP_NOFOLLOW: if the final component of the path is a symbolic
	 * link, do not follow it.
	 */
	LOOKUP_NOFOLLOW			= 1U << 2,
	/*
	 * LOOKUP_BELOW: do not permit walking parent links.
	 */
	LOOKUP_BELOW			= 1U << 3,
	/*
	 * LOOKUP_IN_ROOT: pretend that dirfd is the root directory when walking
	 * this path.
	 */
	LOOKUP_IN_ROOT			= 1U << 4,
	/*
	 * LOOKUP_NO_SYMLINKS: fail if a symbolic link is encountered during
	 * path resolution.
	 */
	LOOKUP_NO_SYMLINKS		= 1U << 5,
	/*
	 * LOOKUP_NO_XDEV: fail if path resolution attempts to traverse across a
	 * mount point.
	 */
	LOOKUP_NO_XDEV			= 1U << 6,
	/*
	 * LOOKUP_NO_IO: fail if any kind of IO needs to be performed during
	 * path resolution.  If lookup fails because IO was needed, do_get_path
	 * returns EAGAIN.
	 */
	LOOKUP_NO_IO			= 1U << 7,
};

int
do_get_path (int dirfd, const char *filename, unsigned int lookup_flags,
		Path *out_path, DEntry **out_parent);

int
vfs_path (int dirfd, const char *filename, unsigned int flags,
		Path *out_path, DEntry **out_parent);

bool
i_may_read (INode *inode);

bool
i_may_write (INode *inode);

bool
i_may_exec (INode *inode);

bool
i_may_search_dir (INode *inode);

void
i_set_rmdired (INode *inode);

bool
i_was_rmdired (INode *inode);

/*
 * VFS functions:
 */

int
ksys_rename (int olddirfd, const char *oldpath,
		int newdirfd, const char *newpath,
		unsigned int rename_flags);

int
ksys_unlink (int dirfd, const char *filename);

int
ksys_rmdir (int dirfd, const char *filename);

int
ksys_mkdir (int dirfd, const char *filename, mode_t mode);

int
ksys_mknod (int dirfd, const char *filename, mode_t mode, dev_t dev);

