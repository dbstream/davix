/**
 * Type definitions for the Davix virtual filesystem.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

struct DEntry;
struct Filesystem;
struct INode;
struct Mount;

struct INodeOps;
struct FilesystemType;

struct File;
struct kstat;

#include <davix/rwmutex.h>
#include <davix/path.h>
#include <davix/refcount.h>
#include <davix/refstr.h>
#include <davix/spinlock.h>
#include <davix/types.h>
#include <dsl/hlist.h>
#include <dsl/list.h>

enum : unsigned int {
	/*
	 * RENAME_NOREPLACE: fail with EEXIST if the rename() operation would
	 * replace another file.
	 */
	RENAME_NOREPLACE		= 1U << 0,
	/*
	 * RENAME_EXCHANGE: atomically exchange two paths.
	 */
	RENAME_EXCHANGE			= 1U << 1,
};

enum : unsigned int {
	/*
	 * D_NEED_LOOKUP: set on a dentry when we don't know whether it is
	 * positive or negative yet.
	 */
	D_NEED_LOOKUP			= 1U << 0,
	/*
	 * D_DETACHED: set on a dentry which is not linked into the dentry
	 * hierarchy, and is thus unreachable except by existing references.
	 */
	D_DETACHED			= 1U << 1,
	/*
	 * D_MOUNTPOINT: set on a dentry when it is possibly a mountpoint.
	 */
	D_MOUNTPOINT			= 1U << 2,
	/*
	 * D_LOOKUP_IN_PROGRESS: set on a dentry when someone is busy looking
	 * it up.
	 */
	D_LOOKUP_IN_PROGRESS		= 1U << 3,
};

enum : unsigned long {
	/*
	 * MNT_DETACHED: set on a mount when it has no parent mount.
	 */
	MNT_DETACHED			= 1UL << 0,
};

/**
 * DName - DEntry name.
 */
struct DName {
	/*
	 * name_ptr: pointer to inline_name or an interned string.
	 */
	const char *name_ptr;
	unsigned int name_len;
	char inline_name[52];
};

/**
 * Mount - the part of a mountpoint structure which is relevant to name lookup.
 */
struct Mount {
	/*
	 * root: The root DEntry of this filesystem mount.
	 */
	DEntry *root;
	/*
	 * mountpoint: Mountpoint location.
	 */
	Path mountpoint;
	/*
	 * mount_hash_list: Mountpoint hashlist.
	 */
	dsl::HListHead mount_hash_list;
	/*
	 * flags: VFS mount flags (VFSMNT_*, _NOT_ MS_*)
	 */
	unsigned int flags;
	/*
	 * Mount spinlock.
	 */
	spinlock_t lock;

	refcount_t refcount;

	/*
	 * Child mountpoints.
	 */
	dsl::ListHead mount_list_linkage;
	dsl::TypedList<Mount, &Mount::mount_list_linkage> child_mounts;
};

typedef dsl::TypedHList<Mount, &Mount::mount_hash_list> MountHashList;

/**
 * DEntry - a directory entry.
 *
 * A DEntry represents a path within a particular filesystem.  DEntries form a
 * tree, where each child DEntry holds a pointer to its parent DEntry.  Every
 * instance of a filesystem has precisely one DEntry tree, which is shared
 * across multiple mounts of the same filesystem.
 *
 * DEntries can be in one of three states:
 * - Positive (DEntry is looked up, non-NULL pointer to an INode)
 * - Negative (DEntry is looked up, no pointer to an INode)
 * - Not looked up (D_NEEDLOOKUP is set in flags).
 */
struct DEntry {
	/*
	 * Parent DEntry, or NULL if this is the filesystem root.
	 */
	DEntry *parent;
	/*
	 * Owning pointer to the filesystem we belong to.
	 */
	Filesystem *fs;
	/*
	 * Onwing pointer to the inode located at this DEntry for a positive
	 * DEntry, or NULL if this DEntry is negative or has not been looked up.
	 */
	INode *inode;
	/*
	 * D_* flags.
	 */
	unsigned int d_flags;
	/*
	 * A spinlock which protects inode and dentry_flags.
	 */
	spinlock_t lock;

