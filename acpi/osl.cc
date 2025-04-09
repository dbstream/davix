/**
 * uACPI operative system specific layer.
 * Copyright (C) 2025-present  dbstream
 */
#include <davix/printk.h>
#include <uacpi/kernel_api.h>

extern "C"
uacpi_status
uacpi_kernel_get_rsdp (uintptr_t *out)
{
	(void) out;
	return UACPI_STATUS_UNIMPLEMENTED;
}

extern "C"
void *
uacpi_kernel_map (uintptr_t addr, size_t len)
{
	(void) addr;
	(void) len;
	return nullptr;
}

extern "C"
void
uacpi_kernel_unmap (void *addr, size_t len)
{
	(void) addr;
	(void) len;
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
