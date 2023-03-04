#include <davix/acpi.h>
#include <davix/printk.h>
#include <davix/printk_lib.h>
#include <davix/mm.h>
#include <asm/page.h>

#ifdef CONFIG_X86
#include <asm/boot.h>
#include <asm/io.h>
#endif

acpi_physical_address acpi_os_get_root_pointer(void)
{
#ifdef CONFIG_X86
	return x86_boot_struct.acpi_rsdp;
#else
	return 0;
#endif
}

static char acpi_prbuf[512];

static inline int starts_with(const char *buf, const char *str)
{
	for(; *str; buf++, str++)
		if(*str != *buf)
			return 0;
	return 1;
}

void acpi_os_vprintf(const char *fmt, va_list ap)
{
	unsigned long len = strlen(acpi_prbuf);
	unsigned long fmt_len = strlen(fmt);
	vsnprintk(acpi_prbuf + len, 512 - len, fmt, ap);
	if(fmt_len && fmt[fmt_len - 1] == '\n') {
		char loglevel = KERN_DEBUG;
		if(starts_with(acpi_prbuf, "ACPI Error: ")
			|| starts_with(acpi_prbuf, "Firmware Error (ACPI): ")) {
				loglevel = KERN_ERROR;
		} else if(starts_with(acpi_prbuf, "ACPI Warning: ")
			|| starts_with(acpi_prbuf, "Firmware Warning (ACPI): ")) {
				loglevel = KERN_WARN;
		} else if(starts_with(acpi_prbuf, "ACPI: ")) {
			loglevel = KERN_INFO;
		}
		printk(loglevel, "%s", acpi_prbuf);
		acpi_prbuf[0] = 0;
	}
}

void acpi_os_printf(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	acpi_os_vprintf(fmt, ap);
	va_end(ap);
}

acpi_thread_id acpi_os_get_thread_id(void)
{
	return 1;
}

acpi_status acpi_os_create_semaphore(u32 max_units,
	u32 initial_units, acpi_semaphore *out_handle)
{
	struct semaphore *sem = kmalloc(sizeof(struct semaphore));
	if(!sem)
		return AE_NO_MEMORY;

	semaphore_init(sem, initial_units);
	*out_handle = sem;
	return AE_OK;
}

acpi_status acpi_os_delete_semaphore(acpi_semaphore sem)
{
	kfree(sem);
	return AE_OK;
}

acpi_status acpi_os_wait_semaphore(acpi_semaphore sem, u32 units, u16 timeout)
{
	if(!sem || (units == 0))
		return AE_BAD_PARAMETER;

	if(units != 1)
		return AE_SUPPORT;

	if(sem_try_wait(sem))
		return AE_OK;

	if(timeout != 0xffff) {
		warn("acpi_os_wait_semaphore() with timeout unimplemented.\n");
		return AE_SUPPORT;
	}

	sem_wait(sem);
	return AE_OK;
}

acpi_status acpi_os_signal_semaphore(acpi_semaphore sem, u32 units)
{
	for(; units; units--)
		sem_signal(sem);
	return AE_OK;
}

acpi_status acpi_os_create_lock(acpi_spinlock *out_handle)
{
	spinlock_t *lock = kmalloc(sizeof(spinlock_t));
	if(!lock)
		return AE_NO_MEMORY;

	spinlock_init(lock);
	*out_handle = lock;
	return AE_OK;
}

void acpi_os_delete_lock(acpi_spinlock lock)
{
	kfree(lock);
}

acpi_cpu_flags acpi_os_acquire_lock(acpi_spinlock lock)
{
	return spin_acquire_irq(lock);
}

void acpi_os_release_lock(acpi_spinlock lock, acpi_cpu_flags flag)
{
	spin_release_irq(lock, flag);
}

acpi_status acpi_os_predefined_override(const struct acpi_predefined_names
	*object, acpi_string *value)
{
	*value = NULL;
	return AE_OK;
}

acpi_status acpi_os_table_override(struct acpi_table_header *table,
	struct acpi_table_header **value)
{
	*value = NULL;
	return AE_OK;
}

acpi_status acpi_os_physical_table_override(struct acpi_table_header *table,
	acpi_physical_address *addr, u32 *len)
{
	*addr = 0;
	return AE_OK;
}

void *acpi_os_map_memory(acpi_physical_address addr, acpi_size size)
{
	acpi_size orig_size = size;
	unsigned long pg_addr = align_down(addr, PAGE_SIZE);
	unsigned long pgoff = addr - pg_addr;
	size = align_up(size + pgoff, PAGE_SIZE);

	unsigned long mem = (unsigned long) vmap(pg_addr, size,
		PG_READABLE | PG_WRITABLE, PG_UNCACHED);
	if(!mem)
		debug("acpi_os_map_memory(%p, %lu) => ENOMEM\n",
			addr, orig_size);

	return (void *) (mem + pgoff);
}

