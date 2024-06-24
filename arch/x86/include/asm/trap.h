/**
 * Exception and trap entry into the kernel.
 */
#ifndef _ASM_TRAP_H
#define _ASM_TRAP_H 1

/**
 * Use 16KiB exception stack. Low cost because there are only
 * nr_cpus*TRAP_STACK_NUM of these.
 */
#define TRAP_STK_SIZE 0x4000

/* Number of exception stacks. */
#define TRAP_STK_NUM	3
#define TRAP_STK_DF	0	/* #DF */
#define TRAP_STK_NMI	1	/* NMI */
#define TRAP_STK_MCE	2	/* #MCE */

#define __IST_NONE	0
#define __IST_DF	(1 + TRAP_STK_DF)
#define __IST_NMI	(1 + TRAP_STK_NMI)
#define __IST_MCE	(1 + TRAP_STK_MCE)

#define X86_TRAP_DE	0
#define X86_TRAP_DB	1
#define X86_TRAP_NMI	2
#define X86_TRAP_BP	3
#define X86_TRAP_OF	4
#define X86_TRAP_BR	5
#define X86_TRAP_UD	6
#define X86_TRAP_NM	7
#define X86_TRAP_DF	8
#define X86_TRAP_TS	10
#define X86_TRAP_NP	11
#define X86_TRAP_SS	12
#define X86_TRAP_GP	13
#define X86_TRAP_PF	14
#define X86_TRAP_MF	16
#define X86_TRAP_AC	17
#define X86_TRAP_MCE	18
#define X86_TRAP_XM	19

#ifndef __ASSEMBLER__

extern void
x86_update_rsp0 (unsigned long value);

extern void
x86_setup_local_traps (void);

extern void
x86_trap_init (void);

#endif /* __ASSEMBLER__ */
#endif /* _ASM_TRAP_H */
