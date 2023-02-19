/* SPDX-License-Identifier: MIT */
#ifndef __ASM_SEGMENT_H
#define __ASM_SEGMENT_H

/*
 * These are bits in the control registers cr0..cr3 and xcr0. Always refer to
 * the AMD64/Ia32e manuals for accurate information.
 */

#define __CR0_PE	(1 << 0)	/* Protection Enable */
#define __CR0_MP	(1 << 1)	/* Monitor Coprocessor */
#define __CR0_EM	(1 << 2)	/* Emulation */
#define __CR0_TS	(1 << 3)	/* Task Switched */
#define __CR0_ET	(1 << 4)	/* Extension Type */
#define __CR0_NE	(1 << 5)	/* Numeric Error */
#define __CR0_WP	(1 << 16)	/* Write Protect */
#define __CR0_AM	(1 << 18)	/* Alignment Mask */
#define __CR0_NW	(1 << 29)	/* Not Write-through */
#define __CR0_CD	(1 << 30)	/* Cache Disable */
#define __CR0_PG	(1 << 31)	/* Paging */

#define __CR3_PWT	(1 << 3)	/* Page-level Write-Through */
#define __CR3_PCD	(1 << 4)	/* Page-level Cache Disable */

#define __CR4_VME	(1 << 0)	/* Virtual-8086 Mode Extensions */
#define __CR4_PVI	(1 << 1)	/* Protected-Mode Virtual Interrupts */
#define __CR4_TSD	(1 << 2)	/* Time Stamp Disable */
#define __CR4_DE	(1 << 3)	/* Debugging Extensions */
#define __CR4_PSE	(1 << 4)	/* Page Size Extensions */
#define __CR4_PAE	(1 << 5)	/* Physical Address Extensions */
#define __CR4_MCE	(1 << 6)	/* Machine-Check Enable */
#define __CR4_PGE	(1 << 7)	/* Page Global Enable */
#define __CR4_PCE	(1 << 8)	/* Performance-Monitoring Counter \
						Enable */
#define __CR4_OSFXSR	(1 << 9)	/* Operating System Support for FXSAVE \
						and FXRSTOR instructions */
#define __CR4_OSXMMEXCPT	(1 << 10)	/* Operating System Support for \
							Unmasked SIMD \
							Floating-Point \
							Exceptions */
#define __CR4_UMIP	(1 << 11)	/* User-Mode Instruction Prevention */
#define __CR4_LA57	(1 << 12)	/* 57-bit linear addresses */
#define __CR4_VMXE	(1 << 13)	/* VMX-Enable Bit */
#define __CR4_SMXE	(1 << 14)	/* SMX-Enable Bit */
#define __CR4_FSGSBASE	(1 << 16)	/* FSGSBASE-Enable Bit */
#define __CR4_PCIDE	(1 << 17)	/* PCID-Enable Bit */
#define __CR4_OSXSAVE	(1 << 18)	/* XSAVE and Processor Extended \
						States-Enable Bit */
#define __CR4_KL	(1 << 19)	/* Key-Locker-Enable Bit */
#define __CR4_SMEP	(1 << 20)	/* Supervisor-mode Execution \
						Prevention Bit */
#define __CR4_SMAP	(1 << 21)	/* Supervisor-mode Access Prevention \
						Bit */
#define __CR4_PKE	(1 << 22)	/* Enable protection keys for \
						user-mode pages */
#define __CR4_CET	(1 << 23)	/* Control-Flow Enforcement \
						Technology */
#define __CR4_PKS	(1 << 24)	/* Enable protection keys for \
						supervisor-mode pages*/

#define __XCR0_X87	(1 << 0)	/* 80387 FP support (must be 1) */
#define __XCR0_SSE	(1 << 1)	/* XSAVE support */
#define __XCR0_AVX	(1 << 2)	/* AVX instruction support */
#define __XCR0_BNDREG	(1 << 3)	/* MPX instruction support, XSAVE \
						management of BND0..3 */
#define __XCR0_BNDCSR	(1 << 4)	/* MPX instruction support, XSAVE \
						management of BNDCFGU and \
						BNDSTATUS */
#define __XCR0_opmask	(1 << 5)	/* AVX-512 instruction support, XSAVE \
						management of k0..7 */
#define __XCR0_ZMM_Hi256	(1 << 6)	/* AVX-512 instruction \
							support, XSAVE \
							management of upper \
							halves of ZMM0..ZMM15 */
#define __XRC0_Hi16_ZMM	(1 << 7)	/* AVX-512 instruction support, XSAVE \
						management of ZMM16..32 */
#define __XCR0_PKRU	(1 << 9)	/* XSAVE management of PKRU register */

#ifndef __ASSEMBLER__

#include <davix/types.h>

struct packed idt_entry {
	u16 offset1;
	u16 cs;
	u8 ist_entry;
	u8 attribs;
	u16 offset2;
	u32 offset3;
	u32 reserved;
};

struct packed segment_ptr {
	u16 limit;
	u64 base;
};

#define BUILD_RW(reg) \
	static inline unsigned long read_##reg(void) { \
		unsigned long r; \
		asm volatile("movq %%" #reg ", %%rax" : "=a"(r) : : ); \
		return r; \
	} \
	static inline void write_##reg(unsigned long value) { \
		asm volatile("movq %%rax, %%" #reg : : "a"(value) : ); \
	}

BUILD_RW(cr0)
BUILD_RW(cr2)
BUILD_RW(cr3)
BUILD_RW(cr4)
BUILD_RW(cr8)

#undef BUILD_RW

static inline void load_idt(unsigned long base, unsigned short limit)
{
	struct segment_ptr segptr = {
		.limit = limit,
		.base = base
	};

	asm volatile("lidt %0" : : "m"(segptr) );
}

/*
 * TSS structure.
 *
 * Usage is as follows:
 *
 *   rsp0:    syscall & fault stack
 *
 *   rsp1..3: unused
 *
 *   ist1:    timer interrupt
 *
 *   ist2:    IRQ stack
 *
 *   ist3:    Double-fault stack
 *
 *   ist4:    NMI & MCE stack
 *
 *   ist5..7: unused
 */
struct packed tss {
	u32 reserved0;
	u64 rsp0, rsp1, rsp2;
	u64 reserved1;
	u64 ist1, ist2, ist3, ist4, ist5, ist6, ist7;
	u64 reserved2;
	u16 reserved3;
	u16 iopb_offset;
};

#endif /* !__ASSEMBLER__ */

#define __GDT_NULL	0
#define __GDT_KERNEL_CS	1
#define __GDT_KERNEL_DS	2
#define __GDT_USER32_CS	3
#define __GDT_USER_DS	4
#define __GDT_USER_CS	5
#define __GDT_TSS	6	/* warning: this is two QWORDs wide */
#define __GDT_SIZE 8

#define GDT_NULL (8 * __GDT_NULL)
#define GDT_KERNEL_CS (8 * __GDT_KERNEL_CS)
#define GDT_KERNEL_DS (8 * __GDT_KERNEL_DS)
#define GDT_USER32_CS (8 * __GDT_USER32_CS)
#define GDT_USER_DS (8 * __GDT_USER_DS)
#define GDT_USER_CS (8 * __GDT_USER_CS)
#define GDT_TSS (8 * __GDT_TSS)

#define GDT_SIZE (8 * __GDT_SIZE)

#endif /* __ASM_SEGMENT_H */
