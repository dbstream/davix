/**
 * Path lookup.
 * Copyright (C) 2025-present  dbstream
 */
#include <davix/fs_types.h>
#include <davix/fs.h>
#include <davix/kmalloc.h>
#include <davix/mutex.h>
#include <davix/path.h>
#include <davix/printk.h>
#include <davix/panic.h>
#include <davix/task.h>
#include <uapi/davix/errno.h>
#include <uapi/davix/fcntl.h>
#include <uapi/davix/stat.h>
#include <string.h>

FSContext init_fs_context = {
	.refcount = 1,
	.fs_uid = 0,
	.fs_gid = 0,
	.umask = 0022,
};

Path
path_get (Path path)
{
	Path ret;
	ret.mount = mnt_get (path.mount);
	ret.dentry = dget (path.dentry);
	return ret;
}

void
path_put (Path path)
{
	dput (path.dentry);
	mnt_put (path.mount);
}

FSContext *
fsctx_get (FSContext *fsctx)
{
	refcount_inc (&fsctx->refcount);
	return fsctx;
}

void
fsctx_put (FSContext *fsctx)
{
	if (!fsctx)
		return;

	if (refcount_dec (&fsctx->refcount)) {
		path_put (fsctx->cwd);
		path_put (fsctx->root);
		kfree (fsctx);
	}
}

static void
read_uid_gid_umask (uid_t *uid, gid_t *gid, mode_t *umask)
{
	FSContext *ctx = get_current_task ()->ctx_fs;
	if (!ctx) {
		*uid = 0;
		*gid = 0;
		*umask = 0022;
		return;
	}
	ctx->lock.lock_dpc ();
	*uid = ctx->fs_uid;
	*gid = ctx->fs_gid;
	*umask = ctx->umask;
	ctx->lock.unlock_dpc ();
}

static constexpr int MAX_NESTED_SYMLINKS = 10;
static constexpr int MAX_TOTAL_SYMLINKS = 40;

class PathwalkComponents {
public:
	RefStr *symlink_contents[MAX_NESTED_SYMLINKS];
	const char *component_ptrs[MAX_NESTED_SYMLINKS + 1];
	bool is_final[MAX_NESTED_SYMLINKS + 1];
	int level = 0;
	int counter = 0;

	~PathwalkComponents (void);

	PathwalkComponents (const char *filename)
	{
		component_ptrs[0] = filename;
	}

	bool
	push_symlink (RefStr *str);

	bool
	leading_slash (void);

	bool
	next (const char **pstring, size_t *plength, bool *pfinal);
};

PathwalkComponents::~PathwalkComponents (void)
{
	while (level)
		put_refstr (symlink_contents[--level]);
}

/**
 * PathwalkComponets::push_symlink - push symbolic link contents to the stack.
 * @str: symbolic link contents
 * Returns true if the operation succeeded.  If this returns false, do_get_path
 * should fail with ELOOP.
 */
bool
PathwalkComponents::push_symlink (RefStr *str)
{
	if (counter >= MAX_TOTAL_SYMLINKS)
		return false;
	if (level >= MAX_NESTED_SYMLINKS)
		return false;
	counter++;
	symlink_contents[level++] = str;
	component_ptrs[level] = str->string;
	return true;
}

/**
 * PathwalkComponents::leading_slash - skip the leading slash.
 * Returns true if there was a leading slash.
 */
bool
PathwalkComponents::leading_slash (void)
{
	const char *s = component_ptrs[level];
	if (*s != '/')
		return false;
	do
		s++;
	while (*s == '/');
	component_ptrs[level] = s;
	return true;
}

/**
 * PathwalkComponents::next - get the next component.
 * @pstring: pointer to string pointer where the component will be stored.
 * @plength: pointer to variable where the component length will be stored.
 * @pmaybe_final: pointer to boolean where true will be stored if this is
 * possibly the final component.
 * Returns false if there is no next component.
 */
