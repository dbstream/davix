/**
 * memdump:  a kernel debugging utility
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#include <stddef.h>
#include <davix/printk.h>

static void
memdump (unsigned char *mem, size_t nbytes)
{
	printk ("memdump(%p):\n", mem);
	while (nbytes >= 16) {
		printk ("  %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
			mem[0], mem[1], mem[2], mem[3], mem[4], mem[5], mem[6], mem[7],
			mem[8], mem[9], mem[10], mem[11], mem[12], mem[13], mem[14], mem[15]);
		mem += 16;
		nbytes -= 16;
	}
	switch (nbytes) {
	case 15:
		printk ("  %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
			mem[0], mem[1], mem[2], mem[3], mem[4], mem[5], mem[6], mem[7],
			mem[8], mem[9], mem[10], mem[11], mem[12], mem[13], mem[14]);
		break;
	case 14:
		printk ("  %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
			mem[0], mem[1], mem[2], mem[3], mem[4], mem[5], mem[6], mem[7],
			mem[8], mem[9], mem[10], mem[11], mem[12], mem[13]);
		break;
	case 13:
		printk ("  %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
			mem[0], mem[1], mem[2], mem[3], mem[4], mem[5], mem[6], mem[7],
			mem[8], mem[9], mem[10], mem[11], mem[12]);
		break;
	case 12:
		printk ("  %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
			mem[0], mem[1], mem[2], mem[3], mem[4], mem[5], mem[6], mem[7],
			mem[8], mem[9], mem[10], mem[11]);
		break;
	case 11:
		printk ("  %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
			mem[0], mem[1], mem[2], mem[3], mem[4], mem[5], mem[6], mem[7],
			mem[8], mem[9], mem[10]);
		break;
	case 10:
		printk ("  %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
			mem[0], mem[1], mem[2], mem[3], mem[4], mem[5], mem[6], mem[7],
			mem[8], mem[9]);
		break;
	case 9:
		printk ("  %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
			mem[0], mem[1], mem[2], mem[3], mem[4], mem[5], mem[6], mem[7],
			mem[8]);
		break;
	case 8:
		printk ("  %02x %02x %02x %02x %02x %02x %02x %02x\n",
			mem[0], mem[1], mem[2], mem[3], mem[4], mem[5], mem[6], mem[7]);
		break;
	case 7:
		printk ("  %02x %02x %02x %02x %02x %02x %02x\n",
			mem[0], mem[1], mem[2], mem[3], mem[4], mem[5], mem[6]);
		break;
	case 6:
		printk ("  %02x %02x %02x %02x %02x %02x\n",
			mem[0], mem[1], mem[2], mem[3], mem[4], mem[5]);
		break;
	case 5:
		printk ("  %02x %02x %02x %02x %02x\n",
			mem[0], mem[1], mem[2], mem[3], mem[4]);
		break;
	case 4:
		printk ("  %02x %02x %02x %02x\n",
			mem[0], mem[1], mem[2], mem[3]);
		break;
	case 3:
		printk ("  %02x %02x %02x\n",
			mem[0], mem[1], mem[2]);
		break;
	case 2:
		printk ("  %02x %02x\n",
			mem[0], mem[1]);
		break;
	case 1:
		printk ("  %02x\n",
			mem[0]);
		break;
	default:
		break;
	}
}
