// SPDX-License-Identifier: GPL-3.0
/**
 * File: acpi/osl.cc
 * uACPI operative system specific layer (OSL).
 *
 * Copyright (C) 2025  dbstream
 */
#include <Hal/mm.h>
#include <Ke/log.h>
#include <Ke/spinlock.h>
#include <Mm/pool.h>
#include <Mm/vmap.h>
#include <Acpi/setup.h>
#include <uacpi/kernel_api.h>
#include <uacpi/uacpi.h>

static unsigned long rsdp_address = 0;
static bool rsdp_address_valid = false;

/**
 * AcpiSetRSDPAddress - set the RSDP physical address.
 * @phys_addr: physical address of ACPI RSDPv1 or RSDPv2
 *
 * This function is typically called by early Hal initialization code that runs
 * before most kernel subsystems have been initialized.
 */
void AcpiSetRSDPAddress(unsigned long phys_addr)
{
	rsdp_address = phys_addr;
	rsdp_address_valid = true;
}

/**
 * uacpi_kernel_get_rsdp - return the physical address of the RSDP via @out.
 * @out: location where the physical address of the RSDP will be returned
 */
uacpi_status uacpi_kernel_get_rsdp(uacpi_phys_addr *out)
{
	if (!rsdp_address_valid)
		return UACPI_STATUS_NOT_FOUND;

	*out = rsdp_address;
	return UACPI_STATUS_OK;
}

/**
 * uacpi_kernel_map - map virtual memory.
 * @addr: physical address
 * @len: length in bytes
 *
 * This function maps a physical memory region starting at @addr of length @len
 * into the kernel's virtual address space.
 *
 * NOTE: this function must cope with unaligned @addr and @len.  It does by
 * calling MmMapVirtual with a page-aligned address and size and returning a
 * pointer offset by @addr masked with the page mask (PAGE_SIZE - 1).
 */
void *uacpi_kernel_map(uacpi_phys_addr addr, uacpi_size len)
{
	unsigned long offset_in_page = addr & (PAGE_SIZE - 1);

	addr -= offset_in_page;
	if (len + offset_in_page < len) {
		[[unlikely]];
		return nullptr;
	}
	len += offset_in_page;
	len = PGALIGN_UP(len);
	if (!len) {
		[[unlikely]]
		return nullptr;
	}

	void *ptr = MmMapVirtual(addr, len, PTEFLAGS_READWRITE);
	if (!ptr)
		return nullptr;

	return (void *) ((uintptr_t) ptr + offset_in_page);
}

/**
 * uacpi_kernel_unmap - unmap virtual memory.
 * @addr: pointer to virtual memory region
 * @len: length in bytes
 *
 * This function unmaps a virtual memory region setup with @uacpi_kernel_map.
 */
void uacpi_kernel_unmap(void *addr, uacpi_size len)
{
	(void) len;
	// MmUnmapVirtual can cope with any pointer into the memory region.
	MmUnmapVirtual(addr);
}

/**
 * uacpi_kernel_log - display a message on the kernel console.
 * @level: uACPI log level
 * @msg: message to display
 */
void uacpi_kernel_log(uacpi_log_level level, const char *msg)
{
	(void) level;
	KePrintf("uACPI: %s", msg);
}

/**
 * uacpi_kernel_pci_device_open - open a PCI device.
 * @address: PCI device address
 * @out_handle: location to return a handle in
 */
uacpi_status uacpi_kernel_pci_device_open(
		uacpi_pci_address address,
		uacpi_handle *out_handle
)
{
	(void) address;
	(void) out_handle;
	KePrintf("warning: uacpi_kernel_pci_device_open is a stub\n");

	return UACPI_STATUS_UNIMPLEMENTED;
}

/**
 * uacpi_kernel_pci_device_close - close a PCI device.
 * @handle: handle returned by uacpi_kernel_pci_device_open
 */
void uacpi_kernel_pci_device_close(uacpi_handle handle)
{
	(void) handle;
	KePrintf("warning: uacpi_kernel_pci_device_close is a stub\n");
}

/**
 * uacpi_kernel_pci_read8 - read an 8-bit value from PCI configuration space.
 * @device: handle returned by uacpi_kernel_pci_device_open
 * @offset: byte offset into the device's configuration space
 * @value: location to return the read value in
 */
