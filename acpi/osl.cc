/**
 * uACPI operative system specific layer.
 * Copyright (C) 2025-present  dbstream
 */
#include <davix/acpisetup.h>
#include <davix/printk.h>
#include <uacpi/kernel_api.h>

static bool rsdp_valid = false;
static uintptr_t rsdp = 0;

void
acpi_set_rsdp (uintptr_t addr)
{
	rsdp = addr;
	rsdp_valid = true;
	printk (PR_INFO "acpi: set RSDP address to %#tx\n", addr);
}

extern "C"
uacpi_status
uacpi_kernel_get_rsdp (uintptr_t *out)
{
	if (!rsdp_valid)
		return UACPI_STATUS_NOT_FOUND;

	*out = rsdp;
	return UACPI_STATUS_OK;
}

extern "C"
void
uacpi_kernel_log (uacpi_log_level level, const char *msg)
{
	switch (level) {
	case UACPI_LOG_INFO:
		printk (PR_NOTICE "uACPI: %s", msg);
		return;
	case UACPI_LOG_WARN:
		printk (PR_WARN "uACPI: %s", msg);
		return;
	case UACPI_LOG_ERROR:
		printk (PR_ERROR "uACPI: %s", msg);
		return;
	default:
		return;
	}
}
