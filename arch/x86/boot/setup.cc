/**
 * Kernel initialization on x86.
 * Copyright (C) 2025-present  dbstream
 */
#include <stddef.h>
#include <string.h>
#include <asm/asm.h>
#include <asm/cpufeature.h>
#include <asm/idt.h>
#include <asm/irql.h>
#include <asm/kmap_fixed.h>
#include <asm/pcpu_init.h>
#include <asm/percpu.h>
#include <davix/acpisetup.h>
#include <davix/early_alloc.h>
#include <davix/efi_types.h>
#include <davix/page.h>
#include <davix/panic.h>
#include <davix/printk.h>
#include <davix/start_kernel.h>
#include <dsl/align.h>
#include <dsl/interval.h>
#include <dsl/minmax.h>
#include "multiboot.h"

static multiboot_params *boot_params;
static uintptr_t load_offset;

constexpr static struct {
	struct iterator {
		multiboot_tag *m_ptr;

		constexpr
		iterator (multiboot_tag *ptr)
			: m_ptr (ptr)
		{}

		constexpr multiboot_tag *
		operator* (void) const
		{
			return m_ptr;
		}

		constexpr bool
		operator!= (const iterator &other) const
		{
			return m_ptr != other.m_ptr;
		}

		iterator &
		operator++ (void)
		{
			uintptr_t as_uintptr = reinterpret_cast<uintptr_t> (m_ptr);
			m_ptr = reinterpret_cast<multiboot_tag *> (
				dsl::align_up (as_uintptr + m_ptr->size, 8)
			);
			if (m_ptr->type == MB2_TAG_END)
				m_ptr = nullptr;
			return *this;
		}
	};

	iterator
	begin (void) const
	{
		return iterator (
			reinterpret_cast<multiboot_tag *> (
				reinterpret_cast<uintptr_t> (boot_params) + 8
			)
		);
	}

	iterator
	end (void) const
	{
		return iterator (nullptr);
	}
} mb2_tags;

static multiboot_memmap *memmap_tag;
static bool memmap_is_efi;

static inline size_t memmap_count;

enum memory_type {
	MEM_USABLE,
	MEM_ACPI_RECLAIM,
	MEM_BOOT_SERVICES,
	MEM_RUNTIME_SERVICES,
	MEM_PERSISTENT_RAM,
	MEM_SPECIAL_PURPOSE,
	MEM_ACPI_NVS,
	MEM_RESERVED
};

static inline bool
should_allocate (memory_type type)
{
	return type == MEM_USABLE;
}

static inline bool
should_map (memory_type type)
{
	return type == MEM_USABLE || type == MEM_ACPI_RECLAIM
			|| type == MEM_BOOT_SERVICES;
}

static inline const char *
memory_type_string (memory_type type)
{
	switch (type) {
	case MEM_USABLE:		return "usable RAM";
	case MEM_ACPI_RECLAIM:		return "ACPI reclaimable";
	case MEM_BOOT_SERVICES:		return "boot services";
	case MEM_RUNTIME_SERVICES:	return "runtime services";
	case MEM_PERSISTENT_RAM:	return "persistent RAM";
	case MEM_SPECIAL_PURPOSE:	return "special purpose";
	case MEM_ACPI_NVS:		return "ACPI NVS memory";
	case MEM_RESERVED:		return "reserved";
	default:			return "(invalid)";
	}
}

static inline void *
memmap_entry_pointer (size_t idx)
{
	return reinterpret_cast<void *> (
		reinterpret_cast<uintptr_t> (
			memmap_tag
		) + sizeof (*memmap_tag) + idx * memmap_tag->entry_size
	);
}

static inline uintptr_t
memmap_entry_start (void *ptr)
{
	if (memmap_is_efi) {
		efi_memory_descriptor *desc =
			reinterpret_cast<efi_memory_descriptor *> (ptr);
		return desc->phys_start;
	} else {
		multiboot_memmap_entry *entry =
			reinterpret_cast<multiboot_memmap_entry *> (ptr);
		return entry->start;
	}
}

static inline size_t
memmap_entry_size (void *ptr)
{
	if (memmap_is_efi) {
		efi_memory_descriptor *desc =
			reinterpret_cast<efi_memory_descriptor *> (ptr);
		return desc->num_pages * PAGE_SIZE;
	} else {
		multiboot_memmap_entry *entry =
			reinterpret_cast<multiboot_memmap_entry *> (ptr);
		return entry->size;
	}
}

