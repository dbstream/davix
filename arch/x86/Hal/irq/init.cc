// SPDX-License-Identifier: GPL-3.0
/*
 * File: Hal/irq/init.cc
 * HAL IRQ subsystem initialization.
 *
 * Copyright (C) 2025  dbstream
 */
#include <Acpi/tables.h>
#include <Hal/irq_vectors.h>
#include <Ke/log.h>
#include <asm/io.h>
#include "internal.h"

static acpi_madt *AcpiMADT;

static void find_madt(void)
{
	void *table_ptr;
	size_t table_index;
	bool found = AcpiGetTable(ACPI_MADT_SIGNATURE, &table_ptr, &table_index);
	if (!found)
		KePanic("HalInitializeIRQSubsystem: no ACPI MADT table!");

	AcpiMADT = (acpi_madt *) table_ptr;
}

static inline void io_wait(void)
{
	io_outb(0x80, 0);
}

/**
 * disable_8259_pic - Remap the 8259A Programmable Interrupt Controller to where
 * it doesn't collide with us, and mask all of its interrupts.
 */
static void disable_8259_pic(void)
{
	unsigned int i8259A_offset1 = IRQ_VECTOR_8259_OFFSET + 0;
	unsigned int i8259A_offset2 = IRQ_VECTOR_8259_OFFSET + 8;

	io_outb(0x20, 0x11);
	io_wait();
	io_outb(0xa0, 0x11);
	io_wait();
	io_outb(0x21, i8259A_offset1);
	io_wait();
	io_outb(0xa1, i8259A_offset2);
	io_wait();
	io_outb(0x21, 0x04);
	io_wait();
	io_outb(0xa1, 0x02);
	io_wait();
	io_outb(0x21, 0x01);
	io_wait();
	io_outb(0xa1, 0x01);
	io_wait();
	io_outb(0x21, 0xff);
	io_outb(0xa1, 0xff);
	io_wait();
}

void HalInitializeIRQSubsystem(void)
{
	find_madt();

	if (AcpiMADT->flags & ACPI_PCAT_COMPAT)
		KePrintf("Hal: ACPI MADT has PCAT_COMPAT set; disabling the PIC.\n");
	else
		KePrintf("Hal: ACPI MADT does not have PCAT_COMPAT set; disabling the PIC anyways.\n");

	disable_8259_pic();
}

