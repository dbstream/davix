/**
 * Operations on struct file
 * Copyright (C) 2025-present  dbstream
 */
#include <davix/file.h>

void
__fput (struct file *file)
{
	if (file->ops->close)
		file->ops->close (file);

	// TODO: free the 'struct file'
}

errno_t
kernel_read_file (struct file *file, void *buf,
		size_t size, off_t offset, ssize_t *out)
{
	if (!file->ops->pread)
		return ENOTSUP;

	return file->ops->pread (file, buf, size, offset, out);
}
