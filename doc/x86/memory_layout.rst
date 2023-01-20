.. SPDX-License-Identifier: MIT

=============
Memory layout
=============

Memory layout without LA57
==========================

.. csv-table:: Memory layout without LA57
	:header: "Start", "End", "Size", "Usage"

	"``0x0000000000000000``", "``0x00000000001fffff``", "2 MiB", "Unused hole"
	"``0x0000000000200000``", "``0x00007fffffffffff``", "~128 TiB", "Available to userspace"
	"``0x0000800000000000``", "``0xffff7fffffffffff``", "~16 EiB", "Architectural memory hole"
	"``0xffff800000000000``", "``0xffffbfffffffffff``", "64 TiB", "Direct mapping of all system RAM"
	"``0xffffc00000000000``", "``0xffffc0ffffffffff``", "1 TiB", "Page structs"
	"``0xffffc10000000000``", "``0xffffe0ffffffffff``", "32 TiB", "``vmap()`` space"
	"``0xffffe10000000000``", "``0xffffffff7fffffff``", "~31TiB", "Unused"
	"``0xffffffff80000000``", "``0xffffffffffffffff``", "2 GiB", "Shared with LA57 memory layout"

Memory layout with LA57
=======================

.. csv-table:: Memory layout with LA57
	:header: "Start", "End", "Size", "Usage"

	"``0x0000000000000000``", "``0x00000000001fffff``", "2 MiB", "Unused hole"
	"``0x0000000000200000``", "``0x00ffffffffffffff``", "~64 PiB", "Available to userspace"
	"``0x0100000000000000``", "``0xfeffffffffffffff``", "~16 EiB", "Architectural memory hole"
	"``0xff00000000000000``", "``0xff7fffffffffffff``", "32 PiB", "Direct mapping of all system RAM"
	"``0xff80000000000000``", "``0xff81ffffffffffff``", "512 TiB", "Page structs"
	"``0xff82000000000000``", "``0xffc1ffffffffffff``", "16 PiB", "``vmap()`` space"
	"``0xffc2000000000000``", "``0xffffffff7fffffff``", "~15.5PiB", "Unused"
	"``0xffffffff80000000``", "``0xffffffffffffffff``", "2 GiB", "Shared with non-LA57 memory layout"

Common fields
=============

.. csv-table:: Common fields
	:header: "Start", "End", "Size", "Usage"

	"``0xffffffff80000000``", "``0xffffffffffdfffff``", "~2 GiB", "Kernel & module mapping space"
	"``0xffffffffffe00000``", "``0xffffffffffffffff``", "2 MiB", "Unused hole"
