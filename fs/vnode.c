/**
 * vnode cache
 * Copyright (C) 2025-present  dbstream
 *
 * This file is responsible for vnode management and lookup.
 */
#include <davix/fs.h>
#include <davix/kmalloc.h>
#include <davix/panic.h>
#include <davix/slab.h>
#include <davix/string.h>

/**
 * Ideas for a vnode LRU cache:
 *   In the futute we might not want to immediately dispose of vnodes as we do
 *   now when their refcount reaches zero.  Instead, when a vnode reaches zero
 *   refs, we should put it on an LRU.  When something wants to collect a node
 *   from the LRU, it does the following:
 *     1) lock the LRU
 *     2) acquire a reference to the cached node and remove it from the LRU.
 *     3) unlock the LRU
 *     4) lock the node's parent
 *     5) kill the node
 *     6) unlock the node's parent
 *   The vnode cache must be careful not to race vnode lookup against step 2.
 *   Otherwise, the same vnode may be removed from the LRU twice.
 */

static struct slab *vnode_slab;

struct vn_hash_bucket {
	struct vnode *first;
	spinlock_t lock;
};

static struct vn_hash_bucket *vn_hash_table;
static size_t vn_hash_table_mask;
static unsigned int vn_hash_table_shift;

void
init_vnode_cache (void)
{
	vnode_slab = slab_create ("struct vnode", sizeof (struct vnode));
	if (!vnode_slab)
		panic ("init_vnode_cache: failed to create vnode_slab!");

	/** 
	 * Use 65536 entries in the hash table. Since a vn_hash_bucket is 16
	 * bytes, this will use 1MiB of memory for the vnode hash table.
	 */
	vn_hash_table_shift = 16;
	vn_hash_table_mask = (1UL << vn_hash_table_shift) - 1;
	size_t n = 1UL << vn_hash_table_shift;

	vn_hash_table = kmalloc (n * sizeof (struct vn_hash_bucket));
	if (!vn_hash_table)
		panic ("init_vnode_cache: failed to allocate vn_hash_table!");

	for (size_t i = 0; i < n; i++) {
		vn_hash_table[i].first = NULL;
		spinlock_init (&vn_hash_table[i].lock);
	}
}

/**
 * Calculate the hash bucket that a vnode belongs to.
 *
 * @parent	the vnode's parent.
 * @name	name of the vnode.
 * @name_len	length of the vnode name
 *
 * NOTE: the returned value is not masked with vn_hash_table_mask.
 */
static size_t
get_vnode_hash (struct vnode *parent, const char *name, size_t name_len)
{
	/**
	 * TODO: validate the decency of this hash function.
	 */
	uint64_t x = (uint64_t) parent;
	for (; name_len; name++, name_len--)
		/**
		 * nothing up my sleeve:
		 *   (x << 17) ^ (x >> 47) is a left-rotation of 17.
		 *   16180339 originates from the golden ratio, ~1.6180339.
		 */
		x = ((x << 17) ^ (x >> 47) ^ *name) + 16180339UL;

	/**
	 * Fold the upper bits of the hash so that when it is masked with
	 * vn_hash_table_mask, they still contribute to the bucket index.
	 */
	return x ^ (x >> vn_hash_table_shift)
			^ (x >> (2 * vn_hash_table_shift))
			^ (x >> (3 * vn_hash_table_shift));
}

/**
 * Unlink a vnode from the hash table.
 *
 * @vnode	vnode to unlink.  vnode->vn_hash must be correct!
 *
 * This does not set VN_UNHOOKED.
 */
static void
__vn_hash_unlink (struct vnode *vnode)
{
	size_t hash = vnode->vn_hash;
	size_t bucket_idx = hash & vn_hash_table_mask;

	spin_lock (&vn_hash_table[bucket_idx].lock);
	*vnode->vn_hash_link = vnode->vn_hash_next;
	if (vnode->vn_hash_next)
		vnode->vn_hash_next->vn_hash_link = vnode->vn_hash_link;
	spin_unlock (&vn_hash_table[bucket_idx].lock);
}

/**
 * Insert a vnode into the hash table.
 *
 * @vnode	vnode to insert.  vnode->vn_hash must be correct!
 */
