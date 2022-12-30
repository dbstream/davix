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

The kernel must leave sensible defaults in ``struct boot_struct``, such as
``NULL`` for ``boot_struct->acpi_rsdp``.

Page tables
===========

``%cr3`` will point to valid page tables. If ``boot_struct.l5_paging_enable`` is
set, ``CR4[LA57]`` is enabled and the HHDM is located at virtual address
``0xff00000000000000``. Otherwise, ``CR4[LA57]`` is disabled and the HHDM is
located at virtual address ``0xffff8000000000000``.

All higher-half mappings will be mapped global. No other global mappings will
exist, neither in the lower-half page tables nor the TLB.

The kernel itself will be mapped according to ``phdr->vaddr`` and ``phdr->flags``
of each ``PT_LOAD`` kernel segment. If the NX bit is supported, the direct map
and non-exec segments will be mapped with it.

The pages for the page tables will be marked ``MEMMAP_KERNEL`` in the memory map
provided to the kernel, with the exception of lower-half page tables that should
be marked ``MEMMAP_LOADER``.

The higher-half direct map is guaranteed to contain all whole pages of physical
memory marked one of ``MEMMAP_USABLE_RAM``, ``MEMMAP_ACPI_DATA``,
``MEMMAP_LOADER`` and ``MEMMAP_KERNEL``, and nothing else.

Physical memory limitations
===========================

The number of physical memory bits addressable by the kernel is limited to the
following expression::

	min(l5_paging_enable ? 52 : 45, phys_addr_width_supported_by_the_cpu)

Any physical memory entries above this hard limit must be ommitted from the
memory map provided to the kernel.
