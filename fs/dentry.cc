/**
 * Directory entry (DEntry) cache.
 * Copyright (C) 2025-present  dbstream
 */
#include <asm/cache.h>
#include <asm/percpu.h>
#include <davix/condwait.h>
#include <davix/fs_types.h>
#include <davix/fs.h>
#include <davix/rcu.h>
#include <davix/panic.h>
#include <davix/slab.h>
#include <davix/vmap.h>
#include <dsl/swap.h>
#include <uapi/davix/errno.h>
#include <string.h>
#include "internal.h"

DEntry *
dget (DEntry *de)
{
	refcount_inc (&de->refcount);
	return de;
}

static SlabAllocator *dentry_slab_cache;

struct dcache_bucket_t {
	DEntryHashList hlist;
	spinlock_t lock;
};

static uintptr_t dcache_hash_size;
static uintptr_t dcache_hash_mask;

static dcache_bucket_t *dcache_hashtable;

void
init_dentry_cache (void)
{
	dentry_slab_cache = slab_create ("DEntry", sizeof (DEntry), CACHELINE_SIZE);
	if (!dentry_slab_cache)
		panic ("Failed to create DEntry slab cache!");
	/*
	 * Number of DEntry hash buckets.  On a 64-bit system number of bytes is
	 * equal to 16 * this.
	 *
	 *   shift  number of entries  table size
	 *       8                256        4KiB
	 *       9                512        8KiB
	 *      10               1024       16KiB
	 *      11               2048       32KiB
	 *      12               4096       64KiB
	 *      13               8192      128KiB
	 *      14              16384      256KiB
	 *      15              32768      512KiB
	 *      16              65536        1MiB
	 *
	 * In the future, we may increase this.  Or we might decide at runtime
	 * based on how much usable RAM is available.
	 */
	dcache_hash_size = 1UL << 10;
	dcache_hash_mask = dcache_hash_size - 1UL;

	dcache_hashtable = (dcache_bucket_t *)
		kmalloc_large (sizeof (dcache_bucket_t) * dcache_hash_size);
	if (!dcache_hashtable)
		panic ("Failed to create DEntry hash table!");

	for (uintptr_t i = 0; i < dcache_hash_size; i++) {
		dcache_hashtable[i].hlist.init ();
		dcache_hashtable[i].lock.init ();
	}
}

/**
 * __detach_dentry - remove a DEntry from the hash table.
 * @de: dentry to remove
 *
 * Locking: the DEntry lock must be held on @de.
 */
static void
__detach_dentry (DEntry *de)
{
	dcache_bucket_t *bucket = &dcache_hashtable[de->d_hash & dcache_hash_mask];

	bucket->lock.lock_dpc ();
	de->dentry_hash_linkage.remove ();
	bucket->lock.unlock_dpc ();
}

/**
 * __attach_dentry - insert a DEntry into the hash table.
 * @de: dentry to insert
 *
 * Locking: the DEntry lock must be held on @de, or the caller must have
 * exclusive access to the DEntry by some other means.
 */
static void
__attach_dentry (DEntry *de)
{
	dcache_bucket_t *bucket = &dcache_hashtable[de->d_hash & dcache_hash_mask];

	bucket->lock.lock_dpc ();
	bucket->hlist.push (de);
	bucket->lock.unlock_dpc ();
}

/**
 * __detach_dentry_unless_referenced - try to remove a DEntry from lookup.
 * @de: dentry to remove
 * Returns true if the DEntry was removed and it has no references.  If this
 * returns false, the DEntry has not been removed from lookup.
 */
static bool
__detach_dentry_unless_referenced (DEntry *de)
{
	dcache_bucket_t *bucket = &dcache_hashtable[de->d_hash & dcache_hash_mask];

	bucket->lock.lock_dpc ();
	if (atomic_load_relaxed (&de->refcount) != 0) {
		bucket->lock.unlock_dpc ();
		return false;
	}

	de->dentry_hash_linkage.remove ();
	bucket->lock.unlock_dpc ();
	return true;
}

