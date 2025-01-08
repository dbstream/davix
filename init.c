/**
 * Init executable.
 * Copyright (C) 2025-present  dbstream
 *
 * This is passed as the boot module to the kernel.
 */

void
_start (void)
{
	/** note that we're alive...  */
	asm volatile ("int3" :: "a" (0xDEADBEEFDEADBEEF));

	/** ... and now sleep... */
	for (;;)
		__builtin_ia32_pause ();
}
