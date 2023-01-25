/* SPDX-License-Identifier: MIT */
#ifndef __ACPI_PLATFORM_ACDAVIX_H
#define __ACPI_PLATFORM_ACDAVIX_H

#include <davix/ctype.h>
#include <davix/types.h>
#include <davix/semaphore.h>
#include <davix/spinlock.h>
#include <davix/kmalloc.h>

#define ACPI_MACHINE_WIDTH 64
#define ACPI_USE_NATIVE_MATH64
#define ACPI_COMPILER_DEPENDENT_INT64 i64
#define ACPI_COMPILER_DEPENDENT_UINT64 u64

#define ACPI_CACHE_T acpi_memory_list
#define ACPI_USE_LOCAL_CACHE 1

#define acpi_spinlock spinlock_t *
#define acpi_semaphore struct semaphore *

#define ACPI_MUTEX_TYPE ACPI_BINARY_SEMAPHORE

#define acpi_cpu_flags int

#define acpi_uintptr_t u64

#define ACPI_USE_DO_WHILE_0

#define ACPI_DEBUG_OUTPUT
#define ACPI_USE_SYSTEM_CLIBRARY
#define ACPI_USE_SYSTEM_INTTYPES

#define ACPI_USE_ALTERNATE_PROTOTYPE_acpi_os_initialize
#define ACPI_USE_ALTERNATE_PROTOTYPE_acpi_os_terminate
#define ACPI_USE_ALTERNATE_PROTOTYPE_acpi_os_allocate
#define ACPI_USE_ALTERNATE_PROTOTYPE_acpi_os_free

#endif /* __ACPI_PLATFORM_ACDAVIX_H */
