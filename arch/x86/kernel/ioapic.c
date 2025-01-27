/**
 * Manage IOAPICs in the system.
 * Copyright (C) 2025-present  dbstream
 *
 * We need to be able to associate GSI no. with a struct ioapic.  We also need
 * to track legacy IRQ -> GSI mappings.
 */
#include <davix/errno.h>
#include <davix/interrupt.h>
#include <davix/kmalloc.h>
#include <davix/list.h>
#include <davix/printk.h>
#include <davix/spinlock.h>
#include <davix/vmap.h>
#include <asm/acpi.h>
#include <asm/apic.h>
#include <asm/interrupt.h>

/**
 *   Legacy IRQ mapping
 *
 * This corresponds to the ACPI "interrupt source override" information.  If
 * none of {force_active_hi, force_active_lo} are set, use the device default
 * polarity.  If none of {force_tgm_edge, force_tgm_level} are set, use the
 * device default trigger mode.
 */

struct legacy_irq_mapping {
	uint32_t gsi;
	bool force_active_hi;
	bool force_active_lo;
	bool force_tgm_edge;
	bool force_tgm_level;
} legacy_irq_mapping[16] = {
	{ 0,	0, 0, 0, 0 },
	{ 1,	0, 0, 0, 0 },
	{ 2,	0, 0, 0, 0 },
	{ 3,	0, 0, 0, 0 },
	{ 4,	0, 0, 0, 0 },
	{ 5,	0, 0, 0, 0 },
	{ 6,	0, 0, 0, 0 },
	{ 7,	0, 0, 0, 0 },
	{ 8,	0, 0, 0, 0 },
	{ 9,	0, 0, 0, 0 },
	{ 10,	0, 0, 0, 0 },
	{ 11,	0, 0, 0, 0 },
	{ 12,	0, 0, 0, 0 },
	{ 13,	0, 0, 0, 0 },
	{ 14,	0, 0, 0, 0 },
	{ 15,	0, 0, 0, 0 }
};

static void
configure_legacy_irq (uint32_t map_source, uint32_t map_dest,
		bool force_active_hi, bool force_active_lo,
		bool force_tgm_edge, bool force_tgm_level)
{
	if (map_source >= 16)
		return;

	legacy_irq_mapping[map_source] = (struct legacy_irq_mapping) {
		map_dest,
		force_active_hi, force_active_lo,
		force_tgm_edge, force_tgm_level
	};
}

struct ioapic {
	unsigned long phys_addr;
	void *mapping;
	uint32_t ioapic_id;
	uint32_t gsi_base;
	uint32_t num_gsis;
	struct list ioapic_list;
};

static DEFINE_EMPTY_LIST (ioapic_list);
static spinlock_t ioapic_lock;

static struct ioapic *
find_ioapic (uint32_t gsi)
{
	list_for_each (entry, &ioapic_list) {
		struct ioapic *apic = struct_parent (struct ioapic, ioapic_list, entry);
		if (apic->gsi_base <= gsi && gsi < apic->gsi_base + apic->num_gsis)
			return apic;
	}

	return NULL;
}

static inline uint32_t
ioapic_read (struct ioapic *ioapic, uint32_t offset)
{
	*(volatile uint32_t *) (ioapic->mapping + 0x00) = offset;
	return *(volatile uint32_t *) (ioapic->mapping + 0x10);
}

static inline void
ioapic_write (struct ioapic *ioapic, uint32_t offset, uint32_t value)
{
	*(volatile uint32_t *) (ioapic->mapping + 0x00) = offset;
	*(volatile uint32_t *) (ioapic->mapping + 0x10) = value;
}

#define IOAPICID		0x00
#define IOAPICVER		0x01
#define IOAPICARB		0x02
#define IOAPICREDTBL(x)		(0x10 + 2 * x)

#define IRQCOMPAT_FLAGS (APIC_DM_BITS | APIC_MODE_BITS)

/**
 * Configure an IOAPIC pin and install an interrupt handler.
 *
 * @gsi		Global System Interrupt no.
 * @value	APIC_* flags.
 * @flags	Flags for pin allocation.
 * @callback	A callback that is called with the selected vector number.
 * @arg		Auxiliary argument to the callback.
 */
static errno_t
ioapic_configure_interrupt_pin (uint32_t gsi, uint32_t value, irqhandler_flags_t flags,
		errno_t (*callback) (unsigned int vector, void *), void *arg)
{
	errno_t e = ENXIO;

	spin_lock (&ioapic_lock);
	struct ioapic *ioapic = find_ioapic (gsi);
	if (ioapic) {
		e = ESUCCESS;
		gsi -= ioapic->gsi_base;
		gsi = IOAPICREDTBL (gsi);

		uint32_t old = ioapic_read (ioapic, gsi);
		if (old & APIC_IRQ_MASK) {
			unsigned int v = alloc_irq_vector ();
			if (v != -1U) {
				e = callback (v, arg);
				if (e == ESUCCESS) {
					ioapic_write (ioapic, gsi + 1, 0);
					ioapic_write (ioapic, gsi, value | (v + IRQ_VECTORS_OFFSET));
				} else
					free_irq_vector (v);
			} else
				e = ENOSPC;
		} else {
			if (flags & (INTERRUPT_EXCLUSIVE))
				e = EEXIST;
			else if ((old & IRQCOMPAT_FLAGS) != (value & IRQCOMPAT_FLAGS))
				e = EEXIST;
			else
				e = callback ((old & 0xffU) - IRQ_VECTORS_OFFSET, arg);
		}
	}
	spin_unlock (&ioapic_lock);
	return e;
}

