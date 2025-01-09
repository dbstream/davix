/**
 * Init executable.
 * Copyright (C) 2025-present  dbstream
 *
 * This is passed as the boot module to the kernel.
 */
#include <asm/syscall_nr.h>

void
_start (void)
{
	/** note that we're alive...  */
	asm volatile ("syscall" :: "a" (__SYS_write_dmesg),
			"D" ("Hello world from userspace!\n"),
			"S" (28)
			: "rdx", "r10", "r8", "r9", "cc", "rcx", "r11");

	/** ... and now sleep... */
	asm volatile ("syscall" :: "a" (__SYS_exit),
			"D" (69)
			: "rsi", "rdx", "r10", "r8", "r9", "cc", "rcx", "r11");
}
