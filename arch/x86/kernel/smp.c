/* SPDX-License-Identifier: MIT */
#include <davix/acpi.h>
#include <davix/sched.h>
#include <davix/time.h>
#include <davix/smp.h>
#include <davix/printk.h>
#include <davix/panic.h>
#include <davix/page_alloc.h>
#include <davix/mm.h>
#include <asm/apic.h>
#include <asm/boot.h>
#include <asm/entry.h>
#include <asm/ioapic.h>
#include <asm/msr.h>
#include <asm/segment.h>
#include <asm/vector.h>

struct page *x86_smpboot_page;
extern char x86_smp_trampoline[];

extern unsigned long x86_gdt[] cpulocal;
extern struct x86_tss x86_tss cpulocal;

static void install_tss(void)
{
	unsigned long addr =
		(unsigned long) cpulocal_address(smp_self(), &x86_tss);

	unsigned long limit = sizeof(struct x86_tss) - 1;
	rdwr_cpulocal(x86_tss).iopb_offset = limit + 1;

	rdwr_cpulocal(x86_gdt[__GDT_TSS]) =
		((addr & 0xffffff) << 16)
		| ((addr & 0xff000000) << 32)
		| (limit & 0xffff)
		| (limit & 0xf0000) << 32
		| (0b1000UL << 52)
		| (0b10001001UL << 40);
	rdwr_cpulocal(x86_gdt[__GDT_TSS + 1]) = addr >> 32;

	asm volatile("ltr %%ax" : : "a"(GDT_TSS) : "memory");
}

struct logical_cpu *cpu_slots = NULL;
unsigned num_cpu_slots = 0;

static unsigned real_num_cpu_slots = 0;

unsigned *acpi_cpu_to_lapic_id;
static unsigned num_acpi_cpus = 0;

static void madt_calc_max_cpu(struct acpi_subtable_header *hdr, void *arg)
{
	(void) arg;

	unsigned no;
	unsigned acpino;
	switch(hdr->type) {
	case ACPI_MADT_TYPE_LOCAL_APIC: do {
		struct acpi_madt_local_apic *apic =
			(struct acpi_madt_local_apic *) hdr;
		no = apic->id;
		acpino = apic->processor_id;
		goto common;
	} while (0);
	case ACPI_MADT_TYPE_LOCAL_X2APIC: do {
		struct acpi_madt_local_x2apic *apic =
			(struct acpi_madt_local_x2apic *) hdr;
		no = apic->local_apic_id;
		acpino = apic->uid;
		goto common;
	} while(0);
	default:
		return;
	}

common:
	if(no + 1 > real_num_cpu_slots)
		real_num_cpu_slots = no + 1;
	if(acpino + 1 > num_acpi_cpus)
		num_acpi_cpus = acpino + 1;
}

static void madt_set_cpu_present(struct acpi_subtable_header *hdr, void *arg)
{
	(void) arg;

	bool present = 0;
	unsigned no;
	unsigned acpino;
	switch(hdr->type) {
	case ACPI_MADT_TYPE_LOCAL_APIC: do {
		struct acpi_madt_local_apic *apic =
			(struct acpi_madt_local_apic *) hdr;
		if(apic->lapic_flags & 0b11)
			present = 1;
		no = apic->id;
		acpino = apic->processor_id;
		goto common;
	} while (0);
	case ACPI_MADT_TYPE_LOCAL_X2APIC: do {
		struct acpi_madt_local_x2apic *apic =
			(struct acpi_madt_local_x2apic *) hdr;
		if(apic->lapic_flags & 0b11)
			present = 1;
		no = apic->local_apic_id;
		acpino = apic->uid;
		goto common;
	} while(0);
	default:
		return;
	}

common:;
	struct logical_cpu *cpu = &cpu_slots[no];
	cpu->possible = 1;
	cpu->present = present;

	acpi_cpu_to_lapic_id[acpino] = no;
}

static void madt_ioapics(struct acpi_subtable_header *hdr, void *arg)
{
	(void) arg;

	if(hdr->type != ACPI_MADT_TYPE_IO_APIC)
		return;

	struct acpi_madt_io_apic *ioapic =
		(struct acpi_madt_io_apic *) hdr;

	register_ioapic(ioapic->address, ioapic->global_irq_base);
}

extern char __cpulocal_start[], __cpulocal_end[];

struct logical_cpu *__smp_self cpulocal;

