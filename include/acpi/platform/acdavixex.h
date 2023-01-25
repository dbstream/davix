/* SPDX-License-Identifie: MIT */
#ifndef __ACPI_PLATFORM_ACDAVIXEX_H
#define __ACPI_PLATFORM_ACDAVIXEX_H

#include <acpi/acexcep.h>

static inline acpi_status acpi_os_initialize(void)
{
	return AE_OK;
}

static inline acpi_status acpi_os_terminate(void)
{
	return AE_OK;
}

#define acpi_os_allocate kmalloc
#define acpi_os_free kfree

#endif /* __ACPI_PLATFORM_ACDAVIXEX_H */
