/**
 * x86 setup code.
 * Copyright (C) 2024  dbstream
 */
#include <acpi/acpi.h>
#include <acpi/tables.h>
#include <davix/align.h>
#include <davix/efi_types.h>
#include <davix/fbcon.h>
#include <davix/initmem.h>
#include <davix/ioresource.h>
#include <davix/kmalloc.h>
#include <davix/main.h>
#include <davix/panic.h>
#include <davix/printk.h>
#include <davix/stddef.h>
#include <davix/string.h>
#include <davix/vmap.h>
#include <asm/acpi.h>
#include <asm/apic.h>
#include <asm/cregs.h>
#include <asm/cpulocal.h>
#include <asm/early_memmap.h>
#include <asm/features.h>
#include <asm/mm_init.h>
#include <asm/sections.h>
#include <asm/smp.h>
#include <asm/time.h>
#include <asm/trap.h>
#include "multiboot.h"

__INIT_DATA static struct multiboot_params *boot_params;

unsigned long kernel_load_offset;

__INIT_TEXT
void
x86_start_kernel (struct multiboot_params *params, unsigned long load_offset)
{
	boot_params = params;
	kernel_load_offset = load_offset;

	__cpulocal_offsets[0] = this_cpu_read (&__cpulocal_offset);

	__cr0_state = read_cr0 ();
	__cr4_state = read_cr4 ();
	bsp_features_init ();
	main ();
}

static inline struct multiboot_tag *
mb2_first (void)
{
	struct multiboot_tag *tag = (struct multiboot_tag *)
		((unsigned long) boot_params + 8);

	if (tag->type == MB2_TAG_END)
		tag = NULL;

	return tag;
}

static inline struct multiboot_tag *
mb2_next (struct multiboot_tag *tag)
{
	tag = (struct multiboot_tag *) ((unsigned long) tag + tag->size);
	tag = ALIGN_UP_PTR(tag, 8);

	if (tag->type == MB2_TAG_END)
		tag = NULL;

	return tag;
}

__INIT_TEXT
static void
reserve_stuff (void)
{
	unsigned long header_start = (unsigned long) __header_start;
	unsigned long kernel_end = (unsigned long) __kernel_end;

	unsigned long embed_size = *(unsigned long *) kernel_end;

	initmem_reserve (header_start - __KERNEL_START + kernel_load_offset,
		ALIGN_UP (embed_size, PAGE_SIZE) + kernel_end - header_start,
		INITMEM_KERN_RESERVED);

	initmem_reserve ((unsigned long) boot_params, boot_params->size,
		INITMEM_KERN_RESERVED);

	/* Reserve the low 64K of memory, since some BIOSes may need it. */
	initmem_reserve (0, 0x10000, 0);
}

__INIT_TEXT
static void
init_acpi_tables_early (void)
{
	struct multiboot_tag *tag, *rsdp = NULL, *rsdp2 = NULL;
	for (tag = mb2_first (); tag; tag = mb2_next (tag)) {
		switch (tag->type) {
		case MB2_TAG_RSDPv1:
			rsdp = tag;
			break;
		case MB2_TAG_RSDPv2:
			rsdp2 = tag;
			break;
		}
	}

	if (rsdp2)
		rsdp = rsdp2;

	if (!rsdp)
		panic ("No ACPI RSD PTR was provided by bootloader.");

	/**
	 * uACPI needs the physical address of the RSDP for some reason.
	 * When this is called, we are operating in low direct-mapped memory
	 * anyways, so phys=virt.
	 */
	acpi_init_tables ((void *) ((unsigned long) rsdp + 8),
			(unsigned long) rsdp + 8);

	for (unsigned long i = 0; i < num_acpi_tables; i++) {
		initmem_reserve (acpi_tables[i].address,
			acpi_tables[i].header.length, 0);
	}
}

static inline unsigned long
memmap_count (struct multiboot_memmap *memmap)
{
	return (memmap->size - sizeof(*memmap)) / memmap->entry_size;
}

static inline void *
memmap_entry (struct multiboot_memmap *memmap, unsigned long i)
{
	return (void *) ((unsigned long) memmap
		+ sizeof(*memmap) + i * memmap->entry_size);
}