void arch_init_smp(void);
void arch_init_smp(void)
{
	struct acpi_table_madt *madt = NULL;
	acpi_get_table(ACPI_SIG_MADT, 0, (struct acpi_table_header **) &madt);

	/*
	 * As a rule, all systems nowadays have the MADT.
	 * Don't bother dealing with the very few unsupported machines.
	 */
	if(!madt)
		panic("arch_init_smp(): no MADT table found.");

	apic_init(madt);

	/*
	 * The BSP might not always be CPU#0...
	 */
	unsigned bsp = apic_read_id();
	debug("smp: CPU#%u is the BSP.\n", bsp);

	smp_self()->id = bsp;

	/*
	 * We need to know the highest logical CPU number to allocate the
	 * 'struct logical_cpu' array.
	 */
	acpi_parse_table(madt, madt_calc_max_cpu, NULL);
	if(!real_num_cpu_slots)
		panic("arch_init_smp(): no LAPIC entries in MADT.");

	info("Max CPUs: %u\n", real_num_cpu_slots);

	if(real_num_cpu_slots <= smp_self()->id)
		panic("arch_init_smp(): Too few CPU slots! (num_cpu_slots: %u, BSP: %u)\n");

	/* TODO: use something else than kmalloc() */
	cpu_slots = kmalloc(real_num_cpu_slots * sizeof(struct logical_cpu));
	if(!cpu_slots)
		panic("arch_init_smp(): Couldn't allocate struct logical_cpu array.");

	num_cpu_slots = real_num_cpu_slots;

	acpi_cpu_to_lapic_id = kmalloc(num_acpi_cpus * sizeof(unsigned));
	if(!acpi_cpu_to_lapic_id)
		panic("arch_init_smp(): Couldn't allocate ACPI CPU => LAPIC ID conversion array.");

	for_each_logical_cpu(cpu) {
		cpu->id = cpu - cpu_slots;	/* magic pointer arithmetic */
		cpu->online = 0;
		cpu->possible = 0;
		cpu->present = 0;
	}

	/*
	 * For the second time, parse the MADT. We now figure out which CPUs
	 * are present.
	 */
	acpi_parse_table(madt, madt_set_cpu_present, NULL);

	/* Set the BSP to online and setup cpu-local state */
	cpu_slots[smp_self()->id].online = 1;
	cpu_slots[smp_self()->id].cpulocal_offset = 0;
	__smp_self = &cpu_slots[smp_self()->id];
	write_msr(MSR_GSBASE, (unsigned long) &__smp_self);

	acpi_parse_table(madt, madt_ioapics, NULL);

	/*
	 * We are done with the MADT now.
	 */
	acpi_put_table((struct acpi_table_header *) madt);

	for_each_logical_cpu(cpu) {
		if(!cpu->possible)
			continue;
		if(cpu->online)
			continue;
		/* TODO: use something else than kmalloc() */
		void *mem = kmalloc(__cpulocal_end - __cpulocal_start);
		if(!mem)
			panic("x86/smp: Cannot allocate memory for CPU-local variables!");
		cpu->cpulocal_offset =
			(unsigned long) mem - (unsigned long) __cpulocal_start;
		memcpy(cpulocal_address(cpu, &x86_gdt[0]),
			&x86_gdt[0], GDT_SIZE);
		rdwr_cpulocal_on(cpu, __smp_self) = cpu;
	}

	for_each_logical_cpu(cpu) {
		if(!cpu->possible)
			continue;

		struct x86_tss *tss = cpulocal_address(cpu, &x86_tss);

		struct page *page = alloc_page(ALLOC_KERNEL, 0);
		if(!page)
			panic("x86/smp: Cannot allocate memory for double-fault stack!");
		tss->ist2 = page_to_virt(page) + PAGE_SIZE;

		page = alloc_page(ALLOC_KERNEL, 0);
		if(!page)
			panic("x86/smp: Cannot allocate memory for NMI stack!");
		tss->ist3 = page_to_virt(page) + PAGE_SIZE;
	}

	install_tss();

	if(!x86_smpboot_page)
		panic("x86/smp: No page for the smpboot trampoline!");

	if(page_to_phys(x86_smpboot_page) > 255 * PAGE_SIZE)
		panic("x86/smp: smpboot trampoline page too high!");

	debug("x86/smp: smpboot trampoline at %p\n",
		page_to_phys(x86_smpboot_page));

	if(!vmap_at(page_to_phys(x86_smpboot_page), PAGE_SIZE,
		PG_READABLE | PG_WRITABLE | PG_EXECUTABLE, PG_WRITEBACK,
		page_to_phys(x86_smpboot_page),
		page_to_phys(x86_smpboot_page) + PAGE_SIZE))
			panic("x86/smp: Couldn't vmap() the smpboot trampoline page!");
}

