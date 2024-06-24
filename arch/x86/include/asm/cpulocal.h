/**
 * CPU-local variables.
 * Copyright (C) 2024  dbstream
 */
#ifndef _ASM_CPULOCAL_H
#define _ASM_CPULOCAL_H 1

#include <asm/sections.h>	/* __CPULOCAL */

/* NOTE: These are CPU-local variables. */
extern unsigned long __cpulocal_offset;
extern unsigned long __stack_canary;
extern unsigned int __this_cpu_id;

extern unsigned long __cpulocal_offsets[];

static inline void *
__add_cpulocal_offset (void *p)
{
	asm ("addq %%gs:0, %0" : "+r" (p));
	return p;
}

static inline void *
__add_cpulocal_offset_on (void *p, unsigned int cpu)
{
	return (void *) ((unsigned long) p + __cpulocal_offsets[cpu]);
}

#define this_cpu_ptr(p)		((typeof(p)) __add_cpulocal_offset(p))
#define this_cpu_deref(p)	(*(typeof (*p) __seg_gs *) (unsigned long) (p))

#define this_cpu_read(p)		({		\
	typeof(*(p)) _p = this_cpu_deref (p);		\
	_p;						\
})

#define this_cpu_write(p, value)	({		\
	this_cpu_deref (p) = (value);			\
})

#define that_cpu_ptr(p, cpu)	((typeof(p)) __add_cpulocal_offset_on((p), (cpu)))
#define that_cpu_deref(p, cpu)	(*that_cpu_ptr((p), (cpu)))

#define that_cpu_read(p, cpu)		({		\
	typeof(*(p)) _p = that_cpu_deref ((p), (cpu));	\
	_p;						\
})

#define that_cpu_write(p, cpu, value)	({		\
	that_cpu_deref ((p), (cpu)) = (value);		\
})

#endif /* _ASM_CPULOCAL_H */