/**
 * Clear an IOAPIC pin and free the associated vector number.
 */
static void
ioapic_clear_interrupt_pin_and_free_vector (uint32_t gsi, unsigned int vector)
{
	spin_lock (&ioapic_lock);
	struct ioapic *ioapic = find_ioapic (gsi);
	if (ioapic) {
		gsi -= ioapic->gsi_base;
		gsi = IOAPICREDTBL (gsi);

		ioapic_write (ioapic, gsi, APIC_IRQ_MASK);
	} else
		printk (PR_WARN "ioapic_clear_interrupt_pin_and_free_vector called for non-present GSI %u\n", gsi);

	free_irq_vector (vector);
	spin_unlock (&ioapic_lock);
}

errno_t
arch_configure_interrupt_pin (unsigned int *irq_number, irqhandler_flags_t flags,
		errno_t (*callback) (unsigned int vector, void *), void *arg)
{
	uint32_t value = 0;
	if (flags & INTERRUPT_ACTIVE_LOW)
		value |= APIC_POLARITY_LOW;
	if (flags & INTERRUPT_LEVEL_TRIGGERED)
		value |= APIC_TRIGGER_MODE_LEVEL;
	if ((flags & INTERRUPT_LEGACY) && *irq_number < 16) {
		if (legacy_irq_mapping[*irq_number].force_active_hi)
			value &= ~APIC_POLARITY_LOW;
		else if (legacy_irq_mapping[*irq_number].force_active_lo)
			value |= APIC_POLARITY_LOW;
		if (legacy_irq_mapping[*irq_number].force_tgm_edge)
			value &= ~APIC_TRIGGER_MODE_LEVEL;
		else if (legacy_irq_mapping[*irq_number].force_tgm_level)
			value |= APIC_TRIGGER_MODE_LEVEL;
		*irq_number = legacy_irq_mapping[*irq_number].gsi;
	}

	return ioapic_configure_interrupt_pin (*irq_number, value, flags,
			callback, arg);
}

void
arch_clear_and_free_interrupt_pin (unsigned int irq_number, unsigned int vector)
{
	ioapic_clear_interrupt_pin_and_free_vector (irq_number, vector);
}

static void
configure_ioapic (uint32_t ioapic_id, unsigned long addr, uint32_t base)
{
	struct ioapic *ioapic = kmalloc (sizeof (*ioapic));
	if (!ioapic) {
		printk (PR_ERR "configure_ioapic: failed to allocate a struct ioapic\n");
		return;
	}

	ioapic->phys_addr = addr;
	ioapic->ioapic_id = ioapic_id;
	ioapic->gsi_base = base;
	ioapic->mapping = vmap_phys ("IOAPIC", addr, 0x20,
			PAGE_KERNEL_DATA | PG_UC);
	if (!ioapic->mapping) {
		printk (PR_ERR "configure_ioapic: failed to map IOAPIC memory\n");
		kfree (ioapic);
		return;
	}

	ioapic->num_gsis = ((ioapic_read (ioapic, IOAPICVER) >> 16) & 0xffU) + 1U;

	printk (PR_INFO "configure_ioapic:  IOAPIC 0x%x  (addr=0x%lx)  GSIs %u-%u\n",
			ioapic_id, addr, base, base + ioapic->num_gsis - 1);

	for (uint32_t i = 0; i < ioapic->num_gsis; i++)
		ioapic_write (ioapic, IOAPICREDTBL (i), APIC_IRQ_MASK);

	spin_lock (&ioapic_lock);
	list_insert_back (&ioapic_list, &ioapic->ioapic_list);
	spin_unlock (&ioapic_lock);
}

void
ioapic_init (void)
{
	x86_madt_parse_interrupt_overrides (configure_legacy_irq);

	printk (PR_INFO "ioapic:  Legacy IRQ mapping:\n");
	printk (PR_INFO "... irq  -> gsi  active_hi  active_lo  tgm_edge  tgm_level\n");
	for (int i = 0; i < 16; i++) {
		printk (PR_INFO "... %3d  -> %3u  %9s  %9s  %8s  %9s\n",
			i, legacy_irq_mapping[i].gsi,
			legacy_irq_mapping[i].force_active_hi ? "yes" : "no",
			legacy_irq_mapping[i].force_active_lo ? "yes" : "no",
			legacy_irq_mapping[i].force_tgm_edge ? "yes" : "no",
			legacy_irq_mapping[i].force_tgm_level ? "yes" : "no");
	}

	x86_madt_parse_ioapics (configure_ioapic);
}
