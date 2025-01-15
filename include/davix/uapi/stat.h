/**
 * Inode attributes.
 * Copyright (C) 2025-present  dbstream
 */
#ifndef _DAVIX_UAPI_STAT_H
#define _DAVIX_UAPI_STAT_H 1

#include <davix/uapi/timespec.h>
#include <davix/uapi/types.h>

struct stat {
	dev_t		st_dev;
	ino_t		st_ino;
	mode_t		st_mode;
	nlink_t		st_nlink;
	uid_t		st_uid;
	gid_t		st_gid;
	dev_t		st_rdev;
	off_t		st_size;
	blksize_t	st_blksize;
	blkcnt_t	st_blocks;

	struct timespec st_atim;	/** Time of last access  */
	struct timespec st_mtim;	/** Time of last modification  */
	struct timespec st_ctim;	/** Time of last status change  */
};

#define S_IFMT		0170000
#define S_IFDIR		0040000
#define S_IFCHR		0020000
#define S_IFBLK		0060000
#define S_IFREG		0100000
#define S_IFIFO		0010000
#define S_IFLNK		0120000
#define S_IFSOCK	0140000

#define S_ISUID		04000	/** setuid bit */
#define S_ISGID		02000	/** setgid bit */
#define S_ISVTX		01000	/** "sticky" bit */
#define S_IRUSR		0400
#define S_IWUSR		0200
#define S_IXUSR		0100
#define S_IRGRP		0040
#define S_IWGRP		0020
#define S_IXGRP		0010
#define S_IROTH		0004
#define S_IWOTH		0002
#define S_IXOTH		0001

#define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
#define S_ISCHR(mode) (((mode) & S_IFMT) == S_IFCHR)
#define S_ISBLK(mode) (((mode) & S_IFMT) == S_IFBLK)
#define S_ISREG(mode) (((mode) & S_IFMT) == S_IFREG)
#define S_ISFIFO(mode) (((mode) & S_IFMT) == S_IFIFO)
#define S_ISLNK(mode) (((mode) & S_IFMT) == S_IFLNK)
#define S_ISSOCK(mode) (((mode) & S_IFMT) == S_IFSOCK)

#endif /** _DAVIX_UAPI_STAT_H  */
