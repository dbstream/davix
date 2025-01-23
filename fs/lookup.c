/**
 * Pathwalker.
 * Copyright (C) 2025-present  dbstream
 */
#include <davix/printk.h>
#include <davix/sched_types.h>
#include <davix/string.h>
#include "internal.h"

errno_t
inode_permission_check (struct inode *inode, mode_t mask)
{
	if (mask & ~(S_IRUSR | S_IWUSR | S_IXUSR | S_IFMT)) {
		printk (PR_ERR "inode_permission called with mask=0%o.  This is a bug.\n",
				mask);
		return EINVAL;
	}

	/**
	 * TODO: get uid and gid from the current process.
	 */
	uid_t uid = 0;
	gid_t gid = 0;

	spin_lock (&inode->i_lock);
	uid_t i_uid = inode->i_uid;
	uid_t i_gid = inode->i_gid;
	mode_t i_mode = inode->i_mode;
	spin_unlock (&inode->i_lock);

	if (mask & S_IFMT) {
		if ((mask & S_IFMT) != (i_mode & S_IFMT)) {
			if (S_ISDIR (mask))
				return ENOTDIR;
			if (S_ISDIR (i_mode))
				return EISDIR;
			return ENXIO;
		}
		mask &= ~S_IFMT;
	}

	_Static_assert (S_IRUSR == S_IRGRP << 3, "inode_permission_check");
	_Static_assert (S_IRUSR == S_IROTH << 6, "inode_permission_check");
	_Static_assert (S_IWUSR == S_IWGRP << 3, "inode_permission_check");
	_Static_assert (S_IWUSR == S_IWOTH << 6, "inode_permission_check");
	_Static_assert (S_IXUSR == S_IXGRP << 3, "inode_permission_check");
	_Static_assert (S_IXUSR == S_IXOTH << 6, "inode_permission_check");

	if (uid == i_uid)
		mask &= ~i_mode;
	if (gid == i_gid)
		mask &= ~(i_mode << 3);
	mask &= ~(i_mode << 6);

	if (mask)
		return EPERM;

	return ESUCCESS;
}

void
pathwalk_finish (struct pathwalk_state *state)
{
	if (state->has_result_component)
		path_put (state->result_component);
	if (state->has_parent_component)
		path_put (state->parent_component);

	path_put (state->origin);
	path_put (state->root);
}

errno_t
pathwalk_begin (struct pathwalk_state *state, int dirfd, const struct pathwalk_info *info)
{
	state->oflag = info->flags;
	state->open_mode = info->mode;
	state->resolve = info->resolve;
	state->has_parent_component = 0;
	state->has_result_component = 0;

	if (dirfd != AT_FDCWD && dirfd != AT_FDROOT) {
		printk (PR_WARN "pathwalk_begin: TODO: implement dirfd.\n");
		return ENOTSUP;
	}

	struct fs_context *fsctx = get_current_task ()->fs;
	if (!fsctx) {
		printk (PR_WARN "pathwalk_begin called on a non-FS process!  (comm: %.16s)\n",
				get_current_task ()->comm);
		return ENXIO;
	}

	struct fs_context_snapshot snapshot = fsctx_take_snapshot (fsctx);
	state->root = snapshot.root;
	state->origin = snapshot.cwd;

	if (state->resolve & RESOLVE_IN_ROOT) {
		path_put (state->root);
		state->root = path_get (state->origin);
	}

	switch (dirfd) {
	case AT_FDCWD:
		state->has_result_component = 1;
		state->result_component = path_get (state->origin);
		break;
	case AT_FDROOT:
		state->has_result_component = 1;
		state->result_component = path_get (state->root);
		break;
	}

	path_put (state->origin);
	state->origin = path_get (state->result_component);

	/**
	 * Ensure that the pathwalk always starts from a present directory.
	 */
	struct inode *inode = state->origin.vnode->vn_inode;
	if (!inode) {
		pathwalk_finish (state);
		return ENOENT;
	}

	spin_lock (&inode->i_lock);
	mode_t mode = inode->i_mode;
	spin_unlock (&inode->i_lock);

	if (!S_ISDIR (mode))
		return ENOTDIR;

	return ESUCCESS;
}

static inline void
clear_parent (struct pathwalk_state *state)
{
	if (state->has_parent_component) {
		path_put (state->parent_component);
		state->has_parent_component = 0;
	}
}

static inline void
clear_result (struct pathwalk_state *state)
{
	if (state->has_result_component) {
		path_put (state->result_component);
		state->has_result_component = 0;
	}
}

static inline void
set_parent (struct pathwalk_state *state, struct path path)
{
	clear_parent (state);
	state->parent_component = path;
	state->has_parent_component = 1;
}

static inline void
set_result (struct pathwalk_state *state, struct path path)
{
	clear_result (state);
	state->result_component = path;
	state->has_result_component = 1;
}

