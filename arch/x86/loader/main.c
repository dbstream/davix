/* SPDX-License-Identifier: MIT */
#include <davix/elf_types.h>
#include <davix/printk_lib.h>
#include <davix/list.h>
#include <asm/cpuid.h>
#include <asm/msr.h>
#include <asm/page.h>
#include <asm/segment.h>
#include "memalloc.h"
#include "memmap.h"
#include "multiboot.h"
#include "printk.h"

static void stop(void)
{
	for(;;)
		__builtin_ia32_pause();
}

extern char __loader_start[], __loader_end[];

extern char mainkernel[], mainkernel_end[];

struct memmap orig_memmap;
struct memmap memmap;

int l5_paging_enable = 0;
static int nx_bit_enable = 0;
static int pdpe1g_enable = 0;
unsigned long max_phys_addr = 1UL << 32;

union cpuid_vendor {
	struct {
		u32 b, d, c;
	};
	char vendor_string[12];
};

static void query_cpu_functions(void)
{
	union cpuid_vendor vendor;
	u32 max, extmax;
	u32 a, b, c, d;

	do_cpuid(0, a, b, c, d);
	vendor = (union cpuid_vendor) {.b = b, .c = c, .d = d};
	max = a;

	do_cpuid(0x80000000, a, b, c, d);
	extmax = a;

	info("CPU: vendor \"%.12s\" max leaf 0x%x max ext. leaf 0x%x\n",
		vendor.vendor_string,
		max, extmax);

	if(max < 7)
		goto do_extended;

	do_cpuid_subleaf(7, 0, a, b, c, d);
	if(c & __ID_00000007_0_ECX_LA57) {
		info("CPU: supports LA57\n");
		l5_paging_enable = 1;
	}

do_extended:
	if(extmax < 0x80000001)
		return;
	do_cpuid(0x80000001, a, b, c, d);
	if(d & __ID_80000001_EDX_PDPE1G) {
		info("CPU: supports PDPE1G\n");
		pdpe1g_enable = 1;
	}
	if(d & __ID_80000001_EDX_NX) {
		info("CPU: supports NX bit\n");
		nx_bit_enable = 1;
		write_msr(MSR_EFER, read_msr(MSR_EFER) | _EFER_NXE);
	}

	if(extmax < 0x80000008)
		return;
	do_cpuid(0x80000008, a, b, c, d);
	max_phys_addr = 1UL << min(l5_paging_enable ? 52 : 45, a & 0xff);
	info("CPU: Physical address width %u => max_phys_addr=%p\n",
		a & 0xff, max_phys_addr);
}

unsigned long highest_mapped_address = (4 * P3E_SIZE);
static void *page_tables;

static inline void *pgtable_alloc(enum memmap_type type)
{
	void *ret = memalloc(PAGE_SIZE, PAGE_SIZE, type);
	if(!ret) {
		critical("pgtable: cannot allocate memory for page tables.\n");
		stop();
	}
	memset(ret, 0, PAGE_SIZE);
	return ret;
}

static void setup_pgtable(void)
{
	unsigned long cr3val = read_cr3();
	page_tables = (void *) PTE_ADDR(cr3val);
}

static p4e_t *get_p4e(void *pgt, unsigned long vaddr,
	enum memmap_type malloc_type)
{
	p5d_t p5d;
	p5e_t *p5e;
	p4d_t p4d;
	if(!l5_paging_enable) {
		p4d = (p4d_t) pgt;
		goto has_p4d;
	}

	p5d = (p5d_t) pgt;
	p5e = &p5d[P5E(vaddr)];
	if(!*p5e)
		*p5e = (force p5e_t) pgtable_alloc(malloc_type)
			| __PG_PRESENT | __PG_WRITE;
	p4d = (p4d_t) PTE_ADDR(*p5e);
has_p4d:
	return &p4d[P4E(vaddr)];
}

static p3e_t *get_p3e(void *pgt, unsigned long vaddr,
	enum memmap_type malloc_type)
{
	p4e_t *p4e = get_p4e(pgt, vaddr, malloc_type);
	if(!*p4e)
		*p4e = (force p4e_t) pgtable_alloc(malloc_type)
			| __PG_PRESENT | __PG_WRITE;
	p3d_t p3d = (p3d_t) PTE_ADDR(*p4e);
	return &p3d[P3E(vaddr)];
}

