/**
 * errno.h - Error code definitions.
 * Copyright (C) 2025-present  dbstream
 */
#ifndef __UAPI_DAVIX_ERRNO_H
#define __UAPI_DAVIX_ERRNO_H 1

#define ENOENT		2	/* No such file or directory */
#define EINTR		4	/* Interrupted system call */
#define ENOMEM		12	/* Cannot allocate memory */
#define EEXIST		17	/* File exists */
#define ENOTDIR		20	/* Not a directory*/
#define EISDIR		21	/* Is a directory */
#define EINVAL		22	/* Invalid argument */
#define ENAMETOOLONG	36	/* File name too long */
#define ENOTEMPTY	39	/* Directory not empty */
#define ETIME		62	/* Timer expired */
#define EOPNOTSUPP	95	/* Operation not supported */

#endif /* __UAPI_DAVIX_ERRNO_H */