/**
 * __find_dentry - Find a DEntry in the hash table.
 * @hash: dentry hash
 * @parent: parent dentry
 * @name: dentry name
 * @name_len: dentry name length
 */
static DEntry *
__find_dentry (uintptr_t hash, DEntry *parent, const char *name, size_t name_len)
{
	dcache_bucket_t *bucket = &dcache_hashtable[hash & dcache_hash_mask];

	bucket->lock.lock_dpc ();
	for (DEntry *de : bucket->hlist) {
		if (de->d_hash != hash)
			continue;
		if (de->parent != parent)
			continue;
		if (de->name.name_len != name_len)
			continue;
		if (!memcmp (de->name.name_ptr, name, name_len)) {
			dget(de);
			bucket->lock.unlock_dpc ();
			return de;
		}
	}
	bucket->lock.unlock_dpc ();
	return nullptr;
}

/**
 * new_dentry - allocate a new DEntry and initialize its fields.
 * @fs: filesystem the dentry will belong to
 */
static DEntry *
new_dentry (Filesystem *fs)
{
	DEntry *de = (DEntry *) slab_alloc (dentry_slab_cache, ALLOC_KERNEL);
	if (!de)
		return nullptr;

	de->parent = nullptr;
	de->fs = fs_get (fs);
	de->inode = nullptr;
	de->d_flags = D_DETACHED | D_NEED_LOOKUP;
	de->lock.init ();
	de->refcount = 1;
	de->d_hash = 0;
	de->name.name_ptr = de->name.inline_name;
	de->name.name_len = 1;
	strncpy (de->name.inline_name, "/", 1);
	return de;
}

/**
 * free_dentry - free a DEntry.
 * @de: dentry to free
 *
 * This does NOT free the DEntry name.
 */
static void
free_dentry (DEntry *de)
{
	fs_put (de->fs);
	slab_free (de);
}

/**
 * allocate_root_dentry - allocate a DEntry for the root inode of a filesystem.
 * @fs: filesystem the dentry will belong to
 */
DEntry *
allocate_root_dentry (Filesystem *fs)
{
	DEntry *de = new_dentry (fs);
	/*
	 * Root never needs lookup.
	 */
	de->d_flags &= ~D_NEED_LOOKUP;
	return de;
}

/**
 * compute_dentry_hash - calculate the hashed value of the parent and the name.
 * @parent: parent pointer
 * @name: dentry name
 * @name_len: dentry name length
 */
uintptr_t
compute_dentry_hash (DEntry *parent, const char *name, size_t name_len)
{
	static constexpr size_t N = (sizeof (uintptr_t) < 8) ? 4 : 8;

	uintptr_t x = (uintptr_t) parent;
	/*
	 * Begin by processing names in chunks of N characters at a time.
	 */
	while (name_len >= N) {
		/*
		 * The compiler will unroll this loop (hopefully).
		 */
		for (size_t i = 0; i < N; i++)
			x += (uintptr_t) name[i] << (8 * i);

		x ^= x << 7;
		x ^= x >> 9;

		name += N;
		name_len -= N;
	}
	/*
	 * Handle the 'tail' of up to N-1 characters.
	 */
	for (size_t i = 0; i < name_len; i++)
		x += name[i] << (8 * i);

	/*
	 * Fold the upper bits of the hash value into the lower bits, so that
	 * when masked with dcache_hash_mask, they contribute to the bucket
	 * index.
	 */
	x ^= x >> 32;
	x ^= x >> 16;
	x ^= x >> 8;
	return x;
}

static bool
make_dname (DName *out, const char *name, size_t name_len)
{
	unsigned int uint_len = name_len;
	if (uint_len < name_len)
		return false;

	if (name_len + 1 > sizeof(out->inline_name))
		/*
		 * File name doesn't fit in inline_name.
		 *
		 * TODO: use a RefStr in this case.
		 */
		return false;

	out->name_len = uint_len;
	out->name_ptr = out->inline_name;
	memcpy (out->inline_name, name, name_len);
	memset (out->inline_name + name_len, 0,
			sizeof(out->inline_name) - name_len);
	out->inline_name[name_len] = 0;
	return true;
}