void acpi_os_unmap_memory(void *mem, acpi_size size)
{
	vunmap((void *) align_down((unsigned long) mem, PAGE_SIZE));
}

void acpi_os_sleep(u64 msecs)
{
	warn("acpi_os_sleep() unimplemented.\n");
}

void acpi_os_stall(u32 usecs)
{
	warn("acpi_os_stall() unimplemented.\n");
}

acpi_status acpi_os_install_interrupt_handler(u32 irq,
	acpi_osd_handler handler, void *param)
{
	warn("acpi_os_install_interrupt_handler() unimplemented.\n");
	return AE_NOT_IMPLEMENTED;
}

acpi_status acpi_os_remove_interrupt_handler(u32 irq, acpi_osd_handler handler)
{
	warn("acpi_os_remove_interrupt_handler() unimplemented.\n");
	return AE_NOT_IMPLEMENTED;
}

acpi_status acpi_os_execute(acpi_execute_type type,
	acpi_osd_exec_callback fn, void *ctx)
{
	warn("acpi_os_execute() unimplemented.\n");
	return AE_NOT_IMPLEMENTED;
}

void acpi_os_wait_events_complete(void)
{
	warn("acpi_os_wait_events_complete() unimplemented.\n");
}

acpi_status acpi_os_signal(u32 type, void *info)
{
	switch(type) {
	case ACPI_SIGNAL_BREAKPOINT:
		debug("ACPI: AML breakpoint: %s\n", (const char *) info);
		return AE_OK;
	case ACPI_SIGNAL_FATAL:;
		struct acpi_signal_fatal_info *i = (struct acpi_signal_fatal_info *) info;
		critical("ACPI: AML fatal signal (type %x code %x argument %x)\n",
			i->type, i->code, i->argument);
		return AE_OK;
	default:
		warn("acpi_os_signal(): unknown signal type %u\n", type);
		return AE_OK;
	}
}

acpi_status acpi_os_enter_sleep(u8 state, u32 reg1, u32 reg2)
{
	debug("acpi_os_enter_sleep() called.\n");
	return AE_OK;
}

u64 acpi_os_get_timer(void)
{
	static atomic unsigned long timerval = 0;
	warn("acpi_os_get_timer() unimplemented.\n");
	return atomic_fetch_add(&timerval, 1, memory_order_relaxed);
}

acpi_status acpi_os_read_memory(acpi_physical_address addr, u64 *value, u32 width)
{
	*value = 0;

	void *virt = acpi_os_map_memory(addr, width / 8);
	if(!virt)
		return AE_ERROR;

	switch(width) {
		case 8:
			*value = *(volatile u8 *) virt;
			break;
		case 16:
			*value = *(volatile u16 *) virt;
			break;
		case 32:
			*value = *(volatile u32 *) virt;
			break;
		case 64:
			*value = *(volatile u64 *) virt;
			break;
	}

	acpi_os_unmap_memory(virt, width / 8);
	return AE_OK;
}

acpi_status acpi_os_write_memory(acpi_physical_address addr, u64 value, u32 width)
{
	void *virt = acpi_os_map_memory(addr, width / 8);
	if(!virt)
		return AE_ERROR;

	switch(width) {
		case 8:
			*(volatile u8 *) virt = value;
			break;
		case 16:
			*(volatile u16 *) virt = value;
			break;
		case 32:
			*(volatile u32 *) virt = value;
			break;
		case 64:
			*(volatile u64 *) virt = value;
			break;
	}

	acpi_os_unmap_memory(virt, width / 8);
	return AE_OK;
}

acpi_status acpi_os_read_pci_configuration(struct acpi_pci_id *dev, u32 offset,
	u64 *value, u32 width)
{
	warn("acpi_os_read_pci_configuration() unimplemented.\n");
	return AE_NOT_IMPLEMENTED;

}

acpi_status acpi_os_write_pci_configuration(struct acpi_pci_id *dev, u32 offset,
	u64 value, u32 width)
{
	warn("acpi_os_write_pci_configuration() unimplemented.\n");
	return AE_NOT_IMPLEMENTED;
}

acpi_status acpi_os_read_port(acpi_io_address port, u32 *value, u32 width)
{
#ifdef CONFIG_X86
	switch(width) {
	case 8:
		*value = io_inb(port);
		break;
	case 16:
		*value = io_inw(port);
		break;
	case 32:
		*value = io_inl(port);
		break;
	}
	return AE_OK;
#else
	return AE_NOT_IMPLEMENTED;
#endif
}

acpi_status acpi_os_write_port(acpi_io_address port, u32 value, u32 width)
{
#ifdef CONFIG_X86
	switch(width) {
	case 8:
		io_outb(port, value);
		break;
	case 16:
		io_outw(port, value);
		break;
	case 32:
		io_outl(port, value);
		break;
	}
	return AE_OK;
#else
	return AE_NOT_IMPLEMENTED;
#endif
}
