/**
 * Local APIC interfaces.
 * Copyright (C) 2024  dbstream
 */
#ifndef _ASM_APIC_H
#define _ASM_APIC_H 1

#include <davix/stdint.h>

/* Don't change! It has to match IDX_APIC_BASE in early_memmap.h */
#define MAPPED_APIC_BASE	_UL(0xffffffffffe00000)

#define APIC_ID			0x20
#define APIC_VERSION		0x30
#define APIC_TPR		0x80
#define APIC_APR		0x90
#define APIC_PPR		0xa0
#define APIC_EOI		0xb0
#define APIC_RRD		0xc0
#define APIC_LDR		0xd0
#define APIC_DFR		0xe0
#define APIC_SPI		0xf0
#define APIC_ESR		0x280
#define APIC_LVTCMCI		0x2f0
#define APIC_ICR_LOW		0x300
#define APIC_ICR_HIGH		0x310
#define APIC_LVTTMR		0x320
#define APIC_LVTTHRM		0x330
#define APIC_LVTPMC		0x340
#define APIC_LVTLINT0		0x350
#define APIC_LVTLINT1		0x360
#define APIC_LVTERR		0x370
#define APIC_TMR_ICR		0x380
#define APIC_TMR_CCR		0x390
#define APIC_TMR_DIV		0x3e0

#define APIC_DM_FIXED		(0U << 8)
#define APIC_DM_LOWPRI		(1U << 8)
#define APIC_DM_SMI		(2U << 8)
#define APIC_DM_NMI		(4U << 8)
#define APIC_DM_INIT		(5U << 8)
#define APIC_DM_SIPI		(6U << 8)

#define APIC_DST_LOGICAL	(1U << 11)

#define APIC_POLARITY_LOW	(1U << 13)
#define APIC_TRIGGER_MODE_LEVEL	(1U << 15)
#define APIC_IRQ_MASK		(1U << 16)

#define APIC_IRQ_PENDING	(1U << 12)

#define APIC_TMR_PERIODIC	(1U << 17)

#define APIC_DST_SELF		(1U << 18)
#define APIC_DST_ALL		(2U << 18)
#define APIC_DST_OTHERS		(3U << 18)

#define APIC_SPI_ENABLE		(1U << 8)
#define APIC_SPI_FCC_ENABLE	(1U << 9)

#ifndef __ASSEMBLER__

extern int apic_is_x2apic;

extern uint32_t cpu_to_apicid[];
extern uint32_t cpu_to_acpi_uid[];

extern uint32_t
apic_read (uint32_t offset);

extern void
apic_write (uint32_t offset, uint32_t value);

extern uint32_t
apic_read_id (void);

extern void
apic_write_icr (uint32_t value, uint32_t target);

extern void
apic_wait_icr (void);

extern void
apic_send_IPI (uint32_t value, uint32_t target);

/* Register that the xAPIC should be located at 'addr'. */
extern void
set_xAPIC_base (unsigned long addr);

extern void
early_apic_init (void);

extern void
apic_init (void);

/* Send APIC End-Of-Interrupt */
static inline void
apic_eoi (void)
{
	apic_write (APIC_EOI, 0);
}

#endif /* !__ASSEMBLER__ */
#endif /* _ASM_APIC_H */
