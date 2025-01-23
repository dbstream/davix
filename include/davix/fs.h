/**
 * Filesystem support.
 * Copyright (C) 2025-present  dbstream
 *
 *  Some basic filesystem concepts
 *
 * Path:  a path is referred to by its vnode and mountpoint.  The vnode
 * describes the location in the specific filesystem, while the mountpoint
 * describes the overall location in the mountpoint hierarchy.
 *
 * vnode: a vnode is a location in a specific filesystem.  The vnode can be
 * either present or non-present.  This corresponds to if there exists an
 * underlying inode in the filesystem.
 *
 * inode: inodes have a one-to-one correspondence with physical files.
 *
 *  Ownership model
 *
 * All vnodes own, and thus hold a reference to, their corresponding inodes.
 */
#ifndef _DAVIX_FS_H
#define _DAVIX_FS_H 1

#include <davix/uapi/stat.h>
#include <davix/errno.h>
#include <davix/list.h>
#include <davix/mutex.h>
#include <davix/refcount.h>
#include <davix/spinlock.h>

struct vnode;
struct inode;
struct inode_ops;
struct filesystem;
struct filesystem_ops;
struct fs_mount_args;
struct mountpoint;

struct path {
	struct vnode *vnode;
	struct mountpoint *mount;
};

struct filesystem_driver {
	/**
	 * This is the name of this filesystem driver.
	 */
	const char *name;

	/**
	 * Mount the filesystem.
	 *
	 * @fs		storage for 'struct filesystem'
	 * @root	storage for the root vnode
	 * @args	filesystem mount arguments
	 */
	errno_t (*do_mount) (struct filesystem **fs, struct vnode **root,
			struct fs_mount_args *args);
};

struct filesystem {
	refcount_t refcount;

	spinlock_t fs_inodes_lock;
	struct list fs_inodes;

	const struct filesystem_ops *ops;
	const struct filesystem_driver *driver;
};

/**
 * Allocate a 'struct filesystem' structure and initialize common fields.
 */
extern struct filesystem *
new_filesystem (void);

extern void
__fs_put (struct filesystem *fs);

static inline struct filesystem *
fs_get (struct filesystem *fs)
{
	refcount_inc (&fs->refcount);
	return fs;
}

static inline void
fs_put (struct filesystem *fs)
{
	if (refcount_dec (&fs->refcount))
		__fs_put (fs);
}

struct filesystem_ops {
	/**
	 * 'close' the filesystem.  This is called when all references to the
	 * filesystem have been removed and it is about to get unmounted.
	 *
	 * @fs		filesystem
	 */
	void (*close) (struct filesystem *fs);
};

/**
 * This is the structure of a vnode name.
 */
struct vname {
	const char *name_ptr;
	unsigned int name_len;
	char inline_name[28];
};

/**
 * Bits in vn_flags.
 */
#define VN_NEEDLOOKUP 0U	/** The vn->inode field hasn't been looked up.  */
#define VN_UNHOOKED 1U		/** The vnode has been "unhooked" from the parent.  */

struct vnode {
		/* cacheline 0 */
	/**
	 * vnode parent.  NULL if this is the filesystem root.
	 */
	struct vnode *vn_parent;

	/**
	 * Pointer to the filesystem we belong to.
	 */
	struct filesystem *fs;

	/**
	 * Stored inode.  If VN_NEEDLOOKUP, vnode lookup is needed before we can
	 * trust it as empty.
	 */
	struct inode *vn_inode;

	/**
	 * VN_* flags.  Accessed under vn_lock.
	 */
	unsigned int vn_flags;

	/**
	 * vnode spinlock.
	 */
	spinlock_t vn_lock;

	/**
	 * reference count for this vnode.
	 */
	refcount_t refcount;

	/**
	 * vn_children is a linked list of our children.  vn_childnext and
	 * vn_childlink are linkage for this list.
	 *
	 * NOTE: this list does not include nodes that are unhooked.
	 */
	struct vnode *vn_children;
	struct vnode *vn_childnext, **vn_childlink;

		/* cacheline 1 */
	/**
	 * vn_hash_table linkage
	 */
	struct vnode *vn_hash_next, **vn_hash_link;
	size_t vn_hash;

	/**
	 * vnode name.
	 */
	struct vname name;
};