static inline memory_type
memmap_entry_type (void *ptr)
{
	if (memmap_is_efi) {
		efi_memory_descriptor *desc =
			reinterpret_cast<efi_memory_descriptor *> (ptr);
		switch (desc->type) {
		case EFI_MEMORY_LOADER_CODE:
		case EFI_MEMORY_LOADER_DATA:
			return MEM_USABLE;
		case EFI_MEMORY_BOOT_SERVICES_CODE:
		case EFI_MEMORY_BOOT_SERVICES_DATA:
			return MEM_BOOT_SERVICES;
		case EFI_MEMORY_RT_SERVICES_CODE:
		case EFI_MEMORY_RT_SERVICES_DATA:
			return MEM_RUNTIME_SERVICES;
		case EFI_MEMORY_ACPI_RECLAIM:
			return MEM_ACPI_RECLAIM;
		case EFI_MEMORY_CONVENTIONAL_RAM:
			if (desc->attribute & EFI_MEMORY_SP)
				return MEM_SPECIAL_PURPOSE;
			if (desc->attribute & EFI_MEMORY_NV)
				return MEM_PERSISTENT_RAM;
			return MEM_USABLE;
		case EFI_MEMORY_ACPI_NVS:
			return MEM_ACPI_NVS;
		case EFI_MEMORY_PERSISTENT_RAM:
			return MEM_PERSISTENT_RAM;
		default:
			return MEM_RESERVED;
		}
	} else {
		multiboot_memmap_entry *entry =
			reinterpret_cast<multiboot_memmap_entry *> (ptr);
		switch (entry->type) {
		case MB2_MEMMAP_USABLE:
			return MEM_USABLE;
		case MB2_MEMMAP_ACPI_RECLAIM:
			return MEM_ACPI_RECLAIM;
		case MB2_MEMMAP_ACPI_NVS:
			return MEM_ACPI_NVS;
		default:
			return MEM_RESERVED;
		}
	}
}

static uintptr_t max_supported_ram;

/**
 * Memory allocation:  we do top-down watermark memory allocation and keep track
 * of a list of "blockers" - memory regions that cannot be allocated from (the
 * kernel and the boot information).
 */
static constexpr int max_blockers = 3;
static int num_blockers = 0;
static dsl::start_end<uintptr_t> blockers[max_blockers];

static void
block_memory (uintptr_t start, size_t size)
{
	if (num_blockers >= max_blockers)
		panic ("block_memory: too many reserved regions already");

	blockers[num_blockers++] = { start, start + size };
}

static inline bool
blocked (dsl::start_end<uintptr_t> blocker, uintptr_t x, size_t size)
{
	return blocker.start () < x + size && blocker.end () > x;
}

static size_t memmap_alloc_idx;
static uintptr_t memmap_alloc_wmark;
static constexpr uintptr_t min_alloc_addr = 0x1000000UL;

static bool use_early_alloc = false;

/**
 * Allocate a 'size'-sized and 'size'-aligned block of physical memory.  'size'
 * must be 2^N for some integer N.
 */
static uintptr_t
alloc_from_memmap (size_t size)
{
	if (use_early_alloc) {
		uintptr_t phys = early_alloc_phys (size, size);
		if (!phys)
			panic ("OOM in early_alloc_phys");
		return phys;
	}

	for (;;) {
		void *eptr = memmap_entry_pointer (memmap_alloc_idx);
		if (!should_allocate (memmap_entry_type (eptr))) {
			if (!memmap_alloc_idx)
				panic ("OOM in setup_memory");
			memmap_alloc_idx--;
			continue;
		}

		uintptr_t estart = memmap_entry_start (eptr);
		uintptr_t eend = estart + memmap_entry_size (eptr);

		if (memmap_alloc_wmark > eend)
			memmap_alloc_wmark = eend;

		if (memmap_alloc_wmark < min_alloc_addr + size)
			panic ("OOM in setup_memory");

		uintptr_t x = dsl::align_down (memmap_alloc_wmark - size, size);
		if (x < estart) {
			if (!memmap_alloc_idx)
				panic ("OOM in setup_memory");
			memmap_alloc_idx--;
			continue;
		}

		bool was_blocked = false;
		for (int i = 0; i < num_blockers; i++) {
			if (blocked (blockers[i], x, size)) {
				was_blocked = true;
				memmap_alloc_wmark = blockers[i].start ();
				break;
			}
		}
		if (!was_blocked) {
			memmap_alloc_wmark = x;
			return x;
		}
	}
}

