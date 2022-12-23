.. SPDX-License-Identifier: MIT

=============
Boot protocol
=============

The kernel is generally split into two parts, the main kernel binary
(``mainkernel``), and the kernel loader (what eventually becomes ``davix``).

Currently, the kernel loader (in ``arch/x86/loader``) only supports booting
with the ``multiboot2`` protocol. This document is intended as a guide for
those seeking to implement other boot protocols.

``struct boot_struct``
======================

At address ``0xffffffff80000000`` in the kernel there will be a
``struct boot_struct``, which contains information about the kernel and space
for parameters which are passed to the kernel. For more information, see
``arch/x86/include/asm/boot.h``.

Page tables
===========

``%cr3`` will point to valid page tables. If ``boot_struct.l5_paging_enable`` is
set, ``CR4[LA57]`` will be enabled and all usable RAM available to the
kernel will be mapped at ``0xff00000000000000``. Otherwise, ``CR4[LA57]``
will be disabled and all usable RAM available to the kernel will be mapped at
``0xffff8000000000000``.

The kernel itself will be mapped according to ``phdr->vaddr`` and ``phdr->flags``
of each ``PT_LOAD`` kernel segment. If the NX bit is supported, the direct map
and non-exec segments will be mapped with it.

The root-level page table will occupy ``MEMMAP_KERNEL`` memory, and all
higher-half page tables will occupy ``MEMMAP_KERNEL`` memory. The state of
lower-half memory is undefined, to allow the bootloader to place itself and
the kernel trampoline there.

It is guaranteed that all the higher-half page table entries will be zero (not
present) upon kernel entry, unless they map kernel segments or the higher-half
direct map. The kernel segments will be mapped as 4KiB pages. The higher-half
direct map may be mapped as 4KiB, 2MiB or 1GiB pages, but it is guaranteed that
only complete pages of cacheable RAM will be mapped by the higher-half direct
map.

Physical memory
===============

The number of physical memory bits addressable by the kernel is limited to the
following expression::

	min(l5_paging_enable ? 52 : 45, phys_addr_width_supported_by_the_cpu)

Any physical memory entries above this hard limit must be ommitted from the
memory map provided to the kernel.

The following memory types are defined to be "usable RAM" for the purposes of
the higher-half direct map:

* ``MEMMAP_USABLE_RAM``
* ``MEMMAP_LOADER``
* ``MEMMAP_KERNEL``
* ``MEMMAP_ACPI_DATA``

All whole pages of memory with one (or more) of these types must be mapped in
the higher-half direct map. The kernel will discard partial pages.
