// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/Acpi/tables.h
 * ACPI table routines.
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

#include <Ke/log.h>
#include <uacpi/acpi.h>
#include <uacpi/tables.h>

/**
 * AcpiGetTable - lookup an ACPI table.
 * @signature: table signature
 * @out: location where a pointer to the table is stored
 * @id: location where a value to be passed to AcpiPutTable is stored
 * Returns true if the table is found.
 */
static inline bool AcpiGetTable(const char *signature, void **out, size_t *id)
{
	uacpi_table table;
	uacpi_status status = uacpi_table_find_by_signature(signature, &table);

	if (status != UACPI_STATUS_OK) {
		if (status != UACPI_STATUS_NOT_FOUND)
			KePrintf("AcpiGetTable: uacpi_table_find_by_signature returned %d\n",
					(int) status);
		return false;
	}

	*out = table.ptr;
	*id = table.index;
	return true;
}

/**
 * AcpiPutTable - release an ACPI table.
 * @ptr: pointer to table header
 * @id: out_id value written by AcpiGetTable
 */
static inline void AcpiPutTable(void *ptr, size_t id)
{
	uacpi_table table = {
		.ptr = ptr,
		.index = id
	};

	uacpi_table_unref(&table);
}