static void
setup_free_memory (void)
{
	use_early_alloc = true;
	for (;; memmap_alloc_idx--) {
		void *eptr = memmap_entry_pointer (memmap_alloc_idx);
		if (!should_allocate (memmap_entry_type (eptr))) {
			if (memmap_alloc_idx)
				continue;
			return;
		}

		uintptr_t estart = memmap_entry_start (eptr);
		uintptr_t eend = estart + memmap_entry_size (eptr);

		if (memmap_alloc_wmark < eend)
			eend = memmap_alloc_wmark;

		while (estart < eend) {
			uintptr_t x = estart, e = estart;
			for (int i = 0; i < num_blockers; i++) {
				if (blockers[i].start () >= eend)
					continue;
				if (blockers[i].end () > x) {
					e = blockers[i].start ();
					x = blockers[i].end ();
				}
			}

			x = dsl::align_up (x, PAGE_SIZE);
			eend = dsl::align_down (eend, PAGE_SIZE);
			early_free_phys (x, eend - x);
			eend = e;
		}

		if (!memmap_alloc_idx)
			return;
	}
}

static uintptr_t root_pgtable;

template<int level>
static volatile uint64_t *
get_next_pte (volatile uint64_t *entry, uintptr_t addr)
{
	constexpr int kmap_idx = KMAP_FIXED_IDX_P1D + (level - 1);
	constexpr uintptr_t kmap_addr = kmap_fixed_address (kmap_idx);
	volatile uint64_t *const table = (volatile uint64_t *) kmap_addr;

	static uintptr_t curr_mapped = 0;
	uint64_t value = *entry;

	if (value) {
		value &= __PG_ADDR_MASK;
		if (curr_mapped != value) {
			kmap_fixed_install (kmap_idx, make_pte_k (value, true, true, false));
			curr_mapped = value;
		}
	} else {
		value = alloc_from_memmap (PAGE_SIZE);
		kmap_fixed_install (kmap_idx, make_pte_k (value, true, true, false));
		curr_mapped = value;

		for (int i = 0; i < 512; i++)
			table[i] = 0;

		*entry = value | __PG_PRESENT | __PG_WRITE;
	}

	return table + __pgtable_index<level> (addr);
}

static volatile uint64_t *
get_p4e (uintptr_t addr)
{
	if (has_feature (FEATURE_LA57)) {
		volatile uint64_t *const table = (volatile uint64_t *)
				kmap_fixed_address (KMAP_FIXED_IDX_P5D);

		return get_next_pte<4> (table + __pgtable_index<5> (addr), addr);
	}

	volatile uint64_t *const table = (volatile uint64_t *)
			kmap_fixed_address (KMAP_FIXED_IDX_P4D);
	return table + __pgtable_index<4> (addr);
}

static volatile uint64_t *
get_p3e (uintptr_t addr)
{
	return get_next_pte<3> (get_p4e (addr), addr);
}

static volatile uint64_t *
get_p2e (uintptr_t addr)
{
	return get_next_pte<2> (get_p3e (addr), addr);
}

static volatile uint64_t *
get_p1e (uintptr_t addr)
{
	return get_next_pte<1> (get_p2e (addr), addr);
}

static inline uintptr_t
addr_end (uintptr_t addr, uintptr_t end, uintptr_t align)
{
	addr = dsl::align_up (addr, align);
	if (!end)
		return addr;
	return dsl::min (addr, end);
}

