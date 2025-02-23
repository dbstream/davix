/**
 * uACPI->kernel interface.
 * Copyright (C) 2025-present  dbstream
 *
 * For now, this is mainly here for shits and giggles and a leaderboard spot on
 * the uACPI kernel benchmark.  Most stuff that would be needed to run on real
 * hardware is unimplemented.
 */

#include <acpi/acpi.h>
#include <davix/interrupt.h>
#include <davix/kmalloc.h>
#include <davix/mutex.h>
#include <davix/panic.h>
#include <davix/printk.h>
#include <davix/semaphore.h>
#include <davix/spinlock.h>
#include <davix/time.h>
#include <davix/vmap.h>
#include <asm/io.h>
#include <asm/task.h>

#include <uacpi/kernel_api.h>

void
uacpi_kernel_log (uacpi_log_level level, const uacpi_char *msg)
{
	switch (level) {
	case UACPI_LOG_DEBUG:
	case UACPI_LOG_TRACE:
		return;
	case UACPI_LOG_INFO:
		printk (PR_INFO "uACPI: %s", msg);
		break;
	case UACPI_LOG_WARN:
		printk (PR_WARN "uACPI: %s", msg);
		break;
	case UACPI_LOG_ERROR:
		printk (PR_ERR "uACPI: %s", msg);
		break;
	}
}

uacpi_status
uacpi_kernel_get_rsdp (uacpi_phys_addr *out)
{
	if (!acpi_rsdp_phys_for_uacpi)
		return UACPI_STATUS_NOT_FOUND;

	*out = acpi_rsdp_phys_for_uacpi;
	return UACPI_STATUS_OK;
}

uacpi_status
uacpi_kernel_pci_device_open (uacpi_pci_address device, uacpi_handle *out)
{
	(void) device;
	(void) out;

	return UACPI_STATUS_UNIMPLEMENTED;
}

void
uacpi_kernel_pci_device_close (uacpi_handle handle)
{
	(void) handle;
}

uacpi_status
uacpi_kernel_pci_read8 (uacpi_handle device, uacpi_size offset, uacpi_u8 *value)
{
	(void) device;
	(void) offset;
	(void) value;

	return UACPI_STATUS_UNIMPLEMENTED;
}

uacpi_status
uacpi_kernel_pci_read16 (uacpi_handle device, uacpi_size offset, uacpi_u16 *value)
{
	(void) device;
	(void) offset;
	(void) value;

	return UACPI_STATUS_UNIMPLEMENTED;
}

uacpi_status
uacpi_kernel_pci_read32 (uacpi_handle device, uacpi_size offset, uacpi_u32 *value)
{
	(void) device;
	(void) offset;
	(void) value;

	return UACPI_STATUS_UNIMPLEMENTED;
}

uacpi_status
uacpi_kernel_pci_write8 (uacpi_handle device, uacpi_size offset, uacpi_u8 value)
{
	(void) device;
	(void) offset;
	(void) value;

	return UACPI_STATUS_UNIMPLEMENTED;
}

uacpi_status
uacpi_kernel_pci_write16 (uacpi_handle device, uacpi_size offset, uacpi_u16 value)
{
	(void) device;
	(void) offset;
	(void) value;

	return UACPI_STATUS_UNIMPLEMENTED;
}

uacpi_status
uacpi_kernel_pci_write32 (uacpi_handle device, uacpi_size offset, uacpi_u32 value)
{
	(void) device;
	(void) offset;
	(void) value;

        return UACPI_STATUS_UNIMPLEMENTED;
}

uacpi_status
uacpi_kernel_io_map(uacpi_io_addr base, uacpi_size len, uacpi_handle *out)
{
	(void) base;
	(void) len;

	*out = (void *) base;
	return UACPI_STATUS_OK;
}

void
uacpi_kernel_io_unmap(uacpi_handle handle)
{
	(void) handle;
}

uacpi_status
uacpi_kernel_io_read8 (uacpi_handle base, uacpi_size offset, uacpi_u8 *out)
{
	*out = io_inb ((unsigned long) base + offset);
	return UACPI_STATUS_OK;
}

uacpi_status
uacpi_kernel_io_read16 (uacpi_handle base, uacpi_size offset, uacpi_u16 *out)
{
	*out = io_inw ((unsigned long) base + offset);
	return UACPI_STATUS_OK;
}

uacpi_status
uacpi_kernel_io_read32 (uacpi_handle base, uacpi_size offset, uacpi_u32 *out)
{
	*out = io_inl ((unsigned long) base + offset);
	return UACPI_STATUS_OK;
}

uacpi_status
uacpi_kernel_io_write8 (uacpi_handle base, uacpi_size offset, uacpi_u8 value)
{
	io_outb ((unsigned long) base + offset, value);
	return UACPI_STATUS_OK;
}

uacpi_status
uacpi_kernel_io_write16 (uacpi_handle base, uacpi_size offset, uacpi_u16 value)
{
	io_outw ((unsigned long) base + offset, value);
	return UACPI_STATUS_OK;
}

uacpi_status
uacpi_kernel_io_write32 (uacpi_handle base, uacpi_size offset, uacpi_u32 value)
{
	io_outl ((unsigned long) base + offset, value);
	return UACPI_STATUS_OK;
}

void *
uacpi_kernel_map (uacpi_phys_addr addr, uacpi_size len)
{
	/**
	 * FIXME: now we map everything as write-back, which is fine on x86
	 * since the MTRRs correctly deal with caching modes.  But ideally we
	 * only want to rely on ourselves for this.
	 */
	return vmap_phys ("uacpi_kernel_map", addr, len,
			PAGE_KERNEL_DATA /* | PG_UC_MINUS */);
}