static void
__vn_hash_insert (struct vnode *vnode)
{
	size_t hash = vnode->vn_hash;
	size_t bucket_idx = hash & vn_hash_table_mask;

	spin_lock (&vn_hash_table[bucket_idx].lock);
	vnode->vn_hash_link = &vn_hash_table[bucket_idx].first;
	vnode->vn_hash_next = vn_hash_table[bucket_idx].first;
	vn_hash_table[bucket_idx].first = vnode;
	if (vnode->vn_hash_next)
		vnode->vn_hash_next->vn_hash_link = &vnode->vn_hash_next;
	spin_unlock (&vn_hash_table[bucket_idx].lock);
}

/**
 * Look for a vnode in the hash table.
 *
 * @hash	vnode hash, as returned by get_vnode_hash
 * @name	vnode name
 * @name_len	length of the vnode name
 *
 * Returns a vnode with its refcount incremented, or NULL if no vnode was found.
 */
static struct vnode *
vn_hash_find (size_t hash, const char *name, size_t name_len)
{
	size_t bucket_idx = hash & vn_hash_table_mask;

	spin_lock (&vn_hash_table[bucket_idx].lock);

	struct vnode *current = vn_hash_table[bucket_idx].first;
	for (; current; current = current->vn_hash_next) {
		/** first, do a cheap test of the hashes */
		if (current->vn_hash != hash)
			continue;

		/** next, do a cheap test of the name length */
		if (current->name.name_len != name_len)
			continue;

		/** finally, actually look at the name */
		bool match = true;
		for (size_t i = 0; i< name_len; i++) {
			if (current->name.name_ptr[i] != name[i]) {
				match = false;
				break;
			}
		}

		if (match) {
			vn_get (current);
			break;
		}
	}

	spin_unlock (&vn_hash_table[bucket_idx].lock);
	return current;
}

/**
 * Anything longer than sizeof(vname->inline_name) is managed using an extra
 * allocation outside of the vnode.  That way we keep struct vnode to a fixed
 * size (which is great for allocation).
 */
struct vname_big {
	refcount_t refcount;
	char name[];
};

void
__vname_big_get (struct vname *name)
{
	struct vname_big *big = struct_parent (
			struct vname_big, name, (void *) name->name_ptr);

	refcount_inc (&big->refcount);
}

void
__vname_big_put (struct vname *name)
{
	struct vname_big *big = struct_parent (
			struct vname_big, name, (void *) name->name_ptr);

	if (refcount_dec (&big->refcount))
		kfree (big);
}

static bool
vname_make (struct vname *vname, const char *name, size_t name_len)
{
	vname->name_len = name_len;
	if (name_len < sizeof (vname->inline_name)) {
		vname->name_ptr = vname->inline_name;
		memcpy (vname->inline_name, name, name_len);
		vname->inline_name[name_len] = 0;
		return true;
	}

	/** avoid addition overflow  :p */
	if (name_len > -2UL - sizeof (refcount_t))
		return false;

	struct vname_big *big = kmalloc (
			offsetof (struct vname_big, name[name_len + 1]));
	if (!big)
		return false;

	refcount_set (&big->refcount, 1);
	vname->name_ptr = big->name;
	memcpy (big->name, name, name_len);
	big->name[name_len] = 0;
	return true;
}

/**
 * Allocate the root vnode of a filesystem.
 */
struct vnode *
alloc_root_vnode (struct filesystem *fs)
{
	struct vnode *node = slab_alloc (vnode_slab);
	if (!node)
		return NULL;

	memset (node, 0, sizeof (*node));

	node->vn_hash = 0;
	node->vn_parent = NULL;
	node->vn_children = NULL;
	node->vn_childnext = NULL;
	node->vn_childlink = NULL;
	node->fs = fs;
	node->vn_inode = NULL;
	node->vn_flags = VN_UNHOOKED;
	spinlock_init (&node->vn_lock);
	refcount_set (&node->refcount, 1);

	return node;
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
		const char *name, size_t name_len)
{
	size_t hash = get_vnode_hash (parent, name, name_len);
	spin_lock (&parent->vn_lock);

	*child = vn_hash_find (hash, name, name_len);
	if (*child) {
		spin_unlock (&parent->vn_lock);
		return ESUCCESS;
	}

	struct vnode *node = slab_alloc (vnode_slab);
	if (!node) {
		spin_unlock (&parent->vn_lock);
		return ENOMEM;
	}

	memset (node, 0, sizeof (*node));

	node->vn_hash = hash;
	if (!vname_make (&node->name, name, name_len)) {
		spin_unlock (&parent->vn_lock);
		slab_free (node);
		return ENOMEM;
	}

	node->vn_parent = vn_get (parent);
	node->vn_children = NULL;
	node->fs = parent->fs;
	node->vn_inode = NULL;
	node->vn_flags = VN_NEEDLOOKUP;
	spinlock_init (&node->vn_lock);
	refcount_set (&node->refcount, 1);

	__vn_hash_insert (node);
	node->vn_childnext = parent->vn_children;
	if (node->vn_childnext)
		node->vn_childlink = &node->vn_childnext;
	node->vn_childlink = &parent->vn_children;
	parent->vn_children = node;
	spin_unlock (&parent->vn_lock);

	*child = node;
	return ESUCCESS;
}