static void
free_dname (DName *name)
{
	(void) name;
}

static void
replace_dname (DName *to, DName *from)
{
	memcpy (to->inline_name, from->inline_name, sizeof(DName::inline_name));
}

static void
swap_dnames (DName *a, DName *b)
{
	char buffer[sizeof(DName::inline_name)];
	memcpy (buffer, a->inline_name, sizeof(DName::inline_name));
	memcpy (a->inline_name, b->inline_name, sizeof(DName::inline_name));
	memcpy (b->inline_name, buffer, sizeof(DName::inline_name));
}

/**
 * d_lookup - get a child dentry.
 * @parent: parent dentry
 * @name: dentry name
 * @name_len: dentry name length
 * Returns a directory entry or nullptr on failure.
 *
 * If the child dentry does not exist in the dentry cache, this function tries
 * to create it.  This function does not look up an underlying inode.  It is
 * assumed that @parent's inode exists and is a directory.
 */
DEntry *
d_lookup (DEntry *parent, const char *name, size_t name_len)
{
	uintptr_t hash = compute_dentry_hash (parent, name, name_len);

	/*
	 * First try looking the DEntry up from the cache without locking
	 * @parent.
	 */
	DEntry *de = __find_dentry (hash, parent, name, name_len);
	if (de)
		return de;

	/*
	 * At some point the child DEntry was not in cache.  Try to create a new
	 * DEntry.
	 */
	de = new_dentry (parent->fs);
	if (!de)
		return nullptr;

	if (!make_dname (&de->name, name, name_len)) {
		free_dentry (de);
		return nullptr;
	}

	de->d_hash = hash;
	de->parent = dget (parent);
	/*
	 * We will insert the DEntry into the hash table immediately.
	 */
	de->d_flags &= ~D_DETACHED;
	/*
	 * If i_lookup on the parent inode is cleared, there is no point in ever
	 * entering lookup on the DEntry.
	 */
	if (!parent->inode->i_ops->i_lookup)
		de->d_flags &= ~D_NEED_LOOKUP;
	/*
	 * Lock the parent to prevent concurrent inserts.
	 */
	d_lock (parent);
	/*
	 * We must recheck the cache under the parent DEntry lock.  Otherwise we
	 * might race inserting our DEntry into the cache with someone else.
	 */
	DEntry *in_cache = __find_dentry (hash, parent, name, name_len);
	if (!in_cache)
		__attach_dentry (de);
	d_unlock (parent);

	if (in_cache) {
		/*
		 * Someone else already inserted their DEntry into the cache.
		 */
		dput (parent);
		free_dname (&de->name);
		free_dentry (de);

		de = in_cache;
	}

	return de;
}

/**
 * d_parent_inode - get the parent dentry's inode of a dentry.  This must not be
 * called for a dentry which is the root of a directory hierarchy.
 */
static INode *
d_parent_inode (DEntry *de)
{
	return de->parent->inode;
}

/**
 * d_ensure_inode - ensure that the inode has been looked up on a dentry.
 * @de: dentry
 * Returns an errno, or zero indicating success.
 *
 * Even if we return zero, it is possible that @de->inode is nullptr.  This is
 * because we consider ENOENT a successful return from i_lookup.  This function
 * however returns with D_NEED_LOOKUP cleared on @de.
 */
