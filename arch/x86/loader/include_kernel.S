/* SPDX-License-Identifier: MIT */

.balign 16
.globl mainkernel
.type mainkernel, @object
mainkernel:

	.incbin PATH_TO_MAINKERNEL

.size mainkernel, . - mainkernel

.globl mainkernel_end
.type mainkernel_end, @object
mainkernel_end:
	/*
	 * Use some space so we don't accidentally get
	 * two symbols with the same value
	 */
	.byte 0
