/**
 * Architecture-specific definitions for ACPI.
 * Copyright (C) 2025-present  dbstream
 */
#include <asm/kmap_fixed.h>
#include <asm/pgtable.h>
#include <uacpi/kernel_api.h>
#include <davix/printk.h>

extern "C"
void *
uacpi_kernel_map (uintptr_t addr, size_t len)
{
	return kmap_fixed (addr, len, make_pte_k (0, true, true, false).value);
}

extern "C"
void
uacpi_kernel_unmap (void *addr, size_t len)
{
	(void) len;
	kunmap_fixed (addr);
}