int
d_ensure_inode (DEntry *de)
{
retry:
	if (!(atomic_load_acquire (&de->d_flags) & D_NEED_LOOKUP))
		/*
		 * DEntry has already been looked up.
		 */
		return 0;

	d_lock (de);
	/*
	 * Recheck under the DEntry lock if someone else has already looked up
	 * this DEntry.
	 */
	if (!(atomic_load_relaxed (&de->d_flags) & D_NEED_LOOKUP)) {
		d_unlock (de);
		return 0;
	}

	if (de->d_flags & D_LOOKUP_IN_PROGRESS) {
		/*
		 * Someone else is in lookup on this DEntry: wait for them to
		 * finish.
		 */
		d_unlock (de);
		int errno = condwait_interruptible(de, de,
			!(atomic_load_acquire (&de->d_flags) & D_LOOKUP_IN_PROGRESS)
		);
		if (errno != 0)
			return errno;
		goto retry;
	}
	/*
	 * Noone else is looking this DEntry up: set the LOOKUP_IN_PROGRESS bit.
	 */
	atomic_set_bits (&de->d_flags, D_LOOKUP_IN_PROGRESS, mo_relaxed);
	d_unlock (de);
	/*
	 * It is safe to rely on the parent inode not changing, as it can only
	 * change as a result of rename or unlink, which require this DEntry to
	 * be looked up.
	 */
	INode *dir = d_parent_inode (de);
	/*
	 * Lock the parent inode.
	 */
	int errno = i_lock_shared (dir);
	if (errno != 0) /* errno == EINTR */ {
		atomic_clr_bits (&de->d_flags, D_LOOKUP_IN_PROGRESS, mo_release);
		condwait_touch (de);
		return errno;
	}
	/*
	 * If i_lookup on the directory is NULL, d_lookup clears D_NEED_LOOKUP
	 * and we wouldn't be here.  Therefore we don't need to check.
	 *
	 * The filesystem driver must be able to handle lookup on a directory
	 * inode which has itself been unlinked.  Correct behavior in that case
	 * is to return ENOENT on everything.
	 */
	errno = dir->i_ops->i_lookup (dir, de);
	i_unlock_shared (dir);
	atomic_clr_bits (&de->d_flags, D_NEED_LOOKUP | D_LOOKUP_IN_PROGRESS,
			mo_release);
	condwait_touch (de);

	return errno == ENOENT ? 0 : errno;
}

static DEntryLRU dcache_lru_list;
static size_t dcache_lru_list_size;
static spinlock_t dcache_lru_list_lock;

static void
d_remove_from_lru (DEntry *de)
{
	dcache_lru_list_lock.lock_dpc ();
	dcache_lru_list_size--;
	de->dentry_lru_head.remove ();
	dcache_lru_list_lock.unlock_dpc ();
}

static void
d_add_to_lru (DEntry *de)
{
	dcache_lru_list_lock.lock_dpc ();
	dcache_lru_list.push_back (de);
	dcache_lru_list_size++;
	dcache_lru_list_lock.unlock_dpc ();
}

/**
 * d_destroy - destroy and free a DEntry.
 * @dentry: DEntry we are destroying
 *
 * This function does not dput() the parent.
 */
static void
d_destroy (DEntry *dentry)
{
	INode *inode = dentry->inode;

	if (inode)
		iput (inode);

	free_dname (&dentry->name);
	free_dentry (dentry);
}

/**
 * d_free_rcu_callback - callback for RCU-freeing a DEntry.
 * @dentry: dentry
 */
static void
d_free_rcu_callback (RCUHead *rcu)
{
	DEntry *dentry = container_of(&DEntry::rcu, rcu);
	d_destroy (dentry);
}

/**
 * d_free_rcu - free a DEntry under RCU.
 * @dentry: dentry to free
 * @flags: last seen flags on the DEntry
 * Returns the next DEntry that should be freed.
 *
 * It is assumed that @dentry is already removed from the hashtable and is
 * unreachable except by other deleters.
 */
static DEntry *
d_free_rcu (DEntry *dentry, unsigned int flags)
{
	if (flags & D_FREED)
		return nullptr;

	DEntry *parent = dentry->parent;
	atomic_store_relaxed (&dentry->d_flags, flags | D_FREED);
	d_unlock (dentry);
	rcu_call (&dentry->rcu, d_free_rcu_callback);
	return parent;
}

/**
 * dput_noinline - the annoying part of dput().
 * @dentry: dentry which is being released
 * @flags: d_flags last seen on the dentry
 * Returns another DEntry that should have its reference count dropped by one,
 * or nullptr if dput is done.
 *
 * This function runs under the RCU read lock held.
 */