__INIT_TEXT
static void
parse_mbi_memmap (struct multiboot_memmap *memmap,
	void (*callback)(unsigned long, unsigned long, initmem_flags_t))
{
	unsigned long num = memmap_count (memmap);
	for (unsigned long i = 0; i < num; i++) {
		struct multiboot_memmap_entry *e = memmap_entry (memmap, i);
		initmem_flags_t flags;
		switch (e->type) {
		case MB2_MEMMAP_USABLE:
		case MB2_MEMMAP_ACPI_RECLAIM:
			flags = 0;
			break;
		case MB2_MEMMAP_ACPI_NVS:
			flags = INITMEM_ACPI_NVS;
			break;
		case MB2_MEMMAP_DEFECTIVE:
		default:
			flags = INITMEM_RESERVE;
		}

		callback (e->start, e->size, flags);
	}
}

__INIT_TEXT
static void
parse_efi_memmap (struct multiboot_memmap *memmap,
	void (*callback)(unsigned long, unsigned long, initmem_flags_t))
{
	unsigned long num = memmap_count (memmap);
	for (unsigned long i = 0; i < num; i++) {
		struct efi_memory_descriptor *e = memmap_entry (memmap, i);
		initmem_flags_t flags;
		switch (e->type) {
		case EFI_MEMORY_LOADER_CODE:
		case EFI_MEMORY_LOADER_DATA:
			flags = 0;
			break;
		case EFI_MEMORY_BOOT_SERVICES_CODE:
		case EFI_MEMORY_BOOT_SERVICES_DATA:
			flags = INITMEM_AVOID_DURING_INIT;
			break;
		case EFI_MEMORY_ACPI_RECLAIM:
		case EFI_MEMORY_CONVENTIONAL_RAM:
			flags = 0;
			if (e->attribute & EFI_MEMORY_NV)
				flags |= INITMEM_PERSISTENT;
			if (e->attribute & EFI_MEMORY_SP)
				flags |= INITMEM_SPECIAL_PURPOSE;
			break;
		case EFI_MEMORY_ACPI_NVS:
			flags = INITMEM_ACPI_NVS;
			break;
		case EFI_MEMORY_PERSISTENT_RAM:
			flags = INITMEM_PERSISTENT;
			break;
		default:
			flags = INITMEM_RESERVE;
		}

		callback (e->phys_start, PAGE_SIZE * e->num_pages, flags);
	}
}

__INIT_TEXT
static void
__parse_memmap (struct multiboot_memmap *memmap,
	void (*callback)(unsigned long, unsigned long, initmem_flags_t))
{
	switch (memmap->type) {
	case MB2_TAG_EFI_MEMMAP:
		parse_efi_memmap (memmap, callback);
		break;
	case MB2_TAG_MEMMAP:
		parse_mbi_memmap (memmap, callback);
		break;
	}
}

__INIT_TEXT
static void
register_reserved_entries (unsigned long start, unsigned long size,
	initmem_flags_t flags)
{
	if (flags & INITMEM_RESERVE_MASK)
		initmem_register (start, size, flags);
}

__INIT_TEXT
static void
register_other_entries (unsigned long start, unsigned long size,
	initmem_flags_t flags)
{
	if (!(flags & INITMEM_RESERVE_MASK))
		initmem_register (start, size, flags);
}

__INIT_TEXT
static void
parse_memmap (void)
{
	struct multiboot_memmap *memmap = NULL, *efi_memmap = NULL;

	struct multiboot_tag *tag;
	for (tag = mb2_first (); tag; tag = mb2_next (tag)) {
		switch (tag->type) {
		case MB2_TAG_MEMMAP:
			memmap = (struct multiboot_memmap *) tag;
			break;
		case MB2_TAG_EFI_MEMMAP:
			efi_memmap = (struct multiboot_memmap *) tag;
			break;
		}
	}

	const char *provider = "multiboot";
	if (efi_memmap) {
		memmap = efi_memmap;
		provider = "EFI";
	}

	if (!memmap)
		panic ("No memory map was provided by the bootloader.");

	printk (PR_INFO "Using %s-memmap for system memory map.\n", provider);

	__parse_memmap (memmap, register_reserved_entries);
	__parse_memmap (memmap, register_other_entries);
}

