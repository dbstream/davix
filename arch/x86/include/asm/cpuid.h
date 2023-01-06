/* SPDX-License-Identifier: MIT */
#ifndef __ASM_CPUID_H
#define __ASM_CPUID_H

#define do_cpuid(leaf, a, b, c, d) \
	asm volatile("cpuid" \
		: "=a"(a), "=b"(b), "=c"(c), "=d"(d) \
		: "a"(leaf))

#define do_cpuid_subleaf(leaf, subleaf, a, b, c, d) \
	asm volatile("cpuid" \
		: "=a"(a), "=b"(b), "=c"(c), "=d"(d) \
		: "a"(leaf), "c"(subleaf))


#define __ID_00000007_0_ECX_LA57 (1 << 16)	/* 5-level paging */
#define __ID_00000001_EDX_PAT (1 << 16) /* Page Attribute Table */
#define __ID_80000001_EDX_NX (1 << 20)	/* No-Execute support */
#define __ID_80000001_EDX_PDPE1G (1 << 26)	/* 1GiB paging */

#endif /* __ASM_CPUID_H */