static DEntry *
dput_noinline [[gnu::noinline]] (DEntry *dentry, unsigned int flags)
{
	d_lock (dentry);
	if (flags & D_DETACHED) {
		/*
		 * The DEntry was already detached: drop it from the LRU and
		 * free it.
		 */
		if (flags & D_ON_LRU) {
			flags &= ~D_ON_LRU;
			d_remove_from_lru (dentry);
			atomic_store_relaxed (&dentry->d_flags, flags);
		}
		return d_free_rcu (dentry, flags);
	}

	if (atomic_load_relaxed (&dentry->refcount) != 0) {
		/*
		 * If anyone has a reference to the DEntry after we have dput
		 * it, we don't need to free it since someone else will observe
		 * refcount=0 when they free it.
		 */
		d_unlock (dentry);
		return nullptr;
	}

	flags = atomic_load_relaxed (&dentry->d_flags);
	if (flags & D_DETACHED) {
		/*
		 * The DEntry is already detached and we will try to free it
		 * without putting it on the LRU.  However, the D_DETACHED flag
		 * might have been set after we dropped the reference count.
		 */

		/*
		 * Remove it from the LRU if needed:
		 */
		if (flags & D_ON_LRU) {
			flags &= ~D_ON_LRU;
			d_remove_from_lru (dentry);
			atomic_store_relaxed (&dentry->d_flags, flags);
		}

		return d_free_rcu (dentry, flags);
	}

	if (flags & (D_DONT_KEEP | D_NEED_LOOKUP)) {
		/*
		 * The filesystem explicitly wishes not to retain this dentry,
		 * or it does not make sense to do so as it has not been looked
		 * up and is thus caching no useful information. Don't put this
		 * dentry on the LRU.
		 */
		if (!__detach_dentry_unless_referenced (dentry)) {
			/*
			 * Someone else referenced the dentry: our work is done.
			 */
			d_unlock (dentry);
			return nullptr;
		}

		/*
		 * Remove it from the LRU if needed:
		 */
		if (flags & D_ON_LRU) {
			flags &= ~D_ON_LRU;
			d_remove_from_lru (dentry);
			atomic_store_relaxed (&dentry->d_flags, flags);
		}

		return d_free_rcu (dentry, flags);
	}

	/*
	 * Put the DEntry on the LRU.
	 */
	if (!(flags & D_ON_LRU)) {
		flags |= D_ON_LRU;
		flags &= ~D_WAS_REFERENCED;
		atomic_store_relaxed (&dentry->d_flags, flags);
		d_add_to_lru (dentry);
	}

	d_unlock (dentry);
	return nullptr;
}

/**
 * dput - release a DEntry.
 * @dentry: DEntry we are releasing
 */
void
dput (DEntry *dentry)
{
	do {
		rcu_read_lock ();
		/*
		 * Read the DEntry flags and set D_WAS_REFERENCED before
		 * decrementing the refcount.
		 */
		unsigned int flags = atomic_fetch_or (&dentry->d_flags,
				D_WAS_REFERENCED, mo_relaxed);

		/*
		 * Optimistically decrement the refcount.
		 */
		if (!refcount_dec (&dentry->refcount)) [[likely]] {
			/*
			 * We weren't the last user of this DEntry.  In this
			 * case we can simply return.
			 */
			rcu_read_unlock ();
			return;
		}

		/*
		 * Do the slow part of dput() in another function so this can
		 * more easily be inlined.
		 */
		dentry = dput_noinline (dentry, flags);
		rcu_read_unlock ();
	} while (dentry);
}

/**
 * d_lru_trim - remove dentries from the LRU.
 * @target: how many dentries to trim
 * @floor: lower limit at which to break
 * Returns the number of dentries that were rcu_free'd by us.
 */
