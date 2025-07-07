/**
 * Filesystem type table.
 * Copyright (C) 2025-present  dbstream
 */
#include <davix/fs_types.h>
#include <davix/fs.h>
#include <davix/kmalloc.h>
#include <davix/printk.h>
#include <davix/spinlock.h>
#include <string.h>

Filesystem *
new_filesystem (FilesystemType *fstype, void *fs_private,
		unsigned long mount_flags)
{
	(void) mount_flags;

	Filesystem *fs = (Filesystem *) kmalloc (sizeof (*fs), ALLOC_KERNEL);
	if (!fs)
		return nullptr;

	fs->ops = fstype;
	fs->refcount = 1;
	fs->fs_flags = 0;
	fs->num_mounts = 0;
	fs->fs_private.ptr = fs_private;
	fs->fs_dentries.init ();
	fs->dentry_list_lock.init ();
	return fs;
}

Filesystem *
fs_get (Filesystem *fs)
{
	refcount_inc (&fs->refcount);
	return fs;
}

void
fs_put (Filesystem *fs)
{
	if (refcount_dec (&fs->refcount)) {
		if (fs->ops->unmount_fs)
			fs->ops->unmount_fs (fs);
		kfree (fs);
	}
}

static dsl::TypedList<FilesystemType, &FilesystemType::fs_type_list> fs_type_list;
static spinlock_t fs_type_list_lock;

void
register_filesystem (FilesystemType *fs_type)
{
	fs_type_list_lock.lock_dpc ();
	fs_type_list.push_back (fs_type);
	fs_type_list_lock.unlock_dpc ();
	printk (PR_INFO "Registered filesystem driver: %s\n", fs_type->name);
}

FilesystemType *
get_filesystem_type (const char *name)
{
	fs_type_list_lock.lock_dpc ();
	for (FilesystemType *type : fs_type_list) {
		if (!strcmp (name, type->name)) {
			fs_type_list_lock.unlock_dpc ();
			return type;
		}
	}
	fs_type_list_lock.unlock_dpc ();
	printk (PR_WARN "get_filesystem_type: no such driver: %s\n", name);
	return nullptr;
}