__INIT_TEXT
static void
parse_multiboot_other (void)
{
	struct multiboot_module *boot_module = NULL;

	struct multiboot_tag *tag;
	for (tag = mb2_first (); tag; tag = mb2_next (tag)) {
		switch (tag->type) {
		case MB2_TAG_MODULE:
			boot_module = (struct multiboot_module *) tag;
			break;
		}
	}

	if (boot_module) {
		boot_module_start = boot_module->mod_start;
		boot_module_end = boot_module->mod_end;
	}
}

/**
 * arch_init() performs the most basic initializations, then does parsing of
 * multiboot_params into kernel-internal formats. We are running under the
 * assumption that the boot_params provided by header.S are fully mapped.
 */
__INIT_TEXT
void
arch_init (void)
{
	x86_pgtable_init ();

	printk (PR_NOTICE "CPU: %s %s\n", cpu_brand, cpu_model);
	printk (PR_NOTICE "max_phys_addr=0x%lx  (%lu GiB)\n", x86_max_phys_addr,
			x86_max_phys_addr >> 30U);

	reserve_stuff ();
	init_acpi_tables_early ();
	parse_memmap ();
	parse_multiboot_other ();

	initmem_dump (&initmem_usable_ram);
	initmem_dump (&initmem_reserved);

	x86_paging_init ();

	x86_init_acpi_tables_early ();
	x86_time_init_early ();
	early_apic_init ();
	x86_smp_init ();
	x86_trap_init ();
	apic_init ();
}

static void
add_framebuffer (struct multiboot_framebuffer *tag)
{
	printk (PR_INFO "mbi framebuffer:  0x%lx  %ux%ux%d  type %d\n",
			tag->framebuffer_addr, tag->width, tag->height,
			tag->bpp, tag->framebuffer_type);
	if (tag->framebuffer_type != MB2_FRAMEBUFFER_COLOR) {
		printk (PR_WARN "mbi framebuffer: only color framebuffers (type 1) are supported at the moment\n");
		return;
	}

	struct ioresource *res = kmalloc (sizeof (*res));
	if (!res) {
		printk (PR_ERR "mbi framebuffer: OOM in add_framebuffer\n");
		return;
	}

	unsigned long size = (unsigned long) tag->height * tag->pitch;
	unsigned long first = ALIGN_DOWN (tag->framebuffer_addr, PAGE_SIZE);
	unsigned long last = ALIGN_UP (tag->framebuffer_addr + size, PAGE_SIZE) - 1;
	res->vma_node.first = first;
	res->vma_node.last = last;
	strncpy (res->name, "mbi framebuffer", sizeof (res->name));
	if (register_ioresource (&iores_system_memory, res) != ESUCCESS) {
		printk (PR_ERR "mbi framebuffer: failed to register ioresource\n");
		kfree (res);
		return;
	}

	void *mapped = vmap_phys ("mbi framebuffer", tag->framebuffer_addr, size,
			PAGE_KERNEL_DATA | PG_WC);
	if (!mapped) {
		printk (PR_ERR "mbi framebuffer: failed to vmap memory\n");
		unregister_ioresource (res);
		kfree (res);
		return;
	}

	struct fbcon_format_info fmt = {
		.bpp = tag->bpp,
		.red_offset = tag->red_shift,
		.green_offset = tag->green_shift,
		.blue_offset = tag->blue_shift,
		.red_mask = ((1U << tag->red_bits) - 1) << tag->red_shift,
		.green_mask = ((1U << tag->green_bits) - 1) << tag->green_shift,
		.blue_mask = ((1U << tag->green_bits) - 1) << tag->blue_shift
	};
	struct fbcon *con = fbcon_register_framebuffer (tag->width, tag->height,
			tag->pitch, &fmt, mapped, NULL);
	if (!con) {
		vunmap (mapped);
		unregister_ioresource (res);
		kfree (res);
	}
}

void
arch_init_late (void)
{
	x86_mm_init_late ();
	ioapic_init ();

	/**
	 * When arch_init_late is called, we have removed the lower-half
	 * identity mapping that boot_params used to reside it.  Move it
	 * to higher-half to make it accessible to us.
	 */
	boot_params = (struct multiboot_params *) phys_to_virt (boot_params);

	struct multiboot_tag *tag;
	for (tag = mb2_first (); tag; tag = mb2_next (tag)) {
		switch (tag->type) {
		case MB2_TAG_FRAMEBUFFER:
			add_framebuffer ((struct multiboot_framebuffer *) tag);
		}
	}
}
