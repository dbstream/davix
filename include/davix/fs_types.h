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
#include <davix/rcu.h>
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
	/*
	 * D_ON_LRU: set on a dentry when it is on the LRU.
	 */
	D_ON_LRU			= 1U << 4,
	/*
	 * D_WAS_REFERENCED: set on a dentry by dput before dropping the
	 * reference count.  Used by LRU.
	 */
	D_WAS_REFERENCED		= 1U << 5,
	/*
	 * D_FREED: set on a dentry when it has been freed.
	 */
	D_FREED				= 1U << 6,
	/*
	 * D_DONT_KEEP: set on a dentry when it should be freed immediately
	 * instead of being placed on the LRU.
	 */
	D_DONT_KEEP			= 1U << 7,
};

enum : unsigned long {
	/*
	 * VFSMNT_ORPHAN: set on a mount when it has no parent mount.
	 */
	VFSMNT_ORPHAN			= 1UL << 0,
	/*
	 * VFSMNT_DETACHED: set on a mount when it does not exist in the mount
	 * table.
	 */
	VFSMNT_DETACHED			= 1UL << 1,
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
	char inline_name[20];
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
	 * fs: The filesystem that this mount belongs to.
	 */
	Filesystem *fs;
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
	union {
		/*
		 * DEntry hash map linkage.
		 */
		dsl::HListHead dentry_hash_linkage;
		/*
		 * RCU head for freeing.
		 */
		RCUHead rcu;
	};
	/*
	 * DEntry hash, a function of the parent DEntry pointer and our name.
	 */
	uintptr_t d_hash;
	/*
	 * DEntry name.
	 */
	DName name;
	/*
	 * LRU linkage.
	 */
	dsl::ListHead dentry_lru_head;
	dsl::ListHead dentry_fs_list;
};

static_assert (sizeof(void *) != 8 || sizeof (DEntry) == 128);

typedef dsl::TypedHList<DEntry, &DEntry::dentry_hash_linkage> DEntryHashList;
typedef dsl::TypedList<DEntry, &DEntry::dentry_lru_head> DEntryLRU;
typedef dsl::TypedList<DEntry, &DEntry::dentry_fs_list> DEntryList;

static inline void
d_lock (DEntry *de)
{
	de->lock.lock_dpc ();
}

static inline void
d_unlock (DEntry *de)
{
	de->lock.unlock_dpc ();
}

static inline bool
d_trylock (DEntry *de)
{
	disable_dpc ();
	if (de->lock.raw_trylock ())
		return true;
	enable_dpc ();
	return false;
}

static inline INode *
d_inode (DEntry *de)
{
	return de->inode;
}

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
	/*
	 * INode metadata needed by stat().
	 */
	nlink_t nlink;
	dev_t rdev;
	ino_t ino;
	off_t size;
	/*
	 * Filesystem private data.
	 */
	void *i_private;
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
	 * Returns true if the inode was successfully dropped from the inode
	 * cache.  If this returns false, iput should be retried.
	 */
	bool (*i_close) (INode *inode);

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
	 * @uid: inode owner
	 * @gid: inode group
	 * @mode: file mode and type
	 * @device: device identifier
	 * Returns an errno, or zero for success.
	 *
	 * The inode mutex on @dir is held for writing.
	 */
	int (*i_mknod) (INode *dir, DEntry *entry,
			uid_t uid, gid_t gid, mode_t mode, dev_t device);

	/**
	 * i_mkdir - create a directory.
	 * @dir: parent directory
	 * @entry: directory entry to create the directory at
	 * @uid: inode owner
	 * @gid: inode group
	 * @mode: the mode with which to create the directory
	 * Returns an errno, or zero for success.
	 *
	 * The inode mutex on @dir is held for writing.
	 */
	int (*i_mkdir) (INode *dir, DEntry *entry,
			uid_t uid, gid_t gid, mode_t mode);

	/**
	 * i_symlink - create a symbolic link.
	 * @dir: parent directory
	 * @entry: directory entry to create the symlink at
	 * @uid: inode owner
	 * @gid: inode group
	 * @mode: the mode with which to create the symlink
	 * @path: symbolic link target
	 * Returns an errno, or zero for success.
	 *
	 * The inode mutex on @dir is held for writing.
	 */
	int (*i_symlink) (INode *dir, DEntry *entry,
			uid_t uid, gid_t gid, mode_t mode,
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
	 * @uid: inode owner
	 * @gid: inode group
	 * @mode: inode mode
	 * Returns an errno, or zero for success.  On error, @inode isn't
	 * written to.
	 *
	 * The inode mutex on @dir is not held but may be taken for reading or
	 * writing.
	 */
	int (*i_tmpfile) (INode *dir, INode **inode,
			uid_t uid, gid_t gid, mode_t mode);

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
	/*
	 * Number of mounts for this filesystem.
	 */
	unsigned long num_mounts;
	/*
	 * Filesystem private data.
	 */
	union {
		void *ptr;
		ino_t ino;
	} fs_private;
	/*
	 * Filesystem DEntry list.
	 */
	DEntryList fs_dentries;
	spinlock_t dentry_list_lock;
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
	 * @fstype: pointer to the FilesystemType
	 * @data: filesystem-specific data
	 * Returns an errno, or zero indicating success.
	 */
	int (*mount_fs) (const char *source, unsigned long mount_flags,
			FilesystemType *fstype, const void *data,
			Filesystem **fs, DEntry **root);

	/**
	 * unmount_fs - unmount a filesystem.
	 * @fs: filesystem
	 *
	 * This is called as a result of the filesystem's reference count
	 * reaching zero.
	 */
	void (*unmount_fs) (Filesystem *fs);

	/**
	 * trim_fs - hook for when num_mounts of a filesystem reaches zero.
	 * @fs: filesystem
	 *
	 * This is needed by tmpfs, which biases DEntry reference counts so that
	 * they would never reach zero unless this is invoked.
	 */
	void (*trim_fs) (Filesystem *fs);

	dsl::ListHead fs_type_list;
};

