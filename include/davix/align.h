/**
 * Align pointers and values.
 * Copyright (C) 2024  dbstream
 */
#ifndef _DAVIX_ALIGN_H
#define _DAVIX_ALIGN_H 1

#define ALIGN_UP(n, alignment) ({				\
	__auto_type _n = (n);					\
	__auto_type _alignment = (alignment);			\
	(_n + _alignment - 1) & ~(_alignment - 1);		\
})

#define ALIGN_DOWN(n, alignment) ((n) & ~((alignment) - 1))

#define ALIGN_UP_PTR(p, alignment)				\
	((typeof(p)) ALIGN_UP((unsigned long) (p), (alignment)))

#define ALIGN_DOWN_PTR(p, alignment)				\
	((typeof(p)) ALIGN_DOWN((unsigned long) (p), (alignment)))

#endif /* _DAVIX_ALIGN_H */
