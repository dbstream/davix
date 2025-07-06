/**
 * stat.h - definitions for stat(), fstat().
 * Copyright (C) 2025-present  dbstream
 */
#ifndef __UAPI_DAVIX_STAT_H
#define __UAPI_DAVIX_STAT_H 1

/*
 * Definitions for st_mode fields.
 */

#define __S_IFMT		0170000

/* File types. */
#define __S_IFSOCK		0140000
#define __S_IFLNK		0120000
#define __S_IFREG		0100000
#define __S_IFBLK		0060000
#define __S_IFDIR		0040000
#define __S_IFCHR		0020000
#define __S_IFIFO		0010000

/* File permission bits. */
#define __S_ISUID		04000
#define __S_ISGID		02000
#define __S_ISVTX		01000
#define __S_IRWXU		00700
#define __S_IRUSR		00400
#define __S_IWUSR		00200
#define __S_IXUSR		00100
#define __S_IRWXG		00070
#define __S_IRGRP		00040
#define __S_IWGRP		00020
#define __S_IXGRP		00010
#define __S_IRWXO		00007
#define __S_IROTH		00004
#define __S_IWOTH		00002
#define __S_IXOTH		00001

#define __S_ISSOCK(__m) (__S_IFSOCK == (__S_IFMT & (__m)))
#define __S_ISLNK(__m) (__S_IFLNK == (__S_IFMT & (__m)))
#define __S_ISREG(__m) (__S_IFREG == (__S_IFMT & (__m)))
#define __S_ISBLK(__m) (__S_IFBLK == (__S_IFMT & (__m)))
#define __S_ISDIR(__m) (__S_IFDIR == (__S_IFMT & (__m)))
#define __S_ISCHR(__m) (__S_IFCHR == (__S_IFMT & (__m)))
#define __S_ISFIFO(__m) (__S_IFIFO == (__S_IFMT & (__m)))

#endif /* __UAPI_DAVIX_STAT_H */