static void
__vn_unhook (struct vnode *vnode)
{
	__vn_hash_unlink (vnode);
	*vnode->vn_childlink = vnode->vn_childnext;
	if (vnode->vn_childnext)
		vnode->vn_childnext->vn_childlink = vnode->vn_childlink;
	vnode->vn_flags |= VN_UNHOOKED;
}

/**
 * Destroy a vnode when its reference count has reached zero.  The vnode has
 * been unhooked by the caller.
 *
 * @vnode	vnode to destroy
 */
static void
vn_put_final (struct vnode *vnode)
{
	if (vnode->vn_inode)
		iput (vnode->vn_inode);
	vname_put (&vnode->name);
	slab_free (vnode);
}

/**
 * Decrease the reference count of a vnode by one.
 *
 * @vnode	vnode whose refcount to decrease
 */
void
vn_put (struct vnode *vnode)
{
	/**
	 * Every vnode owns a reference to its parent.  Perform vn_put work in a
	 * loop so we can vn_put the parent if we reach a refcount of zero.
	 */
	do {
		/**
		 * It really sucks that we have to do it in this way. Maybe in the
		 * future we can RCU it.     :^)
		 */

		/**
		 * Optimistically attempt a fast non-final vn_put.
		 */
		unsigned long old_refcount = refcount_read (&vnode->refcount);
		for (int i = 0; i < 10; i++) {
			unsigned long new_refcount = old_refcount - 1;

			if (!new_refcount)
				/**
				 * We cannot do a final vn_put without holding
				 * vn_lock or knowing that we are unhooked.
				 */
				break;

			if (refcount_cmpxchg (&vnode->refcount,
					&old_refcount, new_refcount))
						return;
		}

		/**
		 * Perform slow (potentially final) vn_put.  Unless VN_UNHOKKED,
		 * we need to acquire the parent's vn_lock so no one can lookup
		 * us.
		 **/
		struct vnode *parent = vnode->vn_parent;
		bool need_unhook = true;
		if (parent) {
			/**
			 * Protect against concurrent unhooking by holding the
			 * parent's lock.
			 */
			spin_lock (&parent->vn_lock);

			/**
			 * Stabilise vnode->vn_flags by holding vn_lock.
			 */
			spin_lock (&vnode->vn_lock);
			if (vnode->vn_flags & VN_UNHOOKED) {
				/**
				 * If we are unhooked, we do not need to hold
				 * any vn_locks.
				 */
				spin_unlock (&vnode->vn_lock);
				spin_unlock (&parent->vn_lock);
				need_unhook = false;
			}
		}

		if (!refcount_dec (&vnode->refcount)) {
			/**
			 * This was a non-final vn_put.  Any final vn_put needs
			 * to hold vn_put, so parent and vnode are not dangling
			 * pointers.
			 */
			if (parent && need_unhook) {
				spin_unlock (&vnode->vn_lock);
				spin_unlock (&parent->vn_lock);
			}

			return;
		}

		if (parent && need_unhook) {
			/**
			 * This was a final vn_put and we must unhook the vnode
			 * before zeroing it.  No new references can be acquired
			 * to the vnode, as the parent is locked.
			 */
			__vn_unhook (vnode);
			spin_unlock (&vnode->vn_lock);
			spin_unlock (&parent->vn_lock);
		}

		vn_put_final (vnode);
		vnode = parent;
	} while (vnode);
}