uacpi_status uacpi_kernel_pci_read8(
		uacpi_handle device,
		uacpi_size offset,
		uacpi_u8 *value
)
{
	(void) device;
	(void) offset;
	(void) value;
	KePrintf("warning: uacpi_kernel_pci_read8 is a stub\n");

	return UACPI_STATUS_UNIMPLEMENTED;
}

/**
 * uacpi_kernel_pci_read16 - read a 16-bit value from PCI configuration space.
 * @device: handle returned by uacpi_kernel_pci_device_open
 * @offset: byte offset into the device's configuration space
 * @value: location to return the read value in
 */
uacpi_status uacpi_kernel_pci_read16(
		uacpi_handle device,
		uacpi_size offset,
		uacpi_u16 *value
)
{
	(void) device;
	(void) offset;
	(void) value;
	KePrintf("warning: uacpi_kernel_pci_read16 is a stub\n");

	return UACPI_STATUS_UNIMPLEMENTED;
}

/**
 * uacpi_kernel_pci_read32 - read a 32-bit value from PCI configuration space.
 * @device: handle returned by uacpi_kernel_pci_device_open
 * @offset: byte offset into the device's configuration space
 * @value: location to return the read value in
 */
uacpi_status uacpi_kernel_pci_read32(
		uacpi_handle device,
		uacpi_size offset,
		uacpi_u32 *value
)
{
	(void) device;
	(void) offset;
	(void) value;
	KePrintf("warning: uacpi_kernel_pci_read32 is a stub\n");

	return UACPI_STATUS_UNIMPLEMENTED;
}

/**
 * uacpi_kernel_pci_write8 - write an 8-bit value to PCI configuration space.
 * @device: handle returned by uacpi_kernel_pci_device_open
 * @offset: byte offset into the device's configuration space
 * @value: value to write
 */
uacpi_status uacpi_kernel_pci_write8(
		uacpi_handle device,
		uacpi_size offset,
		uacpi_u8 value
)
{
	(void) device;
	(void) offset;
	(void) value;
	KePrintf("warning: uacpi_kernel_pci_write8 is a stub\n");

	return UACPI_STATUS_UNIMPLEMENTED;
}

/**
 * uacpi_kernel_pci_write16 - write a 16-bit value to PCI configuration space.
 * @device: handle returned by uacpi_kernel_pci_device_open
 * @offset: byte offset into the device's configuration space
 * @value: value to write
 */
uacpi_status uacpi_kernel_pci_write16(
    uacpi_handle device, uacpi_size offset, uacpi_u16 value
)
{
	(void) device;
	(void) offset;
	(void) value;
	KePrintf("warning: uacpi_kernel_pci_write16 is a stub\n");

	return UACPI_STATUS_UNIMPLEMENTED;
}

/**
 * uacpi_kernel_pci_write32 - write a 32-bit value to PCI configuration space.
 * @device: handle returned by uacpi_kernel_pci_device_open
 * @offset: byte offset into the device's configuration space
 * @value: value to write
 */
uacpi_status uacpi_kernel_pci_write32(
    uacpi_handle device, uacpi_size offset, uacpi_u32 value
)
{
	(void) device;
	(void) offset;
	(void) value;
	KePrintf("warning: uacpi_kernel_pci_write32 is a stub\n");

	return UACPI_STATUS_UNIMPLEMENTED;
}

/**
 * uacpi_kernel_io_map - map a SystemIO range.
 * @base: start of the I/O port range
 * @len: length of the I/O port range
 * @out_handle: location to return a handle in
 */
uacpi_status uacpi_kernel_io_map(
		uacpi_io_addr base,
		uacpi_size len,
		uacpi_handle *out_handle
)
{
	(void) base;
	(void) len;
	(void) out_handle;
	KePrintf("warning: uacpi_kernel_io_map is a stub\n");

	return UACPI_STATUS_UNIMPLEMENTED;
}

/**
 * uacpi_kernel_io_unmap - unmap a SystemIO range.
 * @handle: handle returned by uacpi_kernel_io_map
 */
void uacpi_kernel_io_unmap(uacpi_handle handle)
{
	(void) handle;
	KePrintf("warning: uacpi_kernel_io_unmap is a stub\n");
}

/**
 * uacpi_kernel_io_read8 - read an 8-bit value from SystemIO.
 * @handle: handle returned by uacpi_kernel_io_map
 * @offset: offset from I/O port base
 * @out_value: location to return the value we read in
 */
