/**
 * Bits in the rflags register.
 * Copyright (C) 2025-present  dbstream
 */
#ifndef _ASM_RFLAGS_H
#define _ASM_RGLAGS_H 1

#define __RFL_CF		(1 << 0)
#define __RFL_RA1		(1 << 1)	/* reserved, "always 1" */
#define __RFL_PF		(1 << 2)
#define __RFL_AF		(1 << 4)
#define __RFL_ZF		(1 << 6)
#define __RFL_SF		(1 << 7)
#define __RFL_TF		(1 << 8)
#define __RFL_IF		(1 << 9)
#define __RFL_DF		(1 << 10)
#define __RFL_OF		(1 << 11)
#define __RFL_IOPL(lvl)		((lvl) << 12)
#define __RFL_IOPL0		__RFL_IOPL(0)
#define __RFL_IOPL3		__RFL_IOPL(3)
#define __RFL_NT		(1 << 14)
#define __RFL_RF		(1 << 16)
#define __RFL_VM		(1 << 17)
#define __RFL_AC		(1 << 18)
#define __RFL_VIF		(1 << 19)
#define __RFL_VIP		(1 << 20)
#define __RFL_ID		(1 << 21)

#define __RFL_INITIAL		(__RFL_RA1 | __RFL_IF)
#define __RFL_CLEAR_ON_SYSCALL	(__RFL_CF | __RFL_PF | __RFL_AF | __RFL_ZF	\
		| __RFL_SF | __RFL_TF | __RFL_IF | __RFL_DF | __RFL_OF		\
		| __RFL_IOPL3 | __RFL_NT | __RFL_RF | __RFL_AC | __RFL_ID)

#endif /* _ASM_RFLAGS_H */
