/**
 * struct file
 * Copyright (C) 2025-present  dbstream
 */
#ifndef _DAVIX_FILE_H
#define _DAVIX_FILE_H 1

#include <davix/errno.h>
#include <davix/refcount.h>
#include <davix/stddef.h>
#include <davix/sys_types.h>

struct file_ops;
struct vm_area;
struct vm_pgtable_op;

struct file {
	refcount_t refcount;
	const struct file_ops *ops;
	void *private;
};

struct file_ops {
	/**
	 * Memory map a region of a file.
	 *
	 * @file	struct file
	 * @op		vm_pgtable_op, should be passed to vm functions
	 * @vma		vm_area that is being mapped. ops->mmap should store its
	 *		vm_area_operations in vma->ops and reference itself.
	 * @start	Start of the region being mapped.
	 * @end		End of the region being mapped.
	 * @offset	File offset being mapped.
	 *
	 * BIG FAT WARNING: drivers cannot memcpy() to the mapped region,
	 * because vm_pgtable_op doesn't insert the memory into the memory map
	 * until after this function has finished executing.
	 */
	errno_t (*mmap) (struct file *file, struct vm_pgtable_op *op, struct vm_area *vma,
			unsigned long start, unsigned long end, off_t offset);

	/**
	 * Read from a file at a particular offset.
	 *
	 * @file	struct file
	 * @buf		userspace buffer, already checked with usercopy_ok
	 * @size	size of userspace buffer, already check with usercopy_ok
	 * @offset	offset from which to read
	 * @out		storage for the number of bytes read
	 */
	errno_t (*pread) (struct file *file, void *buf,
			size_t size, off_t offset, ssize_t *out);
};

/**
 * Read a file from kernelspace.
 *
 * @file	file to read from
 * @buf		kernel buffer to read into
 * @size	number of bytes to read
 * @offset	offset to read from
 * @out		storage for the number of bytes read
 *
 * NOTE: the purpose of this function is to be used by execve().
 */
extern errno_t
kernel_read_file (struct file *file, void *buf,
		size_t size, off_t offset, ssize_t *out);

#endif /** _DAVIX_FILE_H */
