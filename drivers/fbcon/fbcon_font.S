/**
 * .incbin stub for VGA9.F16
 * Copyright (C) 2025-present  dbstream
 */
	.file "fbcon_font.S"

#include <asm/sections.h>

	__RODATA
	.globl VGA9_F16
	.type VGA9_F16, @object
	.balign 16
VGA9_F16:	.incbin "drivers/fbcon/VGA9.F16"
	.size VGA9_F16, . - VGA9_F16