bool
PathwalkComponents::next (const char **pstring, size_t *plength,
		bool *pmaybe_final)
{
	for (;;) {
		const char *s = component_ptrs[level];
		const char *end = strchrnul (s, '/');
		if (end == s) {
			if (!level)
				return false;
			put_refstr(symlink_contents[--level]);
			continue;
		}

		*plength = end - s;
		*pstring = s;

		s = end;
		while (*s == '/')
			s++;
		component_ptrs[level] = s;

		if (!*s && (!level || is_final[level - 1])) {
			is_final[level] = true;
			*pmaybe_final = true;
		} else {
			is_final[level] = false;
			*pmaybe_final = false;
		}

		return true;
	}
}

class LookupState {
public:
	PathwalkComponents components;
	Path curpath = { nullptr, nullptr };
	Path root;

	LookupState(const char *path, Path root_)
		: components(path)
	{
		root = root_;
		curpath = path_get(root_);
	}

	~LookupState (void);

	void
	set_curpath (Path path);

	void
	set_dentry (DEntry *dentry);
};

LookupState::~LookupState (void)
{
	path_put (curpath);
	path_put (root);
}

void
LookupState::set_curpath (Path path)
{
	path_put (curpath);
	curpath = path;
}

void
LookupState::set_dentry(DEntry *dentry)
{
	dput (curpath.dentry);
	curpath.dentry = dentry;
}

static int
step_out_of_mount (LookupState &state, unsigned int lookup_flags)
{
	Mount *mnt = mnt_get (state.curpath.mount);
	DEntry *de = dget (state.curpath.dentry);

	while (de == mnt->root) {
		if (mnt == state.root.mount && de == state.root.dentry)
			/*
			 * Don't step out of root.
			 */
			break;
		mnt->lock.lock_dpc ();
		if (mnt->flags & VFSMNT_ORPHAN) {
			mnt->lock.unlock_dpc ();
			break;
		}

		if (lookup_flags & LOOKUP_NO_XDEV) {
			mnt->lock.unlock_dpc ();
			dput (de);
			mnt_put (mnt);
			return EXDEV;
		}

		Mount *pmnt = mnt_get (mnt->mountpoint.mount);
		DEntry *pde = dget (mnt->mountpoint.dentry);
		mnt->lock.unlock_dpc ();

		dput (de);
		mnt_put (mnt);
		de = pde;
		mnt = pmnt;
	}

	dput (state.curpath.dentry);
	mnt_put (state.curpath.mount);
	state.curpath.mount = mnt;
	state.curpath.dentry = de;
	return 0;
}

static int
step_into_mount (Path *path, unsigned int lookup_flags)
{
	Mount *mnt = mnt_get (path->mount);
	DEntry *de = dget (path->dentry);

	for (;;) {
		/*
		 * NB: it is okay to use relaxed here.  If the mount hierarchy
		 * changes, the mount seqcount will update, which will be caught
		 * by the caller of do_get_path, and pathwalk will be retried.
		 */
		if (!(atomic_load_relaxed (&de->d_flags) & D_MOUNTPOINT))
			break;

		Mount *childmnt = mnt_get_at (mnt, de);
		if (!childmnt)
			break;

		if (lookup_flags & LOOKUP_NO_XDEV) {
			mnt_put (childmnt);
			mnt_put (mnt);
			dput (de);
			return EXDEV;
		}

		DEntry *inner_root = dget (childmnt->root);
		dput (de);
		de = inner_root;
		mnt_put (mnt);
		mnt = childmnt;
	}

	mnt_put (path->mount);
	path->mount = mnt;
	dput (path->dentry);
	path->dentry = de;
	return 0;
}

/**
 * do_get_path - do a name lookup.
 * @dirfd: directory file descriptor of origin or special value AT_FDCWD.
 * @filename: the name to look up
 * @lookup_flags: LOOKUP_* flags
 * @out_path: pointer to storage for the result path
 * @out_parent: pointer to storage for the resulting parent DEntry
 * Returns an errno on failure or zero indicating success.
 *
 * FIXME: currently we don't deal with a trailing slash at all.
 */