/**
 * Increase the reference count of a vnode by one.
 *
 * @vnode	vnode whose refcount to increase
 */
static inline struct vnode *
vn_get (struct vnode *vnode)
{
	refcount_inc (&vnode->refcount);
	return vnode;
}

/**
 * Decrease the reference count of a vnode by one.
 *
 * @vnode	vnode whose refcount to decrease
 */
extern void
vn_put (struct vnode *vnode);


extern void
__vname_big_get (struct vname *name);

extern void
__vname_big_put (struct vname *name);

/**
 * Copy a vname.
 *
 * @target	copy destination
 * @source	copy source
 */
static inline void
vname_get (struct vname *target, struct vname *source)
{
	*target = *source;
	if (target->name_ptr == source->inline_name)
		target->name_ptr = target->inline_name;
	else
		__vname_big_get (target);
}

/**
 * Put a vname.
 *
 * @name	vname to put
 */
static inline void
vname_put (struct vname *name)
{
	if (name->name_ptr != name->inline_name)
		__vname_big_put (name);
}

/**
 * Move a vname.
 *
 * @target	destination
 * @source	source
 *
 * This makes the old vname unusable.
 */
static inline void
vname_move (struct vname *target, struct vname *source)
{
	*target = *source;
	if (target->name_ptr == source->inline_name)
		target->name_ptr = target->inline_name;
}


/**
 * Install an inode into a vnode.
 *
 * @vnode	vnode to install into
 * @inode	inode to install
 *
 * This is used by e.g. lookup_inode() or mknod().
 *
 * NOTE:  this does not do an iget() of its own on the inode.
 */
static inline void
vnode_install (struct vnode *vnode, struct inode *inode)
{
	spin_lock (&vnode->vn_lock);
	vnode->vn_inode = inode;
	spin_unlock (&vnode->vn_lock);
}

/**
 * Find or allocate a child vnode.
 *
 * @child	storage for the child vnode
 * @parent	parent vnode
 * @name	child name
 * @name_len	length of child name
 *
 * Returns ESUCCESS and the child vnode with an incremented refcount in @child,
 * or an error code on error.
 */
extern errno_t
vn_get_child (struct vnode **child, struct vnode *parent,
		const char *name, size_t name_len);


/**
 * Inode datastructure.
 */
struct inode {
	nlink_t i_nlink;
	mode_t i_mode;
	uid_t i_uid;
	gid_t i_gid;

	/**
	 * inode spinlock.  This protects the above fields used during name
	 * lookup.  TODO: replace this with a generation counting-approach.
	 */
	spinlock_t i_lock;

	refcount_t refcount;

	/**
	 * Things for stat().
	 */
	dev_t i_dev;

	/**
	 * inode mutex.  TODO: replace this with an rwmutex
	 */
	struct mutex i_mutex;

	const struct inode_ops *ops;
	struct filesystem *fs;

	/**
	 * Filesystem inode list.  Protected by fs->fs_inodes_lock.  Not used
	 * for inode lookup.
	 */
	struct list fs_inodes;
};

/**
 * Acquire the i_mutex in exclusive (writer) mode.
 *
 * @inode	inode to lock
 */
static inline errno_t
i_lock_exclusive (struct inode *inode)
{
	return mutex_lock_interruptible (&inode->i_mutex);
}

/**
 * Release the i_mutex from exclusive (writer) mode.
 *
 * @inode	inode to unlock
 */
static inline void
i_unlock_exclusive (struct inode *inode)
{
	mutex_unlock (&inode->i_mutex);
}

/**
 * Acquire the i_mutex in shared (reader) mode.
 *
 * @inode	inode to lock
 */
static inline errno_t
i_lock_shared (struct inode *inode)
{
	return mutex_lock_interruptible (&inode->i_mutex);
}

/**
 * Release the i_mutex from shared (reader) mode.
 */
static inline void
i_unlock_shared (struct inode *inode)
{
	mutex_unlock (&inode->i_mutex);
}

/**
 * Downgrade an exclusive i_mutex hold to a shared i_mutex hold.
 *
 * @inode	inode whose i_mutex to downgrade
 */
static inline void
i_lock_downgrade (struct inode *inode)
{
	(void) inode;
}