static void
identity_map (uintptr_t virt, uintptr_t phys, size_t size,
		uint64_t flags, int maxhuge)
{
	uint64_t hugeflags = flags;
	if (flags & __PG_PAT)
		hugeflags |= __PG_PAT_HUGE;
	else
		hugeflags |= __PG_HUGE;


	uintptr_t x, end = dsl::align_up (virt + size, PAGE_SIZE);
	virt = dsl::align_down (virt, PAGE_SIZE);
	phys = dsl::align_down (phys, PAGE_SIZE);

	if (end && dsl::align_down (end, P2D_SIZE) < virt)
		maxhuge = dsl::min (maxhuge, 2);
	if (end && dsl::align_down (end, P1D_SIZE) < virt)
		maxhuge = dsl::min (maxhuge, 1);

	if (maxhuge <= 1)
		goto tail_p1d;

	x = addr_end (virt, end, P1D_SIZE);
	for (; virt != x; phys += PAGE_SIZE, virt += PAGE_SIZE)
		*get_p1e (virt) = phys | flags;

	if (maxhuge <= 2)
		goto tail_p2d;

	x = addr_end (virt, end, P2D_SIZE);
	for (; virt != x; phys += P1D_SIZE, virt += P1D_SIZE)
		*get_p2e (virt) = phys | hugeflags;

	x = dsl::align_down (end, P2D_SIZE);
	for (; virt != x; phys += P2D_SIZE, virt += P2D_SIZE)
		*get_p3e (virt) = phys | hugeflags;
tail_p2d:
	x = dsl::align_down (end, P1D_SIZE);
	for (; virt != x; phys += P1D_SIZE, virt += P1D_SIZE)
		*get_p2e (virt) = phys | hugeflags;
tail_p1d:
	for (; virt != end; phys += PAGE_SIZE, virt += PAGE_SIZE)
		*get_p1e (virt) = phys | flags;
}

static void
setup_page_struct (uintptr_t start, uintptr_t end)
{
	start = dsl::align_down ((uintptr_t) phys_to_page (start), PAGE_SIZE);
	end = dsl::align_up ((uintptr_t) phys_to_page (end), PAGE_SIZE);

	for (; start != end; start += PAGE_SIZE) {
		volatile uint64_t *entry = get_p1e (start);
		if (*entry != 0)
			continue;

		uintptr_t value = alloc_from_memmap (PAGE_SIZE);
		pte_t pte = make_pte_k (value, true, true, false);
		volatile unsigned char *p = (volatile unsigned char *)
				kmap_fixed_install (KMAP_FIXED_IDX_SETUP_TMP, pte);
		for (uintptr_t i = 0; i < PAGE_SIZE; i++)
			p[i] = 0;
		*entry = pte.value;
	}
}

extern "C" char __kernel_start[];
extern "C" char __head_start[];
extern "C" char __head_end[];
extern "C" char __text_start[];
extern "C" char __text_end[];
extern "C" char __rodata_start[];
extern "C" char __rodata_end[];
extern "C" char __data_start[];
extern "C" char __data_end[];
extern "C" char __percpu_start[];
extern "C" char __percpu_end[];
extern "C" char __kernel_end[];

#define KERNEL_START 0xffffffff80000000UL
#define sym_addr(name) (load_offset + ((uintptr_t) (name)) - KERNEL_START)

uintptr_t HHDM_OFFSET;
Page *page_map;
uintptr_t USER_VM_FIRST;
uintptr_t USER_VM_LAST;
uintptr_t KERNEL_VM_FIRST;
uintptr_t KERNEL_VM_LAST;