int
do_get_path (int dirfd, const char *filename, unsigned int lookup_flags,
		Path *out_path, DEntry **out_parent)
{
	/*
	 * Validate lookup_flags.
	 */

	bool want_parent_dir = (lookup_flags & LOOKUP_PARENT_DIR) ? 1 : 0;
	bool want_nofollow = (lookup_flags & LOOKUP_NOFOLLOW) ? 1 : 0;
	bool want_no_links = (lookup_flags & LOOKUP_NO_SYMLINKS) ? 1 : 0;
	bool want_below = (lookup_flags & LOOKUP_BELOW) ? 1 : 0;
	bool want_enoent = (lookup_flags & LOOKUP_NO_ENOENT) ? 1 : 0;
	bool want_noxdev = (lookup_flags & LOOKUP_NO_XDEV) ? 1 : 0;

	if (want_parent_dir) {
		if (!want_nofollow)
			/*
			 * If LOOKUP_PARENT_DIR is set, LOOKUP_NOFOLLOW must
			 * also be set.
			 */
			return EINVAL;
	}

	if (lookup_flags & LOOKUP_NO_IO)
		/*
		 * We don't support NO_IO lookups yet.
		 */
		return EOPNOTSUPP;

	if (dirfd != AT_FDCWD)
		/*
		 * We don't support beginning pathwalk at locations specified by
		 * a file descriptor yet.
		 */
		return EOPNOTSUPP;

	if (!*filename)
		/*
		 * In old Unixes, an empty filename was taken to mean 'current
		 * directory'.  However POSIX specifies that this is invalid,
		 * and Linux returns ENOENT in that case.  Clone the Linux
		 * behavior and return ENOENT if the filename is empty.
		 */
		return ENOENT;

	FSContext *ctx = get_current_task()->ctx_fs;
	if (!ctx)
		return EINVAL;
	ctx->lock.lock_dpc ();
	LookupState state (filename, ctx->root);
	state.set_curpath (path_get (ctx->cwd));
	ctx->lock.unlock_dpc ();

	if (want_below) {
		/*
		 * For LOOKUP_BELOW we need someplace to keep track of where
		 * lookup began.  LookupState::root is convenient for this, and
		 * it doesn't impact lookups on '/' because those are forbidden
		 * using LOOKUP_BELOW.
		 */
		path_put (state.root);
		state.root = path_get (state.curpath);
	} else if (*filename == '/') {
		/*
		 * If the path begins with a '/', we should go to root even if
		 * LOOKUP_NO_XDEV is set.  However, if we later discover a
		 * symbolic link with a leading '/', we do not permit walking it
		 * when LOOKUP_NO_XDEV is set in lookup_flags.
		 */
		path_put (state.curpath);
		state.curpath = path_get (state.root);
	}

	for (;;) {
		/*
		 * First, handle leading slashes.
		 *
		 * Symbolic links can 'insert' additional leading slashes in the
		 * middle of a pathwalk, hence why we do this check here and not
		 * outside the loop.
		 */
		if (state.components.leading_slash()) {
			if (want_below)
				/*
				 * LOOKUP_BELOW doesn't permit walking absolute
				 * paths.
				 */
				return EINVAL;
			if (want_noxdev) {
				/*
				 * If LOOKUP_NO_XDEV is set, we cannot go to
				 * root if root is on another device.
				 */
				if (state.root.mount != state.curpath.mount)
					return EXDEV;
			}
			state.set_curpath(path_get(state.root));
		}

		const char *name;
		size_t len;
		bool maybe_final;
		if (!state.components.next(&name, &len, &maybe_final)) {
			/*
			 * It is not obvious how we can get here, but most clear
			 * is probably an empty symlink or a symlink with only a
			 * leading slash.
			 */
			if (want_parent_dir)
				return EINVAL;
			*out_path = path_get (state.curpath);
			return 0;
		}

		/*
		 * Handle any '..' components.  We permit walking to the parent
		 * without checking for permission first.
		 */
		if (len == 2 && !memcmp(name, "..", 2)) {
			if (want_below) {
				/*
				 * LOOKUP_BELOW doesn't permit walking past the
				 * origin of lookup.
				 */
				if (path_equals (state.curpath, state.root))
					return EINVAL;
			}

			/*
			 * Step out of any mountpoint we're in.
			 */
			int ret = step_out_of_mount (state, lookup_flags);
			if (ret)
				return ret;
			/*
			 * Walk to the parent...
			 */
			DEntry *parent = dget_parent (state.curpath.dentry);
			if (parent)
				state.set_dentry (parent);
			/*
			 * ... and continue on with the next component.
			 */
			continue;
		}

		/*
		 * Perform filesystem permission checks before walking '.' or a
		 * child link.
		 */
		if (!i_may_search_dir (d_inode (state.curpath.dentry)))
			return EACCES;

		/*
		 * Handle any '.' components.
		 */
		if (len == 1 && !memcmp(name, ".", 1))
			continue;

		/*
		 * Okay, we are performing an actual child lookup here...
		 */
		DEntry *childde = d_lookup (state.curpath.dentry, name, len);
		if (!childde)
			return ENOMEM;

		Path child = { mnt_get (state.curpath.mount), childde };
		int ret = step_into_mount (&child, lookup_flags);
		if (ret) {
			path_put (child);
			return ret;
		}

		childde = child.dentry;

		/*
		 * TODO: remove this debugging check when we deem the VFS good.
		 */
		d_lock (childde);
		if (childde->d_flags & D_FREED)
			panic("Bah! do_get_path(%s) encountered a freed DEntry.", filename);
		d_unlock (childde);

		/*
		 * We need to ensure the DEntry is not stale.
		 */
		ret = d_ensure_inode (childde);
		if (ret) {
			path_put (child);
			return ret;
		}

		INode *inode = d_inode (childde);
		/*
		 * Handle the ENOENT case:
		 */
		if (!inode) {
			if (!maybe_final) {
				/*
				 * If this is not the final component, we should
				 * always return ENOENT.
				 */
				path_put (child);
				return ENOENT;
			}
			if (!want_enoent) {
				/*
				 * If the caller doesn't handle negative
				 * DEntries, fail with ENOENT.
				 */
				path_put (child);
				return ENOENT;
			}
			/*
			 * Ok, return this as the result.
			 */
			if (want_parent_dir) {
				/*
				 * LOOKUP_PARENT_DIR requires the two paths to
				 * lie within the same mounts.
				 */
				if (state.curpath.mount != child.mount) {
					path_put (child);
					return EXDEV;
				}
				/*
				 * LOOKUP_PARENT_DIR requires LOOKUP_NOFOLLOW to
				 * be set;  therefore, we need not check if this
				 * lookup came from a symlink.
				 */
				*out_parent = dget (state.curpath.dentry);
			}
			*out_path = child;
			return 0;
		}

		/*
		 * ok, rest of code:
		 * - handle the symlink case (differs depending on maybe_final,
		 * LOOKUP_NO_SYMLINKS and LOOKUP_NOFOLLOW).
		 * - handle the other non-directory case.
		 * - go to a child directory (simple).
		 */
		inode->i_lock.lock_dpc ();
		mode_t mode = inode->mode;
		inode->i_lock.unlock_dpc ();

		/*
		 * Handle the symbolic link case.
		 */
		if (__S_ISLNK(mode) && !(maybe_final && want_nofollow)) {
			if (want_no_links) {
				/*
				 * LOOKUP_NO_SYMLINKS forbids walking symbolic
				 * links.
				 */
				path_put (child);
				return ELOOP;
			}

			if (!inode->i_ops->i_readlink) {
				path_put (child);
				return EOPNOTSUPP;
			}

			RefStr *contents;
			ret = inode->i_ops->i_readlink (inode, &contents);
			path_put (child);
			if (ret)
				return ret;
			if (!state.components.push_symlink (contents)) {
				put_refstr (contents);
				return ELOOP;
			}
			/*
			 * We've pushed the symlink to our component stack;
			 * continue walking from the directory we were in.
			 */
			continue;
		}

		if (!maybe_final) {
			if (!__S_ISDIR(mode)) {
				/*
				 * We cannot step into a non-directory.
				 */
				path_put (child);
				return ENOTDIR;
			}
			/*
			 * Enter the directory.
			 */
			state.set_curpath (child);
			continue;
		}

		if (want_parent_dir) {
			if (state.components.level != 0) {
				/*
				 * The parent directory is invalid if the 'real'
				 * final component is a symbolic link which we
				 * have walked along.
				 */
				path_put (child);
				return EINVAL;
			}
			if (state.curpath.mount != child.mount) {
				/*
				 * We cannot return a parent directory in
				 * another mount.
				 */
				path_put (child);
				return EXDEV;
			}

			*out_parent = dget (state.curpath.dentry);
		}
		*out_path = child;
		return 0;
	}
}

