/**
 * Operations on struct file
 * Copyright (C) 2025-present  dbstream
 */
#include <davix/file.h>

errno_t
kernel_read_file (struct file *file, void *buf,
		size_t size, off_t offset, ssize_t *out)
{
	if (!file->ops->pread)
		return ENOTSUP;

	return file->ops->pread (file, buf, size, offset, out);
}
