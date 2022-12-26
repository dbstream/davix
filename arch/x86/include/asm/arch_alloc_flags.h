/* SPDX-License-Identifier: MIT */
#ifndef __ASM_ARCH_ALLOC_FLAGS_H
#define __ASM_ARCH_ALLOC_FLAGS_H

#ifndef __DAVIX_TYPES_H
#warning "Please don't ``#include <asm/arch_alloc_flags.h>`` directly."
#endif /* __DAVIX_TYPES_H */

#define __ALLOC_ARCH_BIT(idx) \
	((force alloc_flags_t) (1UL << (idx + __ALLOC_ARCH_SHIFT)))

#define __ALLOC_LOW4G __ALLOC_ARCH_BIT(0)
#define __ALLOC_LOW16M __ALLOC_ARCH_BIT(1)

#endif /* __ASM_ARCH_ALLOC_FLAGS_H */
