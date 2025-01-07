/**
 * Pagefault handling.
 * Copyright (C) 2025-present  dbstream
 */
#ifndef _DAVIX_PAGEFAULT_H
#define _DAVIX_PAGEFAULT_H 1

typedef int pagefault_status_t;

struct pagefault_info {
	unsigned long addr;
	int access_write : 1;
	int access_exec : 1;
};

#define FAULT_HANDLED		0
#define FAULT_SIGSEGV		1
#define FAULT_SIGBUS		2

extern pagefault_status_t
vm_handle_pagefault (struct pagefault_info *fault);

#endif /* _DAVIX_PAGEFAULT_H  */
