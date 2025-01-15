/**
 * Types for kernel<->userspace interfaces.
 * Copyright (C) 2025-present  dbstream
 */
#ifndef _DAVIX_UAPI_TYPES_H
#define _DAVIX_UAPI_TYPES_H 1

/**
 * dev_t is used to describe device IDs.  It is an integer type.
 */
typedef unsigned long long dev_t;

/**
 * ino_t is used in struct stat to describe inode numbers.  It is an unsigned
 * integer type.
 */
typedef unsigned long long ino_t;

/**
 * mode_t is used for some file attributes (e.g., file mode).  It is an integer
 * type.
 */
typedef unsigned int mode_t;

/**
 * nlink_t is used to describe the number of hard links to a file.  It is an
 * unsigned integer type.
 */
typedef unsigned long long nlink_t;

/**
 * {uid_t, gid_t} is used to describe user and group IDs.  It is an integer
 * type.
 */
typedef unsigned int uid_t;
typedef unsigned int gid_t;

/**
 * pid_t is a type used for storing process IDs, process group IDs, and session
 * IDs.  It is a signed integer type.
 */
typedef int pid_t;

/**
 * off_t is used for describing file sizes.  It is a signed integer type.
 */
typedef long long off_t;

/**
 * blksize_t is a type used for storing filesystem block sizes.  It is a signed
 * integer type.
 */
typedef long long blksize_t;

/**
 * blkcnt_t is used for file block counts.  It is a signed integer type.
 */
typedef long long blkcnt_t;

/**
 * time_t is used for time in seconds.  It is an integer type.
 */
typedef long long time_t;

#endif /**  _DAVIX_UAPI_TYPES_H   */