struct lookup_cookies {
	void *dcache_cookie;
	void *mount_cookie;
};

static lookup_cookies
lookup_seqbegin (void)
{
	return { d_path_seqbegin (), mnt_path_seqbegin () };
}

static bool
lookup_seqretry (lookup_cookies &jar)
{
	bool dcache_retry = d_path_seqretry (&jar.dcache_cookie);
	bool mount_retry = mnt_path_seqretry (&jar.mount_cookie);

	return dcache_retry || mount_retry;
}

/**
 * vfs_path - perform a path lookup.
 * @dirfd: directory file descriptor
 * @filename: filename to lookup
 * @flags: LOOKUP_* flags
 * @out_path: storage for a resulting struct Path
 * @out_parent: storage for the resulting parent DEntry
 * Returns an errno or zero to indicate success.
 *
 * NOTE: @out_parent is only written to if LOOKUP_PARENT_DIR is set in the
 * lookup flags.
 */
int
vfs_path (int dirfd, const char *filename, unsigned int flags,
		Path *out_path, DEntry **out_parent)
{
	if (flags & LOOKUP_PARENT_DIR) {
		if (!out_parent)
			return EINVAL;
	} else {
		if (out_parent)
			return EINVAL;
	}

	lookup_cookies jar = lookup_seqbegin ();
again:
	int ret = do_get_path (dirfd, filename, flags, out_path, out_parent);
	if (ret)
		return ret;

	if (lookup_seqretry (jar)) {
		if (flags & LOOKUP_PARENT_DIR)
			dput (*out_parent);
		path_put (*out_path);
		goto again;
	}

	return 0;
}