void
uacpi_kernel_unmap(void *addr, uacpi_size len)
{
	(void) len;
	vunmap (addr);
}

void *
uacpi_kernel_alloc (uacpi_size size)
{
	return kmalloc (size);
}

void
uacpi_kernel_free (void *mem)
{
	kfree (mem);
}

uacpi_u64
uacpi_kernel_get_nanoseconds_since_boot (void)
{
	return ns_since_boot ();
}

void
uacpi_kernel_stall (uacpi_u8 usec)
{
	if (usec)
		panic ("uacpi_kernel_stall is unimplemented  :/");
}

void
uacpi_kernel_sleep (uacpi_u64 msec)
{
	if (msec)
		panic ("uacpi_ekrnel_sleep is unimplemented  :/");
}

uacpi_handle
uacpi_kernel_create_mutex (void)
{
	struct mutex *mtx = kmalloc (sizeof (*mtx));
	if (mtx)
		mutex_init (mtx);
	return mtx;
}

void
uacpi_kernel_free_mutex (uacpi_handle mtx)
{
	kfree (mtx);
}

/**
 * TODO: implement semaphores.
 */

uacpi_handle
uacpi_kernel_create_event (void)
{
	struct semaphore *sema = kmalloc (sizeof (*sema));
	if (sema)
		sema_init (sema);
	return sema;
}

void
uacpi_kernel_free_event (uacpi_handle sema)
{
	kfree (sema);
}

uacpi_thread_id
uacpi_kernel_get_thread_id (void)
{
	return get_current_task ();
}

uacpi_status
uacpi_kernel_acquire_mutex (uacpi_handle mtx, uacpi_u16 timeout)
{
	usecs_t us;
	if (timeout == 0xffff)
		us = UINT64_MAX;
	else
		us = 1000 * timeout;
	errno_t e = mutex_lock_timeout (mtx, us);
	return e == ESUCCESS ? UACPI_STATUS_OK : UACPI_STATUS_TIMEOUT;
}

void
uacpi_kernel_release_mutex(uacpi_handle mtx)
{
	mutex_unlock (mtx);
}

uacpi_bool uacpi_kernel_wait_for_event(uacpi_handle sema, uacpi_u16 timeout)
{
	usecs_t us;
	if (timeout == 0xffff)
		us = UINT64_MAX;
	else
		us = 1000 * timeout;
	errno_t e = sema_lock_timeout (sema, us);
	return e == ESUCCESS ? true : false;
}

void
uacpi_kernel_signal_event (uacpi_handle sema)
{
	sema_unlock (sema);
}

void
uacpi_kernel_reset_event (uacpi_handle sema)
{
	while (sema_lock_timeout (sema, 0) == ESUCCESS)
		;
}

uacpi_status
uacpi_kernel_handle_firmware_request (uacpi_firmware_request *req)
{
	(void) req;

	return UACPI_STATUS_UNIMPLEMENTED;
}

static irqstatus_t
handle_uacpi_interrupt (struct interrupt_handler *h)
{
	uacpi_interrupt_handler fn = h->ptr1;
	uacpi_handle ctx = h->ptr2;

	uacpi_interrupt_ret ret = fn (ctx);
	return (ret == UACPI_INTERRUPT_HANDLED)
		? INTERRUPT_HANDLED
		: INTERRUPT_NOT_HANDLED;
}

uacpi_status
uacpi_kernel_install_interrupt_handler (uacpi_u32 irq,
		uacpi_interrupt_handler fn, uacpi_handle ctx, uacpi_handle *out)
{
	struct interrupt_handler *h = kmalloc (sizeof (*h));
	if (!h)
		return UACPI_STATUS_OUT_OF_MEMORY;

	h->handler = handle_uacpi_interrupt;
	h->ptr1 = fn;
	h->ptr2 = ctx;
	errno_t e = install_interrupt_handler (h, irq, 0);
	if (e != ESUCCESS)
		return UACPI_STATUS_INTERNAL_ERROR;

	*out = h;
	return UACPI_STATUS_OK;
}

uacpi_status
uacpi_kernel_uninstall_interrupt_handler (uacpi_interrupt_handler fn,
		uacpi_handle handle)
{
	(void) fn;

	struct interrupt_handler *h = handle;
	uninstall_interrupt_handler (h);
	return UACPI_STATUS_OK;
}

uacpi_handle
uacpi_kernel_create_spinlock (void)
{
	spinlock_t *lck = kmalloc (sizeof (*lck));
	if (lck)
		spinlock_init (lck);
	return lck;
}

void
uacpi_kernel_free_spinlock (uacpi_handle lck)
{
	kfree (lck);
}

uacpi_cpu_flags
uacpi_kernel_lock_spinlock (uacpi_handle lck)
{
	return spin_lock_irq (lck);
}

void
uacpi_kernel_unlock_spinlock (uacpi_handle lck, uacpi_cpu_flags flag)
{
	spin_unlock_irq (lck, flag);
}

uacpi_status
uacpi_kernel_schedule_work (uacpi_work_type tp,
		uacpi_work_handler fn, uacpi_handle ctx)
{
	(void) tp;
	(void) fn;
	(void) ctx;

	return UACPI_STATUS_UNIMPLEMENTED;
}

uacpi_status
uacpi_kernel_wait_for_work_completion (void)
{
	return UACPI_STATUS_OK;
}

