/* SPDX-License-Identifier: MIT */
#include <davix/acpi.h>
#include <davix/kmalloc.h>
#include <davix/list.h>
#include <davix/panic.h>
#include <davix/printk.h>
#include <davix/spinlock.h>
#include <asm/ioapic.h>
#include <asm/apic.h>

struct ioapic {
	struct list list;
	void *mapped_at;
	unsigned int gsi_base;
	unsigned int num_gsis;
};

static struct list ioapic_list = LIST_INIT(ioapic_list);
static spinlock_t irq_lock = SPINLOCK_INIT(irq_lock);

static u32 ioapic_read(struct ioapic *ioapic, u32 reg)
{
	*(volatile u32 *) ((unsigned long) ioapic->mapped_at) = reg;
	return *(volatile u32 *) ((unsigned long) ioapic->mapped_at + 0x10);
}

static void ioapic_write(struct ioapic *ioapic, u32 reg, u32 value)
{
	*(volatile u32 *) ((unsigned long) ioapic->mapped_at) = reg;
	*(volatile u32 *) ((unsigned long) ioapic->mapped_at + 0x10) = value;
}

static void ioapic_set_entry(struct ioapic *ioapic,
	unsigned int no, unsigned int value)
{
	ioapic_write(ioapic, 0x10 + (2 * no), APIC_IRQ_MASKED);
	ioapic_write(ioapic, 0x11 + (2 * no), 0);
	ioapic_write(ioapic, 0x10 + (2 * no), value);
}

void register_ioapic(unsigned long addr, unsigned int gsi_base)
{
	struct ioapic *ioapic = kmalloc(sizeof(struct ioapic));
	if(!ioapic)
		panic("x86/irq: Couldn't kmalloc() IOAPIC structure.");

	ioapic->mapped_at = acpi_os_map_memory(addr, 0x20);
	if(!ioapic->mapped_at)
		panic("x86/irq: Couldn't map IOAPIC memory.");

	ioapic->gsi_base = gsi_base;
	ioapic->num_gsis = (ioapic_read(ioapic, 1) >> 16) + 1;
	debug("x86/irq: discovered IOAPIC at %p (irqs %u-%u)\n",
		addr, gsi_base, gsi_base + ioapic->num_gsis - 1);

	for(unsigned int no = 0; no < ioapic->num_gsis; no++)
		ioapic_set_entry(ioapic, no, APIC_IRQ_MASKED);

	int flag = spin_acquire_irq(&irq_lock);
	list_add(&ioapic_list, &ioapic->list);
	spin_release_irq(&irq_lock, flag);
}