static bool
is_ancestor (DEntry *ancestor, DEntry *dentry)
{
	do {
		d_lock (dentry);
		DEntry *parent = dentry->parent;
		d_unlock (dentry);
		dentry = parent;
		if (!dentry)
			return false;
	} while (dentry != ancestor);
	return true;
}

static Mutex rename_mutex;

/**
 * ksys_rename - perform a rename operation.
 * @olddirfd: dirfd of path being moved from
 * @oldpath: path being moved from
 * @newdirfd: dirfd of path being moved to
 * @newpath: path being moved to
 * @flags: RENAME_* flags
 * Returns an errno indicating success or failure reason.
 */
int
ksys_rename (int olddirfd, const char *oldpath,
		int newdirfd, const char *newpath,
		unsigned int flags)
{
	DEntry *from_parent, *to_parent;
	Path from, to;
	lookup_cookies jar = lookup_seqbegin ();
again:
	/*
	 * First lookup the path being moved from.
	 */
	int ret = do_get_path (olddirfd, oldpath,
			LOOKUP_PARENT_DIR | LOOKUP_NOFOLLOW, &from, &from_parent);
	if (ret != 0)
		return ret;
	/*
	 * Next lookup the path being moved to.
	 */
	ret = do_get_path (newdirfd, newpath, LOOKUP_NOFOLLOW | LOOKUP_NO_ENOENT,
			&to, &to_parent);
	if (ret != 0) {
		dput (from_parent);
		path_put (from);
		return ret;
	}

	/*
	 * Validate that the two DEntries belong to the same file system.
	 * NB: this check fails even if the two DEntries belong to the same
	 * filesystem but different mountpoints.  This is intentional.
	 */
	if (from.mount != to.mount) {
		dput (to_parent);
		dput (from_parent);
		path_put (to);
		path_put (from);
		return EXDEV;
	}

	/*
	 * Validate that we may actually write to the directories whose entries
	 * we are trying to modify.
	 */
	INode *old_dir = d_inode (from_parent);
	INode *new_dir = d_inode (to_parent);
	if (!i_may_write (old_dir) || !i_may_write (new_dir)) {
		dput (to_parent);
		dput (from_parent);
		path_put (to);
		path_put (from);
		return EPERM;
	}

	/*
	 * Does the filesystem support rename()?
	 */
	if (!old_dir->i_ops->i_rename) {
		dput (to_parent);
		dput (from_parent);
		path_put (to);
		path_put (from);
		return EOPNOTSUPP;
	}

	/*
	 * Always grab the global rename mutex.  We will release it later if
	 * old_dir == new_dir.
	 */
	ret = rename_mutex.lock_interruptible ();
	/*
	 * We must check lookup_seqretry after taking the rename_mutex.
	 */
	if (ret || lookup_seqretry (jar)) {
		rename_mutex.unlock ();
		dput (to_parent);
		dput (from_parent);
		path_put (to);
		path_put (from);
		if (ret)
			return ret;
		goto again;
	}

	/*
	 * Lock both of the directories: first lock old_dir...
	 */
	ret = i_lock_exclusive (old_dir);
	if (ret) {
		rename_mutex.unlock ();
		dput (to_parent);
		dput (from_parent);
		path_put (to);
		path_put (from);
		return ret;
	}

	/*
	 * ... and next, lock new_dir.
	 */
	if (old_dir != new_dir)
		ret = i_lock_exclusive (new_dir);
	if (ret) {
		i_unlock_exclusive (old_dir);
		rename_mutex.unlock ();
		dput (to_parent);
		dput (from_parent);
		path_put (to);
		path_put (from);
		return ret;
	}

	/*
	 * If old_dir is the same as new_dir, we don't need to hold the rename
	 * mutex any longer.  Release it.
	 */
	if (old_dir == new_dir)
		rename_mutex.unlock ();

	/*
	 * If any of the two DEntries have been detached, it is because we raced
	 * against unlink.  In that case, we must retry the entire thing.
	 */
	if (d_detached (from.dentry) || d_detached (to.dentry)) {
		if (old_dir != new_dir)
			i_unlock_exclusive (new_dir);
		i_unlock_exclusive (old_dir);
		if (old_dir != new_dir)
			rename_mutex.unlock ();
		dput (to_parent);
		dput (from_parent);
		path_put (to);
		path_put (from);
		goto again;
	}

	/*
	 * Ok, at this point we have:
	 * - Two locked parent directories that we have permission to write to.
	 * - Two child DEntries belonging to the parent directories and of the
	 * correct names.
	 * - The global rename mutex is held unless the parent directories are
	 * the same.
	 *
	 * Perform the last checks that this rename operation is valid, then
	 * do the rename.
	 */

	 INode *to_inode = d_inode (to.dentry);
	 INode *from_inode = d_inode (from.dentry);

	 if (from.dentry == to.dentry)
	 	/*
		 * We cannot move a DEntry into itself.
		 */
		ret = EINVAL;

	 if (!ret && flags & RENAME_NOREPLACE && to_inode)
		/*
		 * This operation would replace an INode at the target path.
		 */
		ret = EEXIST;

	if (!ret && old_dir != new_dir && is_ancestor (from.dentry, to.dentry))
		/*
		 * This rename would create a disconnected circuit in the DEntry
		 * tree.
		 */
		ret = EINVAL;

	if (!ret && old_dir != new_dir && (flags & RENAME_EXCHANGE)) {
		if (is_ancestor (to.dentry, from.dentry))
			/*
			 * This rename would create a disconnected circuit in
			 * the DEntry tree.
			 */
			ret = EINVAL;
	}

	if (ret) {
		if (old_dir != new_dir)
			i_unlock_exclusive (new_dir);
		i_unlock_exclusive (old_dir);
		if (old_dir != new_dir)
			rename_mutex.unlock ();
		dput (to_parent);
		dput (from_parent);
		path_put (to);
		path_put (from);
		return ret;
	 }

	 /*
	  * Lock the target inodes and do the rename.  First lock the INode
	  * being moved from...
	  */
	 ret = i_lock_exclusive (from_inode);
	 if (ret) {
		if (old_dir != new_dir)
			i_unlock_exclusive (new_dir);
		i_unlock_exclusive (old_dir);
		if (old_dir != new_dir)
			rename_mutex.unlock ();
		dput (to_parent);
		dput (from_parent);
		path_put (to);
		path_put (from);
		return ret;
	}

	/*
	 * Next, lock the INode being moved to...
	 */
	if (to_inode)
		ret = i_lock_exclusive (to_inode);
	if (ret) {
		i_unlock_exclusive (from_inode);
		if (old_dir != new_dir)
			i_unlock_exclusive (new_dir);
		i_unlock_exclusive (old_dir);
		if (old_dir != new_dir)
			rename_mutex.unlock ();
		dput (to_parent);
		dput (from_parent);
		path_put (to);
		path_put (from);
		return ret;
	}

	/*
	 * Finally, do the rename.
	 */
	ret = old_dir->i_ops->i_rename (
			old_dir, from.dentry,
			new_dir, to.dentry,
			flags
	);

	if (to_inode)
		i_unlock_exclusive (to_inode);
	i_unlock_exclusive (from_inode);
	if (old_dir != new_dir)
		i_unlock_exclusive (new_dir);
	i_unlock_exclusive (old_dir);
	if (old_dir != new_dir)
		rename_mutex.unlock ();
	dput (to_parent);
	dput (from_parent);
	path_put (to);
	path_put (from);
	return ret;
}