uacpi_status uacpi_kernel_io_read8(
		uacpi_handle handle,
		uacpi_size offset,
		uacpi_u8 *out_value
)
{
	(void) handle;
	(void) offset;
	(void) out_value;
	KePrintf("warning: uacpi_kernel_io_read8 is a stub\n");

	return UACPI_STATUS_UNIMPLEMENTED;
}

/**
 * uacpi_kernel_io_read16 - read a 16-bit value from SystemIO.
 * @handle: handle returned by uacpi_kernel_io_map
 * @offset: offset from I/O port base
 * @out_value: location to return the value we read in
 */
uacpi_status uacpi_kernel_io_read16(
		uacpi_handle handle,
		uacpi_size offset,
		uacpi_u16 *out_value
)
{
	(void) handle;
	(void) offset;
	(void) out_value;
	KePrintf("warning: uacpi_kernel_io_read16 is a stub\n");

	return UACPI_STATUS_UNIMPLEMENTED;
}

/**
 * uacpi_kernel_io_read32 - read a 32-bit value from SystemIO.
 * @handle: handle returned by uacpi_kernel_io_map
 * @offset: offset from I/O port base
 * @out_value: location to return the value we read in
 */
uacpi_status uacpi_kernel_io_read32(
		uacpi_handle handle,
		uacpi_size offset,
		uacpi_u32 *out_value
)
{
	(void) handle;
	(void) offset;
	(void) out_value;
	KePrintf("warning: uacpi_kernel_io_read32 is a stub\n");

	return UACPI_STATUS_UNIMPLEMENTED;
}

/**
 * uacpi_kernel_io_write8 - write an 8-bit value to SystemIO.
 * @handle: handle returned by uacpi_kernel_io_map
 * @offset: offset from I/O port base
 * @in_value: value to write
 */
uacpi_status uacpi_kernel_io_write8(
		uacpi_handle handle,
		uacpi_size offset,
		uacpi_u8 in_value
)
{
	(void) handle;
	(void) offset;
	(void) in_value;
	KePrintf("warning: uacpi_kernel_io_write8 is a stub\n");

	return UACPI_STATUS_UNIMPLEMENTED;
}

/**
 * uacpi_kernel_io_write16 - write a 16-bit value to SystemIO.
 * @handle: handle returned by uacpi_kernel_io_map
 * @offset: offset from I/O port base
 * @in_value: value to write
 */
uacpi_status uacpi_kernel_io_write16(
		uacpi_handle handle,
		uacpi_size offset,
		uacpi_u16 in_value
)
{
	(void) handle;
	(void) offset;
	(void) in_value;
	KePrintf("warning: uacpi_kernel_io_write16 is a stub\n");

	return UACPI_STATUS_UNIMPLEMENTED;
}

/**
 * uacpi_kernel_io_write32 - write a 32-bit value to SystemIO.
 * @handle: handle returned by uacpi_kernel_io_map
 * @offset: offset from I/O port base
 * @in_value: value to write
 */
uacpi_status uacpi_kernel_io_write32(
		uacpi_handle handle,
		uacpi_size offset,
		uacpi_u32 in_value
)
{
	(void) handle;
	(void) offset;
	(void) in_value;
	KePrintf("warning: uacpi_kernel_io_write32 is a stub\n");

	return UACPI_STATUS_UNIMPLEMENTED;
}

/**
 * uacpi_kernel_alloc - allocate a block of memory.
 * @size: number of bytes to allocate
 */
void *uacpi_kernel_alloc(uacpi_size size)
{
	if (size <= PAGE_SIZE / 2)
		return MmAllocateObject(size);
	else
		return MmAllocateVirtual(size, PTEFLAGS_READWRITE);
}

/**
 * uacpi_kernel_free - free a block of memory.
 * @mem: pointer that was returned by uacpi_kernel_alloc
 */
void uacpi_kernel_free(void *mem)
{
	if (MmIsVmapPointer(mem))
		MmFreeVirtual(mem);
	else
		MmFreeObject(mem);
}

/**
 * uacpi_kernel_get_nanoseconds_since_boot - get the time since boot.
 *
 * This function must behave like a monotonically increasing clock.
 */
