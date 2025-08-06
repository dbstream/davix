// SPDX-License-Identifier: GPL-3.0
/*
 * File: acpi/tables.cc
 * ACPI Table management and initialization.
 *
 * Copyright (C) 2025  dbstream
 */
#include <Acpi/setup.h>
#include <Ke/log.h>
#include <uacpi/uacpi.h>

void AcpiInitializeTables(void)
{
	KePrintf("ACPI: initializing table access\n");
	uacpi_status status = uacpi_initialize(UACPI_FLAG_NO_ACPI_MODE);
	if (status != UACPI_STATUS_OK)
		KePanic("ACPI: uacpi_initialize returned %d", (int) status);
}