/**
 * do_unlink - perform an unlink operation.
 * @dirfd: directory file descriptor
 * @filename: name to remove
 * @is_rmdir: whether this is an rmdir() operation or not
 */
static int
do_unlink (int dirfd, const char *filename, bool is_rmdir)
{
	lookup_cookies jar = lookup_seqbegin ();
again:
	Path path;
	DEntry *parent;
	int ret = do_get_path (dirfd, filename,
			LOOKUP_PARENT_DIR | LOOKUP_NOFOLLOW, &path, &parent);
	if (ret)
		return ret;

	INode *dir = d_inode (parent);
	ret = i_lock_exclusive (dir);
	if (ret) {
		dput (parent);
		path_put (path);
		return ret;
	}

	if (lookup_seqretry (jar)) {
		i_unlock_exclusive (dir);
		dput (parent);
		path_put (path);
		goto again;
	}

	if (d_detached (path.dentry)) {
		i_unlock_exclusive (dir);
		dput (parent);
		path_put (path);
		goto again;
	}

	INode *inode = d_inode (path.dentry);
	if (!inode)
		ret = ENOENT;
	if (!ret)
		ret = i_lock_exclusive (inode);
	if (ret) {
		i_unlock_exclusive (dir);
		dput (parent);
		path_put (path);
		return ret;
	}

	inode->i_lock.lock_dpc ();
	mode_t mode = inode->mode;
	inode->i_lock.unlock_dpc ();

	if (__S_ISDIR(mode) && !is_rmdir)
		ret = EISDIR;
	else if (!__S_ISDIR(mode) && is_rmdir)
		ret = ENOTDIR;

	if (!ret && !i_may_write (dir))
		ret = EPERM;
	if (!ret && !dir->i_ops->i_unlink)
		ret = EOPNOTSUPP;
	if (!ret)
		ret = dir->i_ops->i_unlink (dir, path.dentry);
	if (!ret)
		i_set_rmdired (inode);

	i_unlock_exclusive (inode);
	i_unlock_exclusive (dir);
	dput (parent);
	path_put (path);
	return ret;
}