uacpi_u64 uacpi_kernel_get_nanoseconds_since_boot(void)
{
	KePrintf("warning: uacpi_kernel_get_nanoseconds_since_boot is a stub\n");
	return 0;
}

/**
 * uacpi_kernel_stall - spin for @usec microseconds.
 * @usec: the number of microseconds to spin for
 */
void uacpi_kernel_stall(uacpi_u8 usec)
{
	(void) usec;
	KePrintf("warning: uacpi_kernel_stall is a stub\n");
}

/**
 * uacpi_kernel_sleep - for @msec milliseconds.
 * @msec: the number of milliseconds to sleep for
 */
void uacpi_kernel_sleep(uacpi_u64 msec)
{
	(void) msec;
	KePrintf("warning: uacpi_kernel_sleep is a stub\n");
}

/**
 * uacpi_kernel_create_mutex - create a mutex object.
 */
uacpi_handle uacpi_kernel_create_mutex(void)
{
	// This is needed for initialization to succeed:
	static unsigned long x = 1UL;
	return (void *) x++;
}

/**
 * uacpi_kernel_free_mutex - free a mutex object.
 * @handle: handle to mutex object
 */
void uacpi_kernel_free_mutex(uacpi_handle handle)
{
	(void) handle;
}

/**
 * uacpi_kernel_create_event - create a semaphore object.
 */
uacpi_handle uacpi_kernel_create_event(void)
{
	// This is needed for initialization to succeed:
	static unsigned long x = 1UL;
	return (void *) x++;
}

/**
 * uacpi_kernel_free_event - free a semaphore object.
 * @handle: handle to semaphore object
 */
void uacpi_kernel_free_event(uacpi_handle handle)
{
	(void) handle;
}

/**
 * uacpi_kernel_get_thread_id - get an identifier for the current thread.
 */
uacpi_thread_id uacpi_kernel_get_thread_id(void)
{
	return (void *) 1UL;
}

/**
 * uacpi_kernel_acquire_mutex - lock a mutex.
 * @handle: handle to mutex object
 * @timeout: millisecond timeout
 *
 * The timeout value has the following meaning:
 * 0x0000 - trylock
 * 0x0001...0xfffe - try to lock the mutex for at least @timeout milliseconds
 * 0xffff - lock the mutex without a timeout
 */
uacpi_status uacpi_kernel_acquire_mutex(uacpi_handle handle, uacpi_u16 timeout)
{
	(void) handle;
	(void) timeout;
	// FIXME: this is a stub.
	return UACPI_STATUS_OK;
}

/**
 * uacpi_kernel_release_mutex - unlock a mutex.
 * @handle: handle to mutex object
 */
void uacpi_kernel_release_mutex(uacpi_handle handle)
{
	(void) handle;
	// FIXME: this is a stub.
}

/**
 * uacpi_kernel_wait_for_event - wait on a semaphore.
 * @handle: handle to semaphore object
 * @timeout: millisecond timeout
 *
 * The timeout value has the following meaning:
 * 0x0000 - trylock
 * 0x0001...0xfffe - wait for the semaphore for at least @timeout milliseconds
 * 0xffff - wait for the semaphore without a timeout
 */
uacpi_bool uacpi_kernel_wait_for_event(uacpi_handle handle, uacpi_u16 timeout)
{
	(void) handle;
	(void) timeout;
	// FIXME: this is a stub.
	return UACPI_TRUE;
}

/**
 * uacpi_kernel_signal_event - signal a semaphore.
 * @handle: handle to semaphore object
 */
void uacpi_kernel_signal_event(uacpi_handle handle)
{
	(void) handle;
	// FIXME: this is a stub.
}

/**
 * uacpi_kernel_reset_event - reset a semaphore.
 * @handle: handle to semaphore object
 */
void uacpi_kernel_reset_event(uacpi_handle handle)
{
	(void) handle;
	// FIXME: this is a stub.
}

/**
 * uacpi_kernel_handle_firmware_request - handle a Breakpoint or Fatal operator.
 * @desc: firmware request descriptor structure
 */
uacpi_status uacpi_kernel_handle_firmware_request(uacpi_firmware_request *desc)
{
	if (desc->type == UACPI_FIRMWARE_REQUEST_TYPE_FATAL) {
		KePanic("ACPI: (AML) Fatal type=%d code=0x%x arg=0x%llx",
				desc->fatal.type,
				desc->fatal.code,
				(unsigned long long) desc->fatal.arg);
	}

	return UACPI_STATUS_OK;
}