static inline void
setup_memory (void)
{
	if (has_feature (FEATURE_LA57)) {
		max_supported_ram = 0x80000000000000UL;
		HHDM_OFFSET = 0xff00000000000000UL;
		page_map = (Page *) 0xff80000000000000UL;
		USER_VM_FIRST = 0;
		USER_VM_LAST = 0xffffffffffffffUL;
		KERNEL_VM_FIRST = 0xff90000000000000UL;
		KERNEL_VM_LAST = 0xffcfffffffffffffUL;
	} else {
		max_supported_ram = 0x400000000000UL;
		HHDM_OFFSET = 0xffff800000000000UL;
		page_map = (Page *) 0xffffc00000000000UL;
		USER_VM_FIRST = 0;
		USER_VM_LAST = 0x7fffffffffffUL;
		KERNEL_VM_FIRST = 0xffffe00000000000UL;
		KERNEL_VM_LAST = 0xffffefffffffffffUL;
	}

	max_supported_ram = dsl::min (max_supported_ram, x86_max_phys_addr);
	asm volatile ("" : "+r" (HHDM_OFFSET), "+r" (page_map) :: "memory");

	multiboot_memmap *mbi_memmap = nullptr, *efi_memmap = nullptr;
	for (multiboot_tag *tag : mb2_tags) {
		if (!mbi_memmap && tag->type == MB2_TAG_MEMMAP)
			mbi_memmap = reinterpret_cast<multiboot_memmap *> (tag);
		else if (!efi_memmap && tag->type == MB2_TAG_EFI_MEMMAP)
			efi_memmap = reinterpret_cast<multiboot_memmap *> (tag);
	}

	if (efi_memmap) {
		memmap_tag = efi_memmap;
		memmap_is_efi = true;
	} else if (mbi_memmap) {
		memmap_tag = mbi_memmap;
		memmap_is_efi = false;
	} else {
		panic ("No memory map!");
	}

	memmap_count = (memmap_tag->size - sizeof (*memmap_tag)) / memmap_tag->entry_size;
	if (!memmap_count)
		panic ("Empty memory map!");

	memmap_alloc_idx = memmap_count - 1;
	memmap_alloc_wmark = max_supported_ram;

	printk (PR_INFO "Memory map:\n");
	for (size_t i = 0; i < memmap_count; i++) {
		void *entry = memmap_entry_pointer (i);
		uintptr_t start = memmap_entry_start (entry);
		uintptr_t end = start + memmap_entry_size (entry);
		memory_type type = memmap_entry_type (entry);

		printk ("  [ %#16lx ... %#16lx ]  %s\n",
				start,
				end - 1,
				memory_type_string (type));
	}

	root_pgtable = alloc_from_memmap (PAGE_SIZE);
	uintptr_t root_vaddr;
	if (has_feature (FEATURE_LA57)) {
		kmap_fixed_install (KMAP_FIXED_IDX_P5D, make_pte_k (root_pgtable, true, true, false));
		root_vaddr = kmap_fixed_address (KMAP_FIXED_IDX_P5D);
	} else {
		kmap_fixed_install (KMAP_FIXED_IDX_P4D, make_pte_k (root_pgtable, true, true ,false));
		root_vaddr = kmap_fixed_address (KMAP_FIXED_IDX_P4D);
	}

	volatile uint64_t *const table = (volatile uint64_t *) root_vaddr;
	for (int i = 0; i < 512; i++)
		table[i] = 0;

	uint64_t rx = make_pte_k (0, true, false, true).value;
	uint64_t r = make_pte_k (0, true, false, false).value;
	uint64_t rw = make_pte_k (0, true, true, false).value;
	identity_map ((uintptr_t) __head_start, sym_addr (__head_start),
			__head_end - __head_start, rw, 2);
	identity_map ((uintptr_t) __text_start, sym_addr (__text_start),
			__text_end - __text_start, rx, 2);
	identity_map ((uintptr_t) __rodata_start, sym_addr (__rodata_start),
			__rodata_end - __rodata_start, r, 2);
	identity_map ((uintptr_t) __data_start, sym_addr (__data_start),
			__percpu_end - __data_start, rw, 2);

	int maxhuge = has_feature (FEATURE_PDPE1GB) ? 3 : 2;
	uintptr_t mstart = 0, mend = 0;
	for (size_t i = 0; i < memmap_count; i++) {
		void *eptr = memmap_entry_pointer (i);
		if (!should_map (memmap_entry_type (eptr)))
			continue;
		uintptr_t estart = memmap_entry_start (eptr);
		uintptr_t eend = estart + memmap_entry_size (eptr);
		if (estart >= max_supported_ram)
			continue;
		if (eend >= max_supported_ram)
			eend = max_supported_ram;
		else if (eend <= estart)
			continue;
		if (estart == mend) {
			mend = eend;
			continue;
		}
		if (mstart != mend) {
			identity_map (HHDM_OFFSET + mstart, mstart,
					mend - mstart, rw, maxhuge);
			setup_page_struct (mstart, mend);
		}
		mstart = estart;
		mend = eend;
	}
	if (mstart != mend) {
		identity_map (HHDM_OFFSET + mstart, mstart,
				mend - mstart, rw, maxhuge);
		setup_page_struct (mstart, mend);
	}

	*get_p2e (KMAP_FIXED_BASE) = sym_addr (__kmap_fixed_page) | __PG_PRESENT | __PG_WRITE;
	write_cr3 (root_pgtable);
	boot_params = (multiboot_params *) phys_to_virt ((uintptr_t) boot_params);
	memmap_tag = (multiboot_memmap *) phys_to_virt ((uintptr_t) memmap_tag);

	setup_free_memory ();
}