struct inode_ops {
	/**
	 * Notify the filesystem that an inode is being freed.
	 *
	 * @inode	inode being freed
	 *
	 * NOTE: actual freeing is done within inode.c.
	 */
	void (*free_inode) (struct inode *inode);

	/**
	 * Lookup an inode.
	 *
	 * @parent	parent inode
	 * @vnode	vnode which is being looked up.
	 */
	errno_t (*lookup_inode) (struct inode *parent, struct vnode *vnode);

	/**
	 * stat() an inode.
	 *
	 * @buf		kernel instance of 'struct stat'
	 * @inode	the inode to stat().
	 */
	errno_t (*stat) (struct stat *buf, struct inode *inode);

	/**
	 * Make a new filesystem node.
	 *
	 * @parent	parent inode
	 * @vnode	path of child inode being created
	 * @uid		uid_t to create the inode with
	 * @gid		gid_t to create the inode with
	 * @mode	file mode to create the inode with
	 * @dev		if S_ISCHR or S_ISBLK, this is the device number to use
	 */
	errno_t (*mknod) (struct inode *parent, struct vnode *vnode,
			uid_t uid, gid_t gid, mode_t mode, dev_t dev);
};

extern struct inode *
alloc_inode (struct filesystem *fs);

extern struct vnode *
alloc_root_vnode (struct filesystem *fs);

/**
 * Free the inode after the final iput().
 */
extern void
__iput (struct inode *inode);

/**
 * Increase the reference count of an inode.
 *
 * @inode	inode whose refcount to increase
 */
static inline struct inode *
iget (struct inode *inode)
{
	refcount_inc (&inode->refcount);
	return inode;
}

/**
 * Decrease the reference count of an inode.
 *
 * @inode	inode whose refcount to decrease
 */
static inline void
iput (struct inode *inode)
{
	if (refcount_dec (&inode->refcount))
		__iput (inode);
}

struct mountpoint {
	struct filesystem *fs;
	struct vnode *root;

	refcount_t refcount;
};

extern void
__mnt_put (struct mountpoint *mnt);

static inline struct mountpoint *
mnt_get (struct mountpoint *mnt)
{
	refcount_inc (&mnt->refcount);
	return mnt;
}

static inline void
mnt_put (struct mountpoint *mnt)
{
	if (refcount_dec (&mnt->refcount))
		__mnt_put (mnt);
}

/**
 * Increase the refcount of a 'struct path'.
 *
 * TODO: use 'struct mountpoint' in 'struct path'.
 */
static inline struct path
path_get (struct path path)
{
	mnt_get (path.mount);
	vn_get (path.vnode);
	return path;
}

/**
 * Decrease the refcount of a 'struct path'.
 *
 * TODO: use 'struct mountpoint' in 'struct path'.
 */
static inline void
path_put (struct path path)
{
	vn_put (path.vnode);
	mnt_put (path.mount);
}

extern void
init_vnode_cache (void);

extern void
init_inode_cache (void);

extern void
init_mounts (void);


/**
 * This is a representation of the arguments to mount().
 */
struct fs_mount_args {
	mode_t		mode;	/** tmpfs: mode of the root entry  */
};

struct fs_context_snapshot {
	struct path cwd;
	struct path root;
};

struct fs_context {
	refcount_t refcount;
	spinlock_t cwd_root_lock;
	struct path cwd;
	struct path root;
	struct mutex fsctx_mutex;
};

static inline struct fs_context_snapshot
fsctx_take_snapshot (struct fs_context *ctx)
{
	spin_lock (&ctx->cwd_root_lock);
	struct fs_context_snapshot snapshot;
	snapshot.cwd = path_get (ctx->cwd),
	snapshot.root = path_get (ctx->root);
	spin_unlock (&ctx->cwd_root_lock);
	return snapshot;
}

static inline void
fsctx_put_snapshot (struct fs_context_snapshot snapshot)
{
	path_put (snapshot.cwd);
	path_put (snapshot.root);
}

extern errno_t
fsctx_create (struct path root);

extern void
__fsctx_put (struct fs_context *ctx);

static inline struct fs_context *
fsctx_get (struct fs_context *ctx)
{
	refcount_inc (&ctx->refcount);
	return ctx;
}

static inline void
fsctx_put (struct fs_context *ctx)
{
	refcount_dec (&ctx->refcount);
}

#endif /* _DAVIX_FS_H */