/**
 * uacpi_kernel_install_interrupt_handler - install an interrupt handler.
 * @irq: IRQ number
 * @function: handler function
 * @ctx: handler context, passed to @function
 * @out_irq_handle: handle passed to uacpi_kernel_uninstall_interrupt_handler
 */
uacpi_status uacpi_kernel_install_interrupt_handler(
		uacpi_u32 irq,
		uacpi_interrupt_handler function,
		uacpi_handle ctx,
		uacpi_handle *out_irq_handle
)
{
	(void) irq;
	(void) function;
	(void) ctx;
	(void) out_irq_handle;
	KePrintf("warning: uacpi_kernel_install_interrupt_handler is a stub\n");

	return UACPI_STATUS_UNIMPLEMENTED;
}

/**
 * uacpi_kernel_uninstall_interrupt_handler - uninstall an interrupt handler.
 * @function: handler function
 * @irq_handle: the handle returned by uacpi_kernel_install_interrupt_handler
 */
uacpi_status uacpi_kernel_uninstall_interrupt_handler(
		uacpi_interrupt_handler function,
		uacpi_handle irq_handle)
{
	(void) function;
	(void) irq_handle;
	KePrintf("warning: uacpi_kernel_uninstall_interrupt_handler is a stub\n");

	return UACPI_STATUS_UNIMPLEMENTED;
}

/**
 * uacpi_kernel_create_spinlock - create a spinlock object.
 */
uacpi_handle uacpi_kernel_create_spinlock(void)
{
	KeIRQSpinlock *lock = MmNew<KeIRQSpinlock>();
	if (lock)
		lock->init();
	return lock;
}

/**
 * uacpi_kernel_free_spinlock - free a spinlock object.
 * @handle: spinlock handle
 */
void uacpi_kernel_free_spinlock(uacpi_handle handle)
{
	KeIRQSpinlock *lock = (KeIRQSpinlock *) handle;
	BUG_ON(!lock->trylock()); // It is a bug to not unlock a spinlock.
	lock->unlock();
	MmDelete(lock);
}

/**
 * uacpi_kernel_lock_spinlock - disable interrupts and lock a spinlock.
 * @handle: handle to spinlock object
 */
uacpi_cpu_flags uacpi_kernel_lock_spinlock(uacpi_handle handle)
{
	KeIRQSpinlock *lock = (KeIRQSpinlock *) handle;
	lock->lock();

	return 0;
}

/**
 * uacpi_kernel_unlock_spinlock - unlock a spinlock and enable interrupts.
 * @handle: handle to spinlock object
 * @cpuflags: uacpi_cpu_flags returned by uacpi_kernel_lock_spinlock
 */
void uacpi_kernel_unlock_spinlock(uacpi_handle handle, uacpi_cpu_flags cpuflags)
{
	(void) cpuflags;

	KeIRQSpinlock *lock = (KeIRQSpinlock *) handle;
	lock->unlock();
}

/**
 * uacpi_kernel_schedule_work - schedule work for deferred execution.
 * @work_type: one of UACPI_WORK_GPE_EXECUTION and UACPI_WORK_NOTIFICATION
 * @function: work function
 * @ctx: context, passed to @function
 *
 * This function schedules one item of work for deferred execution on the GPE
 * queue or the notification queue, depending on the value of @work_type.
 */
uacpi_status uacpi_kernel_schedule_work(
		uacpi_work_type work_type,
		uacpi_work_handler function,
		uacpi_handle ctx
)
{
	(void) work_type;
	(void) function;
	(void) ctx;
	KePrintf("warning: uacpi_kernel_schedule_work is a stub\n");

	return UACPI_STATUS_UNIMPLEMENTED;
}

/**
 * uacpi_kernel_wait_for_work_completion - wait for work to complete.
 *
 * This function waits for two types of work to finish:
 * 1. All interrupts in-flight with handlers installed by
 *    uacpi_install_interrupt_handler.
 * 2. All work scheduled via uacpi_kernel_schedule_work.
 * The waits must be done in this order.
 */
uacpi_status uacpi_kernel_wait_for_work_completion(void)
{
	// FIXME: this is a stub
	return UACPI_STATUS_OK;
}

