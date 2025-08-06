// SPDX-License-Identifier: GPL-3.0
/*
 * File: include/Acpi/setup.h
 * ACPI subsystem initialization.
 *
 * Copyright (C) 2025  dbstream
 */
#pragma once

void AcpiSetRSDPAddress(unsigned long phys_addr);

void AcpiInitializeTables(void);