	refcount_t refcount;
	/*
	 * DEntry hash map linkage.
	 */
	dsl::HListHead dentry_hash_linkage;
	/*
	 * DEntry name and hash.
	 */
	uintptr_t d_name_hash;
	DName name;
};

static_assert (sizeof(void *) != 8 || sizeof (DEntry) == 128);

typedef dsl::TypedHList<DEntry, &DEntry::dentry_hash_linkage> DEntryHashList;

/**
 * INode - an inode.
 */
struct INode {
	/*
	 * Owning pointer to the filesystem we belong to.
	 */
	Filesystem *fs;
	/*
	 * Filesystem reference count.
	 */
	refcount_t refcount;
	/*
	 * Pointer to INode operations structure.
	 */
	const INodeOps *i_ops;
	/*
	 * INode mutex.
	 */
	RWMutex i_mutex;
	/*
	 * INode metadata, read and written under i_lock.  The reason we put
	 * this in the INode struct is for inode permission checks.
	 */
	uid_t uid;
	gid_t gid;
	mode_t mode;
	/*
	 * INode spinlock.  This protects metadata.
	 */
	spinlock_t i_lock;
};

static inline int
i_lock_shared [[gnu::warn_unused_result]] (INode *inode)
{
	return inode->i_mutex.read_lock_interruptible ();
}

static inline int
i_lock_exclusive [[gnu::warn_unused_result]] (INode *inode)
{
	return inode->i_mutex.write_lock_interruptible ();
}

static inline void
i_unlock_shared (INode *inode)
{
	inode->i_mutex.read_unlock ();
}

static inline void
i_unlock_exclusive (INode *inode)
{
	inode->i_mutex.write_unlock ();
}

/**
 * INodeOps - inode operations structure.
 *
 * NOTE: every function pointer in here which returns an 'int' returns an errno.
 */
struct INodeOps {
	/**
	 * i_lookup - look up a directory entry.
	 * @dir: directory to search
	 * @entry: directory entry to look up
	 * Returns an errno, or zero for success.  On success, writes the
	 * resulting inode into @entry.  ENOENT is considered a successful
	 * lookup, but it does not write anything into @entry.
	 *
	 * The inode mutex on @dir is held for reading.
	 */
	int (*i_lookup) (INode *dir, DEntry *entry);

	/**
	 * i_close - close an inode.
	 * @inode: inode that we are closing
	 */
	int (*i_close) (INode *inode);

	/**
	 * i_unlink - unlink a directory entry.
	 * @dir: parent directory
	 * @entry: directory entry to remove
	 * Returns an errno, or zero for success.
	 *
	 * If @entry is itself a directory, i_unlink is responsible for
	 * verifying that the directory is empty.
	 *
	 * The inode mutex on @dir is held for writing.
	 */
	int (*i_unlink) (INode *dir, DEntry *entry);

	/**
	 * i_mknod - create a special filesystem node.
	 * @dir: parent directory
	 * @entry: directory entry to create the node at
	 * @mode: file mode and type
	 * @device: device identifier
	 * Returns an errno, or zero for success.
	 *
	 * The inode mutex on @dir is held for writing.
	 */
	int (*i_mknod) (INode *dir, DEntry *entry, mode_t mode, dev_t device);

	/**
	 * i_mkdir - create a directory.
	 * @dir: parent directory
	 * @entry: directory entry to create the directory at
	 * @mode: the mode with which to create the directory
	 * Returns an errno, or zero for success.
	 *
	 * The inode mutex on @dir is held for writing.
	 */
	int (*i_mkdir) (INode *dir, DEntry *entry, mode_t mode);

	/**
	 * i_symlink - create a symbolic link.
	 * @dir: parent directory
	 * @entry: directory entry to create the symlink at
	 * @mode: the mode with which to create the symlink
	 * @path: symbolic link target
	 * Returns an errno, or zero for success.
	 *
	 * The inode mutex on @dir is held for writing.
	 */
	int (*i_symlink) (INode *dir, DEntry *entry, mode_t mode,
			const char *path);

