/* SPDX-License-Identifier: MIT */
#include <davix/acpi.h>
#include <davix/panic.h>
#include <davix/printk.h>
#include <davix/smp.h>
#include <davix/resource.h>
#include <davix/time.h>
#include <davix/list.h>
#include <asm/apic.h>
#include <asm/page.h>
#include <asm/cpuid.h>
#include <asm/vector.h>

struct apic *active_apic_interface;

unsigned long apic_base = 0xfee00000;

struct apicnmi {
	struct list list;
	unsigned cpu;
	u8 lint;
	bool pol_low;
	bool tgm_level;
};

static struct list apic_nmis = LIST_INIT(apic_nmis);

static void apic_parse_madt(struct acpi_subtable_header *entry, void *arg)
{
	(void) arg;

	u8 nmi_lint;
	u16 nmi_flags;
	unsigned nmi_cpu;

	switch(entry->type) {
	case ACPI_MADT_TYPE_LOCAL_APIC_OVERRIDE:;
		struct acpi_madt_local_apic_override *override =
			(struct acpi_madt_local_apic_override *) entry;

		apic_base = override->address;
		return;
	case ACPI_MADT_TYPE_LOCAL_APIC_NMI:;
		struct acpi_madt_local_apic_nmi *nmi =
			(struct acpi_madt_local_apic_nmi *) entry;

		nmi_lint = nmi->lint;
		nmi_flags = nmi->inti_flags;
		nmi_cpu = nmi->processor_id == 0xff
			? 0xffffffff : nmi->processor_id;
		goto nmi_common;
	case ACPI_MADT_TYPE_LOCAL_X2APIC_NMI:;
		struct acpi_madt_local_x2apic_nmi *x2nmi =
			(struct acpi_madt_local_x2apic_nmi *) entry;

		nmi_lint = x2nmi->lint;
		nmi_flags = x2nmi->inti_flags;
		nmi_cpu = x2nmi->uid;
	default:
		return;
	}
nmi_common:
	nmi_cpu = nmi_cpu == 0xffffffff
		? nmi_cpu
		: acpi_cpu_to_lapic_id[nmi_cpu];

	struct apicnmi *nmi = kmalloc(sizeof(struct apicnmi));
	if(!nmi)
		panic("apic_init(): Couldn't allocate struct to hold information about LINT pins.");

	nmi->cpu = nmi_cpu;
	nmi->lint = nmi_lint;
	nmi->pol_low = (nmi_flags & 2) ? 1 : 0;
	nmi->tgm_level = (nmi_flags & 8) ? 1 : 0;
	list_add(&apic_nmis, &nmi->list);
}

static unsigned long calibrate_timer(void)
{
	apic_write(APIC_TIMER, APIC_TIMER_MASKED);
	apic_write(APIC_TIMER_DIV, 3);
	apic_write(APIC_TIMER_INITIAL_COUNT, -1);

	usecs_t wait_for = 50000; /* We might want to tweak this. */
	udelay(wait_for);
	return (1000000 / wait_for) * (-1 - apic_read(APIC_TIMER_COUNT));
}

void apic_configure(void)
{
	unsigned me = smp_self()->id;

	apic_write(APIC_LINT0, APIC_IRQ_MASKED);
	apic_write(APIC_LINT1, APIC_IRQ_MASKED);

	struct apicnmi *nmi;
	list_for_each(nmi, &apic_nmis, list) {
		if(nmi->cpu != 0xffffffff && nmi->cpu != me)
			continue;

		debug("apic: route CPU%u.LINT%hhu to NMI (%s %s)\n",
			me, nmi->lint,
			nmi->pol_low ? "active-low" : "active-high",
			nmi->tgm_level ? "level" : "edge");

		apic_write(nmi->lint ? APIC_LINT1 : APIC_LINT0, APIC_DM_NMI
			| (nmi->pol_low ? APIC_POLARITY_LOW : 0)
			| (nmi->tgm_level ? APIC_TGM_LEVEL : 0));
	}

	apic_write(APIC_SPR_VECTOR, (1 << 8) | SPURIOUS_VECTOR);

	unsigned long apic_hz = calibrate_timer();
	debug("apic: CPU%u timer: %lu.%01ukhz\n",
		me, apic_hz / 1000, (apic_hz % 1000) / 100);

	apic_write(APIC_TIMER, APIC_TIMER_PERIODIC | TIMER_VECTOR);
	apic_write(APIC_TIMER_DIV, 3);
	apic_write(APIC_TIMER_INITIAL_COUNT, apic_hz / TIMER_HZ);
	apic_send_eoi(); /* why not */
}

void apic_init(struct acpi_table_madt *madt)
{
	unsigned long a, b, c, d;
	do_cpuid(1, a, b, c, d);
	if(c & __ID_00000001_ECX_X2APIC) {
		info("apic_init(): x2apic supported.\n");
		active_apic_interface = &x2apic_impl;
	} else {
		info("apic_init(): x2apic not supported.\n");
		active_apic_interface = &xapic_impl;
	}

	if(madt->address)
		apic_base = madt->address;

	acpi_parse_table(madt, apic_parse_madt, NULL);

	struct resource *apic_resource = alloc_resource_at(&system_memory,
		apic_base, PAGE_SIZE, "memory-mapped APIC registers");

	if(!apic_resource)
		panic("apic_init(): couldn't allocate struct resource for APIC registers (memory already in use?)");

	active_apic_interface->init();
}