static size_t
__d_trim_lru (size_t target, size_t floor)
{
	size_t nremoved;
	for (;;) {
		dcache_lru_list_lock.lock_dpc ();
		if (dcache_lru_list_size <= floor) {
			dcache_lru_list_lock.unlock_dpc ();
			break;
		}
		/*
		 * Grab a dentry from the LRU list.
		 */
		rcu_read_lock ();
		DEntry *de = *dcache_lru_list.begin ();

		unsigned int flags;
		/*
		 * Trylock the DEntry.  If it doesn't succeed, we will need to
		 * drop the LRU lock and recheck everything afterwards.
		 */
		if (!(d_trylock (de))) {
			dcache_lru_list_lock.unlock_dpc ();
			d_lock (de);
			flags = atomic_load_relaxed (&de->d_flags);
			/*
			 * Check if the DEntry is still on the LRU.
			 */
			if (!(flags & D_ON_LRU)) {
				/*
				 * The DEntry was removed from the LRU by
				 * someone.
				 */
				d_unlock (de);
				rcu_read_unlock ();
				continue;
			}
			dcache_lru_list_lock.lock_dpc ();
		} else
			/*
			 * Yay!
			 */
			flags = atomic_load_relaxed (&de->d_flags);
		/*
		 * Take it off the LRU.
		 */
		de->dentry_lru_head.remove ();
		if (atomic_load_relaxed (&de->refcount) != 0) {
			/*
			 * If the refcount is nonzero, bookkeep taking it off
			 * the LRU and leave it off the LRU.
			 */
			flags &= ~D_ON_LRU;
			dcache_lru_list_size--;
			dcache_lru_list_lock.unlock_dpc ();
			atomic_store_relaxed (&de->d_flags, flags);
			d_unlock (de);
			rcu_read_unlock ();
			continue;
		}
		if (flags & D_WAS_REFERENCED) {
			/*
			 * For a DEntry where D_WAS_REFERENCED is set, clear it
			 * and let it cycle through the LRU once more.
			 */
			flags &= ~D_WAS_REFERENCED;
			dcache_lru_list.push_back (de);
			dcache_lru_list_lock.unlock_dpc ();
			atomic_store_relaxed (&de->d_flags, flags);
			d_unlock (de);
			rcu_read_unlock ();
			continue;
		}
		dcache_lru_list_size--;
		dcache_lru_list_lock.unlock_dpc ();
		/*
		 * FIXME: do we need to check D_DETACHED here?  Probably not,
		 * since a detached DEntry is never put on the LRU.
		 */
		if (!__detach_dentry_unless_referenced (de)) {
			/*
			 * The DEntry refcount is again nonzero.
			 */
			flags &= ~D_ON_LRU;
			atomic_store_relaxed (&de->d_flags, flags);
			d_unlock (de);
			rcu_read_unlock ();
			continue;
		}
		/*
		 * Now we are committed to freeing this DEntry.
		 */
		flags &= ~D_ON_LRU;
		d_free_rcu (de, flags);
		rcu_read_unlock ();
		nremoved++;

		if (nremoved >= target)
			break;
	}

	return nremoved;
}

static inline void
__d_set_flag (DEntry *de, unsigned int flag)
{
	unsigned int flags = atomic_load_relaxed (&de->d_flags);
	flags |= flag;
	atomic_store_relaxed (&de->d_flags, flags);
}

static inline void
__d_clear_flag (DEntry *de, unsigned int flag)
{
	unsigned int flags = atomic_load_relaxed (&de->d_flags);
	flags &= ~flag;
	atomic_store_relaxed (&de->d_flags, flags);
}

/*
 * Functions called by the filesystem driver:
 */

/**
 * d_set_inode - set the inode pointed to by a DEntry.
 * @dentry: DEntry
 * @inode: INode to install
 *
 * This function is typically called for the following operations:
 * - i_lookup
 * - i_mknod
 * - i_mkdir
 * - i_symlink
 * - i_link
 */
void
d_set_inode (DEntry *dentry, INode *inode)
{
	d_lock (dentry);
	dentry->inode = iget (inode);
	d_unlock (dentry);
}

/**
 * d_set_inode_nocache - set the inode pointed to by a DEntry.
 * @dentry: DEntry
 * @inode: INode to install.
 *
 * This function additionally tells the VFS to free the DEntry immediately when
 * it reaches a reference count of zero, instead of putting it on the LRU.
 *
 * This function is typically called for the following operations:
 * - i_lookup
 * - i_mknod
 * - i_mkdir
 * - i_symlink
 * - i_link
 */