	/**
	 * i_link - give an inode a name.
	 * @dir: parent directory
	 * @entry: directory entry to hardlink the inode at
	 * Returns an errno, or zero for success.
	 * @inode: the inode to hardlink
	 * Returns an errno, or zero for success.
	 *
	 * The inode mutex on @dir is held for writing.
	 */
	int (*i_link) (INode *dir, DEntry *entry, INode *inode);

	/**
	 * i_rename - move a directory entry.
	 * @fromdir: parent directory of origin
	 * @from: origin directory entry
	 * @todir: parent directory of target
	 * @to: target directory entry
	 * @flags: RENAME_* flags
	 * Returns an errno, or zero for success.
	 *
	 * The inode mutex on both @fromdir and @todir is held for writing.
	 */
	int (*i_rename) (INode *fromdir, DEntry *from, INode *todir, DEntry *to,
			unsigned int flags);

	/**
	 * i_chmod - change permissions of an inode.
	 * @inode: inode to change permissions for
	 * @mode: new inode permissions
	 * Returns an errno, or zero for success.
	 *
	 * The inode mutex on @inode is held for writing.
	 */
	int (*i_chmod) (INode *inode, mode_t mode);

	/**
	 * i_chown - change inode owner.
	 * @inode: inode to change the owner of
	 * @uid: new owner id
	 * @gid: new group id
	 * Returns an errno, or zero for success.
	 *
	 * The inode mutex on @inode is held for writing.
	 */
	int (*i_chown) (INode *inode, uid_t uid, gid_t gid);

	/**
	 * i_stat - retrieve inode metadata.
	 * @inode: inode
	 * @kstat: buffer to return inode metadata in
	 * Returns an errno, or zero for success.
	 *
	 * The inode mutex on @inode is not held but may be taken for reading.
	 */
	int (*i_stat) (INode *inode, struct kstat *kstat);

	/**
	 * i_readlink - read symlink contents.
	 * @inode: inode
	 * @str: pointer to RefStr pointer to return the path into.
	 * Returns an errno, or zero for success.  On error, @str isn't written
	 * to.
	 *
	 * The inode mutex on @inode is not held but may be taken for reading.
	 */
	int (*i_readlink) (INode *inode, RefStr **str);

	/**
	 * i_tmpfile - create a tmpfile.
	 * @dir: parent directory inode
	 * @inode: pointer to INode pointer where the new inode is returned.
	 * Returns an errno, or zero for success.  On error, @inode isn't
	 * written to.
	 *
	 * The inode mutex on @dir is not held but may be taken for reading or
	 * writing.
	 */
	int (*i_tmpfile) (INode *dir, INode **inode);

	/**
	 * i_open - open a file.
	 * @inode: inode being opened
	 * @file: pointer to struct File
	 * Returns an errno, or zero if the file open operation may continue.
	 *
	 * The inode mutex on @inode is not held.
	 *
	 * i_open gives the filesystem a place to install filesystem-specific
	 * data into the File struct.
	 */
	int (*i_open) (INode *inode, File *file);

	/**
	 * i_truncate - truncate an inode.
	 * @inode: inode being truncated
	 * @length: new byte length
	 * Returns an errno, or zero indicating success.
	 *
	 * The inode mutex on @inode is held for writing.
	 */
	int (*i_truncate) (INode *inode, off_t length);
};

/**
 * Filesystem - a physical filesystem mount.
 */
struct Filesystem {
	const FilesystemType *ops;
	refcount_t refcount;
	/*
	 * Per-filesystem MNT_* flags.
	 */
	unsigned long fs_flags;
};

/**
 * FilesystemType - represents a filesystem driver.
 */
struct FilesystemType {
	/*
	 * name: Human-readable filesystem type name
	 */
	char name[32];

	/**
	 * mount_fs - mount a filesystem.
	 * @source: source string
	 * @mount_flags: MOUNT_* flags
	 * @data: filesystem-specific data
	 * Returns an errno, or zero indicating success.
	 */
	int (*mount_fs) (const char *source, unsigned long mount_flags,
			const void *data, Filesystem **fs);

	/**
	 * unmount_fs - unmount a filesystem.
	 * @fs: filesystem
	 *
	 * This is called as a result of the filesystem's reference count
	 * reaching zero.
	 */
	void (*unmount_fs) (Filesystem *fs);
};