static spinlock_t smpboot_lock = SPINLOCK_INIT(smpboot_lock);
static struct logical_cpu *currently_booting;

static bool reached_c;

void x86_smp_fixup_gdt(unsigned long gdtptr);

void x86_smp_cpu_setupcode(void);
void x86_smp_cpu_setupcode(void)
{
	atomic_store(&reached_c, 1, memory_order_seq_cst);
	x86_smp_fixup_gdt((unsigned long) cpulocal_address(currently_booting, &x86_gdt[0]));
	write_msr(MSR_GSBASE, (unsigned long)
		cpulocal_address(currently_booting, &__smp_self));
	rdwr_cpulocal(preempt_disabled) = 1;
	info("\"Hello, World!\" from CPU#%u\n", smp_self()->id);
	install_tss();
	x86_load_idt_ap();
	smp_self()->online = 1;
	active_apic_interface->init_other();
	apic_configure();
	x86_idle_task();
}

void smp_boot_cpu(struct logical_cpu *cpu)
{
	info("smpboot: Booting CPU#%u...\n", cpu->id);
	if(current_task()->mm != &kernelmode_mm)
		panic("smpboot: smpboot can only be done on kernelmode MM!");

	spin_acquire(&smpboot_lock);

	if(cpu->online) {
		warn("smpboot: trying to boot already onlined CPU#%u\n",
			cpu->id);
		spin_release(&smpboot_lock);
		return;
	}

	reached_c = 0;
	currently_booting = cpu;

	unsigned long kernel_stack =
		page_to_virt(rdwr_cpulocal_on(cpu,
		idle_task)->arch_task_info.kernel_stack) + PAGE_SIZE;

	memcpy((void *) page_to_phys(x86_smpboot_page),
		x86_smp_trampoline, PAGE_SIZE);

	/*
	 * Fill in the '$imm32' in the 'movl $imm32, %eax'.
	 */
	*(u32 *) (page_to_phys(x86_smpboot_page)
		+ 2) = page_to_phys(x86_smpboot_page);

	u32 *sp = (u32 *) (page_to_phys(x86_smpboot_page) + PAGE_SIZE);
	*(--sp) = (kernel_stack >> 32);
	*(--sp) = kernel_stack;
	*(--sp) = read_cr0();
	*(--sp) = read_msr(MSR_EFER) & ~_EFER_LMA;
	*(--sp) = read_cr3();
	*(--sp) = read_cr4();

	apic_write_icr(APIC_ICR_INIT | APIC_ICR_LEVEL | APIC_ICR_ASSERT, cpu->id);
	apic_write_icr(APIC_ICR_INIT | APIC_ICR_LEVEL, cpu->id);

	udelay(500);

	for(int i = 0; i < 2; i++) {
		apic_write_icr(APIC_ICR_SIPI | page_to_pfn(x86_smpboot_page), cpu->id);

		udelay(500);

		apic_write(APIC_ESR, 0);
		u32 apic_error = apic_read(APIC_ESR);
		if(apic_error)
			panic("APIC error 0x%x when starting CPU#%u\n",
				apic_error, cpu->id);

		if(atomic_load(&reached_c, memory_order_seq_cst))
			goto done;
	}

	mdelay(10);
	if(!atomic_load(&reached_c, memory_order_seq_cst))
		panic("Failed to start CPU#%u", cpu->id);

done:
	do {
		asm volatile("pause");
	} while(!*(volatile bool *) &cpu->online);
	spin_release(&smpboot_lock);
}

void arch_smp_notify(struct logical_cpu *cpu)
{
	preempt_disable();
	apic_write_icr(APIC_ICR_FIXED | SMP_NOTIFY_VECTOR, cpu->id);
	preempt_enable();
}

void smp_send_nmi(struct logical_cpu *cpu)
{
	/*
	 * This is only called from panic(), so we have already
	 * disabled preemption.
	 */
	apic_write_icr(APIC_ICR_NMI, cpu->id);
}
