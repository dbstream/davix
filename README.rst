.. SPDX-License-Identifier: MIT

======
README
======

Currently, the only supported architecture is x86. This document is tailored
towards it.

Building the kernel
===================

To build the kernel, simply run ``make`` from the command-line. This requires
having some tools installed. They are listed here:

* GNU Make. I use the GNU version, but other versions of ``make`` will probably
work too.

* GCC and Binutils for ``x86_64-elf``. They must be accessible as
``x86_64-elf-gcc``, ``x86_64-elf-objcopy``, etc.

* Sparse, the Linux syntax parser. This is an optional depencendy, used for
static analysis of the kernel. This is not needed when building the kernel, but
recommended for developers working on the kernel.

* m4 for generating arch/x86/loader/include_kernel.S.

If I forgot anything in this list of dependencies, report an issue.

Building the kernel on Windows
==============================

Just, dont...

It is probably possible to build the kernel from WSL (or WSL2), but it is
strongly recommended to work from a real Linux (or Unix-like) system when
building the kernel.

If you insist on using Windows (won't switch to Linux or dual-boot), you will
have to write your own script to run the kernel. This involves creating a kernel
image with a bootloader installed on it, copying it to the Windows host
filesystem and invoking QEMU on the host.

Building in a different location
================================

Simply run ``objtree=/path/to/build-directory make``.

Running the kernel in an emulator
=================================

The kernel ships a script that allows for running the kernel in an emulator by
simply typing ``./run`` on the command-line. This has its own set of additional
dependencies:

* GNU GRUB. This is the bootloader installed to the kernel image.

* xorriso. This program is a depencency of ``grub-mkrescue`` needed to write an
ISO file.

* QEMU.

If I forgot anything in this list of dependencies, report an issue.

Licenses
========

All kernel source files should have a SPDX license identifier field, similar to
this::

	/* SPDX-License-Identifier: <your favorite license> */
	#include /*...*/

Make sure that the license you apply to your code exists at the location
``Licenses/<your favorite license>``.

The preferred license for kernel code is the MIT license. However, other
licenses are acceptable, and sometimes necessary. For example, all code under
tools/kconfig is copied from the Linux kernel v6.0 (scripts/kconfig, with slight
modifications), and is under the GPL-2.0 license.
