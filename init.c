/**
 * Init executable.
 * Copyright (C) 2025-present  dbstream
 *
 * This is passed as the boot module to the kernel.
 */
#include <davix/uapi/openat.h>
#include <davix/uapi/reboot.h>
#include <davix/uapi/stat.h>
#include <asm/syscall_nr.h>

/**
 * memcpy, memset, and memmove are implemented by the arch, not util/string.c.
 * This means we need to implement them here, as we cannot depend on string.c to
 * provide them.  :-(
 */

#undef memcpy
#undef memset
#undef memmove

void *
memcpy (void *dst, const void *src, unsigned long n)
{
	char *d = dst;
	const char *s = src;
	for (; n; n--)
		*(d++) = *(s++);
	return dst;
}

void *
memset (void *dst, int x, unsigned long n)
{
	char *d = (char *) dst;
	for (; n; n--)
		*(d++) = x;
	return dst;
}

void *
memmove (void *dst, const void *src, unsigned long n)
{
	if ((unsigned long) dst <= (unsigned long) src)
		return memcpy (dst, src, n);

	char *d = (char *) dst;
	char *s = (char *) src;
	while (n--)
		d[n] = s[n];
	return dst;
}

#include "util/snprintf.c"
#include "util/string.c"

static void
dmesg (const char *fmt, ...)
{
	char buf[128];

	va_list ap;
	va_start (ap, fmt);
	vsnprintf (buf, sizeof (buf), fmt, ap);
	va_end (ap);

	asm volatile ("syscall" :: "a" (__SYS_write_dmesg),
			"D" (buf),
			"S" (strnlen (buf, sizeof (buf)))
			: "rdx", "r10", "r8", "r9", "cc", "rcx", "r11");
}

static int
stat (struct stat *buf, size_t buf_size, const char *path,
		int dirfd, struct pathwalk_info *pwinfo, size_t pwinfo_size)
{
	register long r10 asm("r10") = dirfd;
	register long r8 asm("r8") = (long) pwinfo;
	register long r9 asm("r9") = pwinfo_size;

	int ret;
	asm volatile ("syscall" : "=a" (ret) : "a" (__SYS_stat),
			"D" (buf), "S" (buf_size), "d" (path),
			"r" (r10), "r" (r8), "r" (r9)
			: "cc", "rcx", "r11");
	return ret;
}

static int
mknod (const char *path, int dirfd,
		struct pathwalk_info *pwinfo, size_t pwinfo_size,
		mode_t mode, dev_t dev)
{
	register long r10 asm ("r10") = pwinfo_size;
	register long r8 asm ("r8") = mode;
	register long r9 asm ("r9") = dev;

	int ret;
	asm volatile ("syscall" : "=a" (ret) : "a" (__SYS_mknod),
			"D" (path), "S" (dirfd), "d" (pwinfo),
			"r" (r10), "r" (r8), "r" (r9)
			: "cc", "rcx", "r11");
	return ret;
}

static int
unlink (const char *path, int dirfd,
		struct pathwalk_info *pwinfo, size_t pwinfo_size)
{
	register long r10 asm ("r10") = pwinfo_size;

	int ret;
	asm volatile ("syscall" : "=a" (ret) : "a" (__SYS_unlink),
			"D" (path), "S" (dirfd), "d" (pwinfo),
			"r" (r10)
			: "cc", "r8", "r9", "rcx", "r11");
	return ret;
}

static void
show_stat (const char *path)
{
	struct stat buf;
	int ret = stat (&buf, sizeof (buf), path, AT_FDCWD, NULL, 0);
	dmesg ("stat (%s) = %d\n", path, ret);
	if (ret)
		return;

	if (buf.st_valid & STAT_ATTR_NLINK)	dmesg ("st_nlink=%llu\n", buf.st_nlink);
	if (buf.st_valid & STAT_ATTR_MODE)	dmesg ("st_mode=%o\n", buf.st_mode);
	if (buf.st_valid & STAT_ATTR_UID)	dmesg ("st_uid=%u\n", buf.st_uid);
	if (buf.st_valid & STAT_ATTR_GID)	dmesg ("st_gid=%u\n", buf.st_gid);
	if (buf.st_valid & STAT_ATTR_DEV)	dmesg ("st_dev=%llu\n", buf.st_dev);
}

static inline int
reboot (int cmd, int arg)
{
	register long r10 asm ("r10") = DAVIX_REBOOT_MAG1;
	int ret;
	asm volatile ("syscall" : "=a" (ret) : "a" (__SYS_reboot),
			"D" (cmd), "S" (arg),
			"d" (DAVIX_REBOOT_MAG0), "r" (r10)
			: "r8", "r9", "cc", "rcx", "r11");
	return ret;
}

void
_start (void)
{
	/** note that we're alive...  */
	dmesg ("\"%s\" from userspace!\n", "hello world");

	if (1) goto exit;

	/** test the VFS... */
	show_stat ("/");
	show_stat ("/foo");

	struct pathwalk_info pwinfo = {
		.flags = O_FILETYPE,
		.mode = S_IFREG
	};

	for (int i = 0; i < 2; i++) {
		dmesg ("mknod (/foo) = %d\n",
				mknod ("/foo", AT_FDROOT, NULL, 0, S_IFREG | 0644, 0));
		show_stat ("/");
		show_stat ("/foo");
		dmesg ("unlink (/foo) = %d\n",
				unlink ("/foo", AT_FDCWD, &pwinfo, sizeof (pwinfo)));
		show_stat ("/");
		show_stat ("/foo");
	}

	/** ... and now sleep... */
#if 0	/* Enable this code to test SYS_reboot shutdown.  */
	dmesg ("SYS_reboot(REBOOT_CMD_POWEROFF) returned %d\n",
			reboot (REBOOT_CMD_POWEROFF, 0));
#endif
exit:
	asm volatile ("syscall" :: "a" (__SYS_exit),
			"D" (0)
			: "rsi", "rdx", "r10", "r8", "r9", "cc", "rcx", "r11");
}