static p2e_t *get_p2e(void *pgt, unsigned long vaddr,
	enum memmap_type malloc_type)
{
	p3e_t *p3e = get_p3e(pgt, vaddr, malloc_type);
	if(!*p3e)
		*p3e = (force p3e_t) pgtable_alloc(malloc_type)
			| __PG_PRESENT | __PG_WRITE;
	p2d_t p2d = (p2d_t) PTE_ADDR(*p3e);
	return &p2d[P2E(vaddr)];
}

static p1e_t *get_p1e(void *pgt, unsigned long vaddr,
	enum memmap_type malloc_type)
{
	p2e_t *p2e = get_p2e(pgt, vaddr, malloc_type);
	if(!*p2e)
		*p2e = (force p2e_t) pgtable_alloc(malloc_type)
			| __PG_PRESENT | __PG_WRITE;
	p1d_t p1d = (p1d_t) PTE_ADDR(*p2e);
	return &p1d[P1E(vaddr)];
}

static void map_all_mem(void)
{
	unsigned long granularity = pdpe1g_enable ? P3E_SIZE : P2E_SIZE;
	struct memmap_entry *entry;
	list_for_each(entry, &orig_memmap.entry_list, list) {
		if(!(entry->type == MEMMAP_USABLE_RAM
			|| entry->type == MEMMAP_ACPI_DATA))
				continue;
		unsigned long start = align_down(entry->start, granularity);
		unsigned long end = align_up(entry->end, granularity);
		for(; start < end; start += granularity) {
			if(granularity == P3E_SIZE) {
				p3e_t *p3e = get_p3e(page_tables, start,
					MEMMAP_LOADER);
				*p3e = (force p3e_t) start | __PG_PRESENT
					| __PG_WRITE | __PG_SIZE;
			} else {
				p2e_t *p2e = get_p2e(page_tables, start,
					MEMMAP_LOADER);
				*p2e = (force p2e_t) start | __PG_PRESENT
					| __PG_WRITE | __PG_SIZE;
			}
			highest_mapped_address = max(highest_mapped_address,
				start + granularity);
		}
	}
#if 0
	/*
	 * Catch paging mess-ups early so that we avoid unfortunate commits to the kernel.
	 */
	list_for_each(entry, &memmap.entry_list, list) {
		if(entry->type != MEMMAP_USABLE_RAM)
			continue;
		for(unsigned long i = entry->start; i < entry->end; i += P2E_SIZE) {
			*(volatile unsigned char *) i = *(volatile unsigned char *) i ^ 0b10101010;
		}
	}
#endif
}

static int should_directmap(enum memmap_type type)
{
	return type == MEMMAP_USABLE_RAM
		|| type == MEMMAP_ACPI_DATA;
}

static void directmap(void *pgt, unsigned long offset, unsigned long start,
	unsigned long end, enum memmap_type malloc_type)
{
	pte_t flags = __PG_PRESENT | __PG_WRITE | __PG_GLOBAL;
	if(nx_bit_enable)
		flags |= __PG_NOEXEC;

	start = align_up(start, PAGE_SIZE);
	end = align_down(end, PAGE_SIZE);

	/*
	 * Fill in 4KiB pages up until the first 2MiB page.
	 */
	for(; start < min(end, align_up(start, P2E_SIZE)); start += PAGE_SIZE) {
		p1e_t *p1e = get_p1e(pgt, start + offset, malloc_type);
		*p1e = ((force p1e_t) start) | flags;
	}

	if(!pdpe1g_enable)
		goto no_1GiB_pages;

	/*
	 * Fill in 2MiB pages up until the first 1GiB page.
	 */
	for(; start < min(align_down(end, P2E_SIZE), align_up(start, P3E_SIZE)); start += P2E_SIZE) {
		p2e_t *p2e = get_p2e(pgt, start + offset, malloc_type);
		*p2e = ((force p2e_t) start) | flags | __PG_SIZE;
	}

	/*
	 * Fill in as many 1GiB pages as possible.
	 */
	for(; start < align_down(end, P3E_SIZE); start += P3E_SIZE) {
		p3e_t *p3e = get_p3e(pgt, start + offset, malloc_type);
		*p3e = ((force p3e_t) start) | flags | __PG_SIZE;
	}

no_1GiB_pages:
	/*
	 * Fill in remaining 2MiB pages.
	 */
	for(; start < align_down(end, P2E_SIZE); start += P2E_SIZE) {
		p2e_t *p2e = get_p2e(pgt, start + offset, malloc_type);
		*p2e = ((force p2e_t) start) | flags | __PG_SIZE;
	}

	/*
	 * Fill in remaining 4KiB pages.
	 */
	for(; start < end; start += PAGE_SIZE) {
		p1e_t *p1e = get_p1e(pgt, start + offset, malloc_type);
		*p1e = ((force p1e_t) start) | flags;
	}
}

