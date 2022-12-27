#ifndef __ASM_REGS_H
#define __ASM_REGS_H

#define __RFLAGS_TF (1 << 8)	/* Trap */
#define __RFLAGS_IF (1 << 9)	/* Interrupt mask */

#define __RFLAGS_IOPL_SHIFT 12	/* IO privilege level */

#define __RFLAGS_NT (1 << 14)	/* Nested Task*/
#define __RFLAGS_RF (1 << 16)	/* Resume */
#define __RFLAGS_VM (1 << 17)	/* Enable VM86 */
#define __RFLAGS_AC (1 << 18)	/* Alignment check, access control */
#define __RFLAGS_VIF (1 << 19)	/* Virtual interrupt mask */
#define __RFLAGS_VIP (1 << 20)	/* Virtual interrupt pending */
#define __RFLAGS_ID (1 << 21)	/* CPUID bit */

#ifndef __ASSEMBLER__

static inline unsigned long x86_read_rflags(void)
{
	unsigned long rflags;
	asm volatile("pushfq; popq %0" : "=r" (rflags) : : "memory");
	return rflags;
}

#endif /* __ASSEMBLER__ */

#endif /* __ASM_REGS_H */
