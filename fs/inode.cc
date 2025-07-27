/**
 * INode management.
 * Copyright (C) 2025-present  dbstream
 */
#include <asm/cache.h>
#include <davix/fs_types.h>
#include <davix/fs.h>
#include <davix/panic.h>
#include <davix/slab.h>

static SlabAllocator *inode_allocator;

void
init_vfs_inodes (void)
{
	inode_allocator = slab_create ("INode",
			sizeof (INode), CACHELINE_SIZE);
	if (!inode_allocator)
		panic ("Failed to create INode slab cache!");
}

INode *
new_inode (Filesystem *fs, void *i_private)
{
	INode *inode = (INode *) slab_alloc (inode_allocator, ALLOC_KERNEL);
	if (!inode)
		return nullptr;

	inode->fs = fs_get (fs);
	inode->refcount = 1;
	inode->i_ops = nullptr;
	inode->i_mutex.init ();
	inode->uid = 0;
	inode->gid = 0;
	inode->mode = 0;
	inode->i_lock.init ();
	inode->i_state = 0;
	inode->nlink = 0;
	inode->rdev = 0;
	inode->ino = 0;
	inode->size = 0;
	inode->i_private = i_private;
	return inode;
}

INode *
iget (INode *inode)
{
	refcount_inc (&inode->refcount);
	return inode;
}

INode *
iget_maybe_zero (INode *inode)
{
	if (refcount_inc_old_value (&inode->refcount) == 0)
		refcount_inc (&inode->refcount);
	return inode;
}

void
iput (INode *inode)
{
retry:
	if (refcount_dec (&inode->refcount)) {
		if (inode->i_ops->i_close) {
			if (!inode->i_ops->i_close (inode))
				goto retry;
		}

		fs_put (inode->fs);
		slab_free (inode);
	}
}

void
i_set_nlink (INode *inode, nlink_t count)
{
	inode->i_lock.lock_dpc ();
	inode->nlink = count;
	inode->i_lock.unlock_dpc ();
}

void
i_incr_nlink (INode *inode, nlink_t count)
{
	inode->i_lock.lock_dpc ();
	inode->nlink += count;
	inode->i_lock.unlock_dpc ();
}

void
i_decr_nlink (INode *inode, nlink_t count)
{
	inode->i_lock.lock_dpc ();
	inode->nlink -= count;
	inode->i_lock.unlock_dpc ();
}

void
i_set_rmdired (INode *inode)
{
	inode->i_lock.lock_dpc ();
	inode->i_state |= I_DIR_DELETED;
	inode->i_lock.unlock_dpc ();
}

static inline unsigned int
read_i_state (INode *inode)
{
	inode->i_lock.lock_dpc ();
	unsigned int state = inode->i_state;
	inode->i_lock.unlock_dpc ();
	return state;
}

bool
i_was_rmdired (INode *inode)
{
	return (read_i_state (inode) & I_DIR_DELETED) ? true : false;
}