/*
 * Return value: main kernel entry point.
 */
unsigned long loader_main(struct mb2_info *multiboot);
unsigned long loader_main(struct mb2_info *multiboot)
{
	uart_init();
	info("loader_main(): called with bootloader information at %p\n",
		multiboot);

	query_cpu_functions();

	memmap_init(&memmap);
	for_each_multiboot_tag(tag, multiboot) {
		if(tag->type != MB2_TAG_MEMMAP)
			continue;

		struct mb2_memmap *memmap_tag = (struct mb2_memmap *) tag;
		unsigned long entsize = memmap_tag->entry_size;
		if(entsize < sizeof(struct mb2_memmap_entry)) {
			critical("multiboot2-memmap: too small entry_size\n");  
			stop();
		}

		unsigned long entries =
			(memmap_tag->hdr.size - offsetof(struct mb2_memmap,
				entries[0]))/ entsize;

		info("Bootloader-supplied memory map, revision %u:\n",
			memmap_tag->revision);

		struct mb2_memmap_entry *entry;
		char tmp[32];
		for(unsigned long i = 0; i < entries; i++) {
			entry = (struct mb2_memmap_entry *)
				&memmap_tag->entries[i * entsize];

			enum memmap_type type;
			switch(entry->type) {
			case MB2_MEMMAP_RAM:
				type = MEMMAP_USABLE_RAM;
				snprintk(tmp, 32, "System RAM");
				break;
			case MB2_MEMMAP_RECLAIM:
				type = MEMMAP_ACPI_DATA;
				snprintk(tmp, 32, "ACPI Reclaimable Memory");
				break;
			case MB2_MEMMAP_ACPI_NVS:
				type = MEMMAP_ACPI_NVS;
				snprintk(tmp, 32, "ACPI NVS Memory");
				break;
			case MB2_MEMMAP_BADRAM:
				type = MEMMAP_RESERVED;
				snprintk(tmp, 32, "Defective System RAM");
				break;
			default:
				type = MEMMAP_RESERVED;
				snprintk(tmp, 32, "Reserved, type %u",
					entry->type);
			}

			unsigned long start = entry->addr;
			unsigned long end = entry->addr + entry->size;
			info("  [%2lu]: [mem %p - %p]: %s\n",
				i, start, end - 1, tmp);
			if(end > max_phys_addr) {
				end = max_phys_addr;
				if(end <= start)
					continue;
			}
			memmap_update_region(&memmap, start, end, type, 1);
		}
		break;
	}

	memmap_update_region(&memmap, 0, 0x1000, MEMMAP_RESERVED, 0);
	memmap_update_region(&memmap, 0xa0000, 0x100000, MEMMAP_RESERVED, 0);

	debug("Copying original memory map...\n");
	memmap_copy(&orig_memmap, &memmap);

	memmap_update_region(&memmap,
		(unsigned long) __loader_start,
		(unsigned long) __loader_end, MEMMAP_LOADER, 0);
	memmap_update_region(&memmap,
		(unsigned long) multiboot,
		(unsigned long) multiboot + multiboot->size,
		MEMMAP_LOADER, 0);

	for_each_multiboot_tag(tag, multiboot) {
		if(tag->type == MB2_TAG_MODULE) {
			struct mb2_module *module = (struct mb2_module *) tag;
			memmap_update_region(&memmap,
				module->mod_start,
				module->mod_end, MEMMAP_KERNEL, 0);
		}
	}

	setup_pgtable();
	map_all_mem();

	info("Loading main kernel...\n");

	struct elf64_hdr *elf_kernel = (struct elf64_hdr *) mainkernel;
	if(mainkernel_end - mainkernel < sizeof(*elf_kernel)) {
		critical("Bogus mainkernel: size is smaller than sizeof(struct elf64_hdr)\n");
		stop();
	}
	if(strncmp(elf_kernel->hdr.mag, ELF_MAGIC, 4) != 0) {
		critical("Bogus mainkernel: incorrect ELF magic\n");
		stop();
	}
	if(elf_kernel->hdr.elfclass != ELF_CLASS_64BIT) {
		critical("Bogus mainkernel: not Elf64\n");
		stop();
	}
	if(elf_kernel->hdr.data != ELF_ORDER_LITTLEENDIAN) {
		critical("Bogus mainkernel: not little-endian\n");
		stop();
	}
	if(elf_kernel->hdr.type != ET_EXEC) {
		critical("Bogus mainkernel: not an executable\n");
		stop();
	}
	if(elf_kernel->hdr.machine != EM_AMD64) {
		critical("Bogus mainkernel: not for x86\n",
			elf_kernel->hdr.machine);
		stop();
	}

	unsigned phnum = elf_kernel->phnum;
	unsigned long phentsize = elf_kernel->phentsize;
	unsigned long phoff = elf_kernel->phoff;

	if(phoff > mainkernel_end - mainkernel) {
		critical("Bogus mainkernel: phoff outside of kernel image (phoff: %p)\n",
			phoff);
		stop();
	}

	if(phoff + phnum * phentsize > mainkernel_end - mainkernel) {
		critical("Bogus mainkernel: too many/big segments (phnum: %u, phentsize: %u)\n",
			phnum,
			phentsize);
		stop();
	}

	void *mainkernel_pgtables = pgtable_alloc(MEMMAP_KERNEL);

	for(unsigned i = 0; i < phnum; i++) {
		struct elf64_phdr *phdr = (struct elf64_phdr *)
				(mainkernel + phoff + i * phentsize);

		if(phdr->type != PT_LOAD)
			continue;

		if(phdr->memsz == 0)
			continue;

		debug("Loading kernel segment [%u] at [virt %p - %p] (%c%c%c)...\n",
			i, phdr->vaddr, phdr->vaddr + phdr->memsz - 1,
			phdr->flags & PF_R ? 'R' : '-',
			phdr->flags & PF_W ? 'W' : '-',
			phdr->flags & PF_X ? 'X' : '-');

		if(phdr->filesz > phdr->memsz) {
			critical("Bogus mainkernel: segment with FileSiz > MemSiz\n");
			stop();
		}

		if(phdr->filesz && (phdr->offset > mainkernel_end - mainkernel
			|| phdr->offset + phdr->filesz > mainkernel_end - mainkernel)) {
			critical("Bogus mainkernel: segment out of kernel image\n");
			stop();
		}

		if(phdr->vaddr < 0xffffffff80000000UL) {
			critical("This loader does not support loading the kernel below -2GiB\n");
			stop();
		}

		if(phdr->vaddr & (PAGE_SIZE - 1)) {
			critical("Segments need to be page-aligned\n");
			stop();
		}

		if(phdr->memsz & (PAGE_SIZE - 1)) {
			critical("Segments need to be page-aligned\n");
			stop();
		}

		if(phdr->vaddr + phdr->memsz < phdr->vaddr) {
			critical("Bogus mainkernel: integer overflow\n");
			stop();
		}

		void *mem_for_segment = memalloc(phdr->memsz, PAGE_SIZE, MEMMAP_KERNEL);
		if(!mem_for_segment) {
			critical("Failed to allocate memory for segment [%u]\n", i);
			stop();
		}

		phdr->paddr = (unsigned long) mem_for_segment;
		memcpy(mem_for_segment, mainkernel + phdr->offset, phdr->filesz);
		memset(mem_for_segment + phdr->filesz, 0, phdr->memsz - phdr->filesz);

		for(unsigned long j = 0; j < phdr->memsz; j += PAGE_SIZE) {
			p1e_t *p1e = get_p1e(mainkernel_pgtables, phdr->vaddr + j, MEMMAP_KERNEL);
			p1e_t flags = __PG_PRESENT | __PG_GLOBAL;
			if(phdr->flags & PF_W)
				flags |= __PG_WRITE;
			if(phdr->flags & PF_X) {
				if(phdr->flags & PF_W)
					warn("Loading W&X segment [%u]...\n", i);
			} else if(nx_bit_enable) {
				flags |= __PG_NOEXEC;
			}
			*p1e = (force p1e_t) (phdr->paddr + j) | flags;
		}
	}

	unsigned long hhdm_base = l5_paging_enable
		? 0xff00000000000000UL : 0xffff800000000000UL;

	debug("HHDM base: %p\n", hhdm_base);

	struct list *pos = orig_memmap.entry_list.next;
	unsigned long start = 0;
	unsigned long end = 0;
	for(;;) {
		if(pos == &orig_memmap.entry_list) {
			if(start != end)
				directmap(mainkernel_pgtables, hhdm_base,
					start, end, MEMMAP_KERNEL);
			break;
		}
		struct memmap_entry *entry = container_of(pos,
			struct memmap_entry, list);
		if(should_directmap(entry->type)) {
			if(end == entry->start) {
				end = entry->end;
			} else {
				if(start != end)
					directmap(mainkernel_pgtables, hhdm_base,
						start, end, MEMMAP_KERNEL);
				start = entry->start;
				end = entry->end;
			}
		} else {
			if(start != end)
				directmap(mainkernel_pgtables, hhdm_base,
					start, end, MEMMAP_KERNEL);
			start = 0;
			end = 0;
		}
		pos = pos->next;
	}

	/*
	 * Copy the page table entries mapping the kernel loader.
	 */
	memcpy(mainkernel_pgtables, page_tables, 256 * sizeof(pte_t));
	write_cr3((unsigned long) mainkernel_pgtables);


	struct boot_struct *boot_struct =
		(struct boot_struct *) 0xffffffff80000000UL;

	if(strcmp(boot_struct->bootstruct_magic, BOOTSTRUCT_MAGIC) != 0) {
		critical("Bogus kernel: wrong BOOTSTRUCT_MAGIC\n");
		stop();
	}


	u64 orig_protocol_version = boot_struct->protocol_version;
	u64 protocol_version = min(orig_protocol_version, BOOTPROTOCOL_VERSION_1_0_0);
	boot_struct->protocol_version = protocol_version;

	list_init(&boot_struct->memmap_entries);
	list_move_all(&boot_struct->memmap_entries, &memmap.entry_list);

	boot_struct->l5_paging_enable = l5_paging_enable;

	int has_xsdt = 0;
	for_each_multiboot_tag(tag, multiboot) {
		if(tag->type == MB2_TAG_RSDP) {
			if(has_xsdt)
				continue;
			struct mb2_rsdp *rsdp = (struct mb2_rsdp *) tag;
			boot_struct->acpi_rsdp = rsdp->ac_pRSDT;
		} else if(tag->type == MB2_TAG_XSDP) {
			has_xsdt = 1;
			struct mb2_rsdp *rsdp = (struct mb2_rsdp *) tag;
			boot_struct->acpi_rsdp = rsdp->ac_pXSDT;
		} else if(tag->type == MB2_TAG_MODULE) {
			struct mb2_module *module = (struct mb2_module *) tag;
			boot_struct->initrd_start = module->mod_start;
			boot_struct->initrd_size =
				module->mod_end - module->mod_start;
		}
	}

	info("Kernel entry point: %p\n", elf_kernel->entry);
	info("Booting %s, protocol version %u.%u.%u...\n",
		boot_struct->kernel,
		BOOTPROTOCOL_VERSION_MAJOR(orig_protocol_version),
		BOOTPROTOCOL_VERSION_MINOR(orig_protocol_version),
		BOOTPROTOCOL_VERSION_PATCH(orig_protocol_version));

	uart_fillin_boot_struct(boot_struct);
	strcpy(boot_struct->bootloader_name, "Included multiboot2-based Davix loader");
	return elf_kernel->entry;
}