/**
 * ksys_unlink - unlink a name.
 * @dirfd: directory file descriptor
 * @filename: name to remove
 */
int
ksys_unlink (int dirfd, const char *filename)
{
	return do_unlink (dirfd, filename, false);
}

/**
 * ksys_rmdir - remove a directory.
 * @dirfd: directory file descriptor
 * @filename: name to remove
 */
int
ksys_rmdir (int dirfd, const char *filename)
{
	return do_unlink (dirfd, filename, true);
}

/**
 * ksys_mkdir - make a directory.
 * @dirfd: directory file descriptor
 * @filename: name of new directory
 * @mode: mode with which to create the directory
 */
int
ksys_mkdir (int dirfd, const char *filename, mode_t mode)
{
	DEntry *parent;
	Path path;

	uid_t uid;
	gid_t gid;
	mode_t umask;
	read_uid_gid_umask (&uid, &gid, &umask);

	mode = mode & 0777 & ~umask;
	mode = mode | __S_IFDIR;

again:
	int ret = vfs_path (dirfd, filename,
			LOOKUP_PARENT_DIR | LOOKUP_NOFOLLOW | LOOKUP_NO_ENOENT,
			&path, &parent);
	if (ret)
		return ret;

	INode *dir = d_inode(parent);
	ret = i_lock_exclusive (dir);
	if (ret) {
		dput (parent);
		path_put (path);
		return ret;
	}

	if (i_was_rmdired (dir)) {
		i_unlock_exclusive (dir);
		dput (parent);
		path_put (path);
		goto again;
	}

	d_lock (path.dentry);
	if (path.dentry->parent != parent || d_detached (path.dentry)) {
		d_unlock (path.dentry);
		i_unlock_exclusive (dir);
		goto again;
	}
	d_unlock (path.dentry);

	if (d_inode (path.dentry))
		ret = EEXIST;
	if (!ret && !i_may_write (dir))
		ret = EPERM;
	if (!ret && !dir->i_ops->i_mkdir)
		ret = EOPNOTSUPP;
	if (!ret)
		ret = dir->i_ops->i_mkdir (dir, path.dentry, uid, gid, mode);

	i_unlock_exclusive (dir);
	dput (parent);
	path_put (path);
	return ret;
}