static void
setup_early_acpi (void)
{
	multiboot_tag *rsdp1 = nullptr, *rsdp2 = nullptr;
	for (multiboot_tag *tag : mb2_tags) {
		switch (tag->type) {
		case MB2_TAG_RSDPv1:
			rsdp1 = tag;
			break;
		case MB2_TAG_RSDPv2:
			rsdp2 = tag;
			break;
		default:
			break;
		}
	}

	if (rsdp2)
		rsdp1 = rsdp2;
	if (!rsdp1)
		panic ("No ACPI RSD PTR was provided by the bootloader.");

	acpi_set_rsdp (virt_to_phys ((uintptr_t) rsdp1) + 8);

	/**
	 * Setup early table access.
	 * NB: this uses the boot_pagetables memory as the temporary buffer.
	 */
	uacpi_setup_early_table_access ((void *) (KERNEL_START + 0x4000), 0x2000UL);
}

static void
setup_boot_console (void)
{
	multiboot_framebuffer *mfb = nullptr;

	for (multiboot_tag *tag : mb2_tags) {
		switch (tag->type) {
		case MB2_TAG_FRAMEBUFFER:
			mfb = (multiboot_framebuffer *) tag;
			break;
		default:
			break;
		}
	}

	if (!mfb)
		return;

	uintptr_t addr = mfb->framebuffer_addr;
	size_t nbytes = (size_t) mfb->height * (size_t) mfb->pitch;
	printk (PR_INFO "framebuffer:  %#tx (%dx%dx%d, %zu bytes)\n",
			addr, mfb->width, mfb->height, mfb->bpp, nbytes);
	if (mfb->framebuffer_type != MB2_FRAMEBUFFER_COLOR) {
		printk (PR_WARN "framebuffer:  type is %d; ignoring\n",
				mfb->framebuffer_type);
		return;
	}

	if (addr + nbytes > max_supported_ram) {
		printk (PR_WARN "framebuffer:  address is too high, ignoring\n");
		return;
	}

	uintptr_t offset = addr & (PAGE_SIZE - 1);
	addr -= offset;
	nbytes += offset;

	uintptr_t virt = phys_to_virt (addr);
	uint64_t flags = make_pte_k (0, true, true, false).value | PG_WC;
	identity_map (virt, addr, nbytes, flags, 2);
	memset ((void *) virt, 0, nbytes);
}

extern "C" void
x86_start_kernel (multiboot_params *params, uintptr_t offset)
{
	boot_params = params;
	load_offset = offset;
	asm volatile ("movq %%gs:0, %0" : "=r" (pcpu_detail::offsets[0]) :: "memory");

	x86_setup_idt ();
	__write_irql_high (0 | __IRQL_NONE_PENDING);
	__write_irql_dispatch (0 | __IRQL_NONE_PENDING);
	call_pcpu_constructors_for (0);

	printk (PR_NOTICE "x86_start_kernel: load_offset is 0x%tx  multiboot_params is 0x%tx\n",
			load_offset,
			reinterpret_cast<uintptr_t> (boot_params));
	for (multiboot_tag *tag : mb2_tags) {
		printk (PR_INFO "multiboot:  tag  type %2u   size %#6x\n",
				tag->type,
				tag->size);
	}

	cpufeature_init ();
	printk (PR_NOTICE "CPU: %s %s\n", cpu_brand_string, cpu_model_string);
	printk (PR_NOTICE "CPU: maxphyaddr=0x%tx\n", x86_max_phys_addr);

	block_memory (sym_addr (__kernel_start), __kernel_end - __kernel_start);
	block_memory ((uintptr_t) boot_params, boot_params->size);
	block_memory (0, PAGE_SIZE);
	setup_memory ();
	setup_boot_console ();
	setup_early_acpi ();

	start_kernel ();
	for (;;)
		asm volatile ("hlt");
}
