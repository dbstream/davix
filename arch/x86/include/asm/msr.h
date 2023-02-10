/* SPDX-License-Identifier: MIT */
#ifndef __ASM_MSR_H
#define __ASM_MSR_H

/*
 * Please keep this list of MSRs sorted by address.
 */

#define MSR_PAT 0x277

#define MSR_EFER 0xc0000080
#define _EFER_SCE (1 << 0)	/* enable syscall/sysret instructions */
#define _EFER_LME (1 << 8)	/* long mode enable */
#define _EFER_LMA (1 << 10)	/* long mode active (RO) */
#define _EFER_NXE (1 << 11) 	/* no execute enable */

#define MSR_FSBASE 0xc0000100
#define MSR_GSBASE 0xc0000101
#define MSR_KERNELGSBASE 0xc0000102

#ifndef __ASSEMBLER__

#include <davix/types.h>

static inline void __read_msr(u32 msr, u32 *low, u32 *high)
{
	asm volatile("rdmsr" : "=a"(*low), "=d"(*high) : "c"(msr) : "memory");
}

static inline void __write_msr(u32 msr, u32 low, u32 high)
{
	asm volatile("wrmsr" : : "a"(low), "d"(high), "c"(msr) : "memory");
}

static inline u64 read_msr(u32 msr)
{
	u32 low, high;
	__read_msr(msr, &low, &high);
	return (u64) low | ((u64) high << 32);
}

static inline void write_msr(u32 msr, u64 value)
{
	__write_msr(msr, value, value >> 32);
}

#endif /* __ASSEMBLER__ */

#endif /* __ASM_MSR_H */
