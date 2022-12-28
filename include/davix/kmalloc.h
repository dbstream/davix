/* SPDX-License-Identifier: MIT */
#ifndef __DAVIX_KMALLOC_H
#define __DAVIX_KMALLOC_H

void init_kmalloc(void);
void dump_kmalloc_slabs(void);

void *kmalloc(unsigned long size);
void kfree(void *mem);

#endif /* __DAVIX_KMALLOC_H */
