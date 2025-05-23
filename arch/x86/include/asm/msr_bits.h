/**
 * Bits in MSRs (model-specific registers).
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#define MSR_APIC_BASE	0x1b
#define _APIC_BASE_ENABLED	(1 << 11)
#define _APIC_BASE_X2APIC	(1 << 10)

#define MSR_PAT		0x277

#define MSR_EFER	0xc0000080
#define _EFER_SCE	(1 << 0)	/* enable syscall/sysret insns */
#define _EFER_LME	(1 << 8)	/* enable long mode */
#define _EFER_LMA	(1 << 10)	/* long mode active */
#define _EFER_NXE	(1 << 11)	/* no execute enable */

#define MSR_STAR		0xc0000081
#define MSR_LSTAR		0xc0000082
#define MSR_CSTAR		0xc0000083
#define MSR_SFMASK		0xc0000084

#define MSR_FSBASE		0xc0000100
#define MSR_GSBASE		0xc0000101
#define MSR_KERNELGSBASE	0xc0000102