static const char *
strchrnul (const char *s, char c)
{
	while (*s != c) {
		if (!*s)
			break;
		s++;
	}
	return s;
}

struct components {
	const char *cursor;
	unsigned int trailing_slash :1;
};

static void
init_components (struct components *c, const char *pathname)
{
	c->cursor = pathname;
}

static void
free_components (struct components *c)
{
	(void) c;
}

static inline bool
leading_slash (struct components *c)
{
	if (*c->cursor != '/')
		return false;
	do
		c->cursor++;
	while (*c->cursor == '/');
	return true;
}

static inline bool
next_component (struct components *c, const char **component, size_t *length)
{
	if (!*c->cursor)
		return false;

	const char *end = strchrnul (c->cursor, '/');
	*component = c->cursor;
	*length = end - c->cursor;

	c->trailing_slash = *end == '/';
	while (*end == '/')
		end++;
	c->cursor = end;
	return true;
}

static inline bool
is_last (struct components *c)
{
	return !*c->cursor;
}

static inline bool
has_trailing_slash (struct components *c)
{
	return c->trailing_slash;
}

static errno_t
go_to_root (struct pathwalk_state *state)
{
	if (state->resolve & RESOLVE_BENEATH)
		return EXDEV;

	clear_parent (state);
	set_result (state, path_get (state->root));
	return ESUCCESS;
}

static inline bool
can_go_up (struct pathwalk_state *state)
{
	if (!(state->resolve & RESOLVE_BENEATH))
		return true;

	if (state->origin.mount != state->result_component.mount)
		return true;

	if (state->origin.vnode != state->result_component.vnode)
		return true;

	return false;
}

static errno_t
go_up (struct pathwalk_state *state)
{
	if (!can_go_up (state))
		return EXDEV;

	clear_parent (state);

	struct mountpoint *mnt = state->result_component.mount;
	struct vnode *vnode = state->result_component.vnode;

	spin_lock (&vnode->vn_lock);
	if (!vnode->vn_parent || mnt->root == vnode) {
		spin_unlock (&vnode->vn_lock);
		return ESUCCESS;
	}

	struct vnode *parent = vn_get (vnode->vn_parent);
	spin_unlock (&vnode->vn_lock);
	vn_put (vnode);
	state->result_component.vnode = parent;
	return ESUCCESS;
}

errno_t
do_pathwalk (struct pathwalk_state *state, const char *pathname)
{
	struct components comp;
	init_components (&comp, pathname);

	const char *component;
	size_t length;

	errno_t e = ESUCCESS;
	do {
		if (leading_slash (&comp)) {
			e = go_to_root (state);
			continue;
		}

		if (!next_component (&comp, &component, &length))
			break;

		if (!state->has_result_component) {
			printk (PR_ERR "do_pathwalk: state has no result path.  This is a bug.\n");
			e = EINVAL;
			break;
		}

		if (length == 2 && !memcmp ("..", component, 2)) {
			e = go_up (state);
			continue;
		}

		struct inode *inode = state->result_component.vnode->vn_inode;
		e = inode_permission_check (inode, S_IFDIR | S_IXUSR);
		if (e != ESUCCESS)
			break;

		if (length == 1 && !memcmp (".", component, 1)) {
			clear_parent (state);
			continue;
		}

		i_lock_shared (inode);

		struct vnode *child;
		e = vn_get_child (&child, state->result_component.vnode,
				component, length);
		if (e != ESUCCESS) {
			i_unlock_shared (inode);
			break;
		}

		spin_lock (&child->vn_lock);
		if (child->vn_flags & VN_NEEDLOOKUP) {
			if (state->resolve & RESOLVE_NOIO) {
				e = EAGAIN;
			} else {
				e = ENOENT;
				if (inode->ops->lookup_inode)
					e = inode->ops->lookup_inode (inode, child);
			}

			if (e == ESUCCESS || e == ENOENT)
				child->vn_flags &= ~VN_NEEDLOOKUP;
		} else if (!child->vn_inode)
			e = ENOENT;

		spin_unlock (&child->vn_lock);
		i_unlock_shared (inode);

		if (is_last (&comp)) {
			if (e == ENOENT)
				e = ESUCCESS;
		} else if (e == ESUCCESS) {
			inode = child->vn_inode;
			if (!inode) {
				printk (PR_ERR "do_pathwalk: lookup_inode() returned ESUCCESS, but inode is NULL");
				e = ENOENT;
			} else if (!S_ISDIR (inode->i_mode))
				e = ENOTDIR;
		}

		if (e != ESUCCESS) {
			vn_put (child);
			break;
		}

		set_parent (state, path_get (state->result_component));
		vn_put (state->result_component.vnode);
		state->result_component.vnode = child;
	} while (e == ESUCCESS);

	free_components (&comp);
	return e;
}
