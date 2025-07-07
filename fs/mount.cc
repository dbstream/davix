/**
 * Mounts management.
 * Copyright (C) 2025-present  dbstream
 */
#include <asm/cache.h>
#include <davix/fs_types.h>
#include <davix/fs.h>
#include <davix/panic.h>
#include <davix/slab.h>
#include <davix/vmap.h>
#include "internal.h"

static SlabAllocator *mount_slab_cache;

struct mount_hash_bucket {
	MountHashList hlist;
	spinlock_t lock;
};

static uintptr_t mount_hash_size;
static uintptr_t mount_hash_mask;

static mount_hash_bucket *mount_hashtable;

void
init_mount_table (void)
{
	mount_slab_cache = slab_create ("Mount", sizeof (Mount), CACHELINE_SIZE);
	if (!mount_slab_cache)
		panic("Failed to create Mount slab cache!");
	/*
	 * Number of Mount hash buckets.  On a 64-bit system number of bytes is
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
	 *
	 * The mount hashtable doesn't need to be this big, but since we
	 * allocate it using kmalloc_large, it doesn't make sense to allocate
	 * less than 4KiB (the page size on x86) for it.
	 */
	mount_hash_size = 1UL << 8;
	mount_hash_mask = mount_hash_size - 1UL;

	mount_hashtable = (mount_hash_bucket *)
		kmalloc_large (sizeof (mount_hash_bucket) * mount_hash_size);
	if (!mount_hashtable)
		panic ("Failed to create DEntry hash table!");

	for (uintptr_t i = 0; i < mount_hash_size; i++) {
		mount_hashtable[i].hlist.init ();
		mount_hashtable[i].lock.init ();
	}
}

Mount *
mnt_get (Mount *mnt)
{
	refcount_inc (&mnt->refcount);
	return mnt;
}

static spinlock_t mount_lock;

Mount *
do_mount_root (const char *fstype, const char *source,
		unsigned long mount_flags, const void *data)
{
	FilesystemType *typ = get_filesystem_type (fstype);
	if (!typ)
		panic ("Failed to mount root: No such filesystem type: %s", fstype);

	Mount *mount = (Mount *) slab_alloc (mount_slab_cache, ALLOC_KERNEL);
	if (!mount)
		panic ("Failed to mount root: Cannot allocate memory!");

	mount->root = nullptr;
	mount->fs = nullptr;
	mount->mountpoint.mount = nullptr;
	mount->mountpoint.dentry = nullptr;
	mount->flags = mount_flags | VFSMNT_ORPHAN | VFSMNT_DETACHED;
	mount->lock.init ();
	mount->refcount = 1;
	mount->child_mounts.init ();

	int errno = typ->mount_fs (source, mount_flags, typ, data,
			&mount->fs, &mount->root);
	if (errno != 0)
		panic ("Failed to mount root: errno %d\n", errno);

	mount_lock.lock_dpc ();
	mount->fs->num_mounts++;
	mount_lock.unlock_dpc ();

	return mount;
}

void
mnt_put (Mount *mnt)
{
	do {
		if (!refcount_dec (&mnt->refcount))
			return;
		dput (mnt->root);
		mount_lock.lock_dpc ();
		if (!--mnt->fs->num_mounts) {
			if (mnt->fs->ops->trim_fs)
				mnt->fs->ops->trim_fs (mnt->fs);
		}
		mount_lock.unlock_dpc ();
		fs_put (mnt->fs);
		Mount *next = nullptr;
		if (!(mnt->flags & VFSMNT_ORPHAN)) {
			dput (mnt->mountpoint.dentry);
			next = mnt->mountpoint.mount;
		}
		slab_free (mnt);
		mnt = next;
	} while (mnt);
}

