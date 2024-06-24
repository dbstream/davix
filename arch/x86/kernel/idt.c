/**
 * Interrupt Descriptor Table (IDT) and related state.
 * Copyright (C) 2024  dbstream
 *
 * also manage TSS here as it is related to stack switching on exception entry
 */
#include <davix/cpuset.h>
#include <davix/initmem.h>
#include <davix/panic.h>
#include <davix/printk.h>
#include <davix/string.h>
#include <asm/cpulocal.h>
#include <asm/entry.h>
#include <asm/gdt.h>
#include <asm/interrupt.h>
#include <asm/sections.h>
#include <asm/trap.h>

extern unsigned long percpu_GDT[];

struct __attribute__((packed)) idt_entry {
	unsigned short	offset0;
	unsigned short	cs;
	unsigned char	ist;
	unsigned char	flags;
	unsigned short	offset1;
	unsigned int	offset2;
	unsigned int	reserved;
};

__DATA_PAGE_ALIGNED
static __attribute__((aligned (0x1000))) struct idt_entry idt_table[256];

__INIT_TEXT
static void
set_idt_entry (int idx, void *handler, int ist, int dpl)
{
	idt_table[idx].offset0 = (unsigned long) handler;
	idt_table[idx].cs = __KERNEL_CS;
	idt_table[idx].ist = ist;
	idt_table[idx].flags = 0x8e | (dpl << 5);
	idt_table[idx].offset1 = (unsigned long) handler >> 16;
	idt_table[idx].offset2 = (unsigned long) handler >> 32;
	idt_table[idx].reserved = 0;
}

struct __attribute__((packed)) tss_struct {
	unsigned int	reserved0;
	unsigned long	rsp[3];
	unsigned int	reserved1;
	unsigned int	reserved2;
	unsigned long	ist[7];
	unsigned int	reserved3;
	unsigned int	reserved4;
	unsigned short	reserved5;
	unsigned short	iopb_offset;
};

static __CPULOCAL __attribute__((aligned(128))) struct tss_struct tss_struct;

void
x86_update_rsp0 (unsigned long value)
{
	this_cpu_write (&tss_struct.rsp[0], value);
}

struct __attribute__((packed)) segment_ptr {
	unsigned short limit;
	unsigned long address;
};

void
x86_setup_local_traps (void)
{
	struct segment_ptr idt_ptr = { 0xfff, (unsigned long) idt_table };
	asm volatile ("ltr %0" :: "r" (__GDT_TSS));
	asm volatile ("lidt %0" :: "m" (idt_ptr));
}

__INIT_TEXT
static void
setup_traps_for (unsigned int cpu)
{
	/* allocate stacks */
	struct tss_struct *tss = that_cpu_ptr (&tss_struct, cpu);
	for (unsigned int i = 0; i < TRAP_STK_NUM; i++) {
		void *stk = initmem_alloc_virt (TRAP_STK_SIZE, TRAP_STK_SIZE);
		if (!stk)
			panic ("Out-of-memory while allocating exception stacks.");

		/**
		 * Store the kernel GSBASE at the start of the stack.
		 * Low-level atomic entry does the equivalent of:
		 *	if(gsbase != *(unsigned long *) (rsp & ~0x3fff))
		 *		swapgs()
		 * to ensure a correct GSBASE.
		 */
		*(unsigned long *) stk = __cpulocal_offsets[cpu];
		tss->ist[i] = (unsigned long) stk + TRAP_STK_SIZE;
	}

	/* disable IOPB */
	tss->iopb_offset = 0x68;

	unsigned long *gdt = that_cpu_ptr (&percpu_GDT[0], cpu);
	unsigned long addr = (unsigned long) tss;

	if (cpu != 0)
		/* Copy GDT entries from the BSP. */
		memcpy (gdt, this_cpu_ptr (&percpu_GDT[0]), 48);

	gdt[(__GDT_TSS >> 3) + 0] =
		((addr & 0xffffffUL) << 16) | ((addr & 0xff000000UL) << 32)
		| 0x890000000067UL;
	gdt[(__GDT_TSS >> 3) + 1] = addr >> 32;
}

extern char asm_handle_BP[];

extern char asm_handle_NMI[];
extern char asm_handle_MCE[];

extern char asm_handle_APIC_timer[];
extern char asm_handle_IRQ_spurious[];

extern void *asm_idtentry_irq_array[];

__INIT_TEXT
void
x86_trap_init (void)
{
	for (unsigned int i = 0; i < nr_cpus; i++)
		setup_traps_for (i);

	set_idt_entry (X86_TRAP_BP, asm_handle_BP, __IST_NONE, 3);

	set_idt_entry (X86_TRAP_NMI, asm_handle_NMI, __IST_NMI, 0);
	set_idt_entry (X86_TRAP_MCE, asm_handle_MCE, __IST_MCE, 0);

	set_idt_entry (VECTOR_APIC_TIMER, asm_handle_APIC_timer, __IST_NONE, 0);
	set_idt_entry (VECTOR_SPURIOUS, asm_handle_IRQ_spurious, __IST_NONE, 0);

	for (int i = 0; i < IRQ_VECTORS_PER_CPU; i++)
		set_idt_entry (IRQ_VECTORS_OFFSET + i,
			asm_idtentry_irq_array[i], __IST_NONE, 0);

	x86_setup_local_traps ();
}