/**
 * ksys_mknod - make a filesystem node.
 * @dirfd: directory file descriptor
 * @filename: name of new filesystem node
 * @mode: file creation mode
 * @dev: device number
 */
int
ksys_mknod (int dirfd, const char *filename, mode_t mode, dev_t dev)
{
	switch (mode & __S_IFMT) {
	case __S_IFREG:
	case __S_IFBLK:
	case __S_IFCHR:
		break;
	default:
		return EINVAL;
	}

	mode_t type = mode & __S_IFMT;
	DEntry *parent;
	Path path;

	uid_t uid;
	gid_t gid;
	mode_t umask;
	read_uid_gid_umask (&uid, &gid, &umask);

	mode = mode & 0777 & ~umask;
	mode = mode | __S_IFDIR;
	mode = mode | type;

again:
	int ret = vfs_path (dirfd, filename,
			LOOKUP_PARENT_DIR | LOOKUP_NOFOLLOW | LOOKUP_NO_ENOENT,
			&path, &parent);
	if (ret)
		return ret;

	INode *dir = d_inode(parent);
	ret = i_lock_exclusive (dir);
	if (ret) {
		dput (parent);
		path_put (path);
		return ret;
	}

	if (i_was_rmdired (dir)) {
		i_unlock_exclusive (dir);
		dput (parent);
		path_put (path);
		goto again;
	}

	d_lock (path.dentry);
	if (path.dentry->parent != parent || d_detached (path.dentry)) {
		d_unlock (path.dentry);
		i_unlock_exclusive (dir);
		goto again;
	}
	d_unlock (path.dentry);

	if (d_inode (path.dentry))
		ret = EEXIST;
	if (!ret && !i_may_write (dir))
		ret = EPERM;
	if (!ret && !dir->i_ops->i_mknod)
		ret = EOPNOTSUPP;
	if (!ret)
		ret = dir->i_ops->i_mknod (dir, path.dentry, uid, gid, mode, dev);

	i_unlock_exclusive (dir);
	dput (parent);
	path_put (path);
	return ret;
}