void
d_set_inode_nocache (DEntry *dentry, INode *inode)
{
	d_lock (dentry);
	dentry->inode = iget (inode);
	__d_set_flag (dentry, D_DONT_KEEP);
	d_unlock (dentry);
}

/**
 * d_set_nocache - tell the VFS to not cache this DEntry.
 * @dentry: DEntry
 *
 * This function is typically called for the following operations:
 * - i_lookup
 */
void
d_set_nocache (DEntry *dentry)
{
	d_lock (dentry);
	__d_set_flag (dentry, D_DONT_KEEP);
	d_unlock (dentry);
}

/**
 * d_unlink - remove a DEntry from the tree.
 * @dentry: DEntry
 *
 * This function is typically called for the following operations:
 * - i_unlink
 */
void
d_unlink (DEntry *dentry)
{
	d_lock (dentry);
	/*
	 * Make this dentry unreachable.
	 */
	__detach_dentry (dentry);
	__d_set_flag (dentry, D_DETACHED);
	d_unlock (dentry);
}

/**
 * d_rename - rename a DEntry.
 * @from: source DEntry
 * @to: target DEntry
 * @rename_flags: RENAME_* flags (we only care about RENAME_EXCHANGE)
 *
 * This function is typically called for the following operations:
 * - i_rename
 */
void
d_rename (DEntry *from, DEntry *to, unsigned int rename_flags)
{
	/*
	 * (from,to)->parent is protected by the parent inode locks that are
	 * held during rename.  They also prevent the DEntry locking below
	 * from deadlocking.
	 */
	DEntry *from_parent = from->parent;
	DEntry *to_parent = to->parent;
	d_lock (from_parent);
	if (from_parent != to_parent)
		d_lock (to_parent);
	d_lock (from);
	d_lock (to);

	__detach_dentry (from);
	__detach_dentry (to);

	from->parent = to_parent;
	if (rename_flags & RENAME_EXCHANGE) {
		to->parent = from_parent;
		dsl::swap (from->d_hash, to->d_hash);
		swap_dnames (&from->name, &to->name);
		__attach_dentry (to);
	} else {
		from->d_hash = to->d_hash;
		dget (to_parent);
		replace_dname (&to->name, &from->name);
	}

	__attach_dentry (from);

	d_unlock (to);
	d_unlock(from);
	if (from_parent != to_parent)
		d_unlock (to_parent);
	d_unlock (from_parent);

	if (!(rename_flags & RENAME_EXCHANGE))
		dput (from_parent);
}

/**
 * d_set_overmounted - mark a DEntry as a mountpoint.
 * @dentry: DEntry
 */
void
d_set_overmounted (DEntry *dentry)
{
	d_lock (dentry);
	__d_set_flag (dentry, D_MOUNTPOINT);
	d_unlock (dentry);
}

/**
 * d_clear_overmounted - mark a DEntry as not being a mountpoint.
 * @dentry: DEntry
 */
void
d_clear_overmounted (DEntry *dentry)
{
	d_lock (dentry);
	__d_clear_flag (dentry, D_MOUNTPOINT);
	d_unlock (dentry);
}

static Mutex dcache_lru_trim_mutex;

/**
 * d_trim_lru_full - trim the DEntry cache as much as possible.
 */
void
d_trim_lru_full (void)
{
	dcache_lru_trim_mutex.lock ();
	__d_trim_lru (-1UL, 0);
	dcache_lru_trim_mutex.unlock ();
}

/**
 * d_trim_lru_partial - trim approximately a third of the DEntry cache.
 */
void
d_trim_lru_partial (void)
{
	dcache_lru_trim_mutex.lock ();

	dcache_lru_list_lock.lock_dpc ();
	size_t size = dcache_lru_list_size;
	dcache_lru_list_lock.unlock_dpc ();

	if (size > 3) {
		size /= 3;
		__d_trim_lru (size, 2 * size);
	}
	dcache_lru_trim_mutex.unlock ();
}

