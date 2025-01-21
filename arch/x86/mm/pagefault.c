/**
 * Pagefault handler.
 * Copyright (C) 2025-present  dbstream
 *
 * We also implement the Stack-Segment (SS) Fault and General Protection (GP)
 * handlers here.
 */
#include <davix/pagefault.h>
#include <davix/panic.h>
#include <davix/printk.h>
#include <asm/cregs.h>
#include <asm/entry.h>
#include <asm/irq.h>
#include <asm/usercopy.h>

#define PF_P	0x00000001U
#define PF_WR	0x00000002U
#define PF_US	0x00000004U
#define PF_RSVD	0x00000008U
#define PF_ID	0x00000010U
#define PF_PK	0x00000020U
#define PF_SS	0x00000040U
#define PF_HLAT	0x00000080U
#define PF_SGX	0x00001000U

void
handle_SS_exception (struct entry_regs *regs)
{
	panic ("x86: Stack-Segment Fault in userspace!  (rip=0x%lx  ss:rsp=%lu:0x%lx)",
		regs->rip, regs->ss, regs->rsp);
}

void
handle_SS_exception_k (struct entry_regs *regs)
{
	panic ("x86: Stack-Segment Fault in kernelspace!  (rip=0x%lx  ss:rsp=%lu:0x%lx)",
		regs->rip, regs->ss, regs->rsp);
}

void
handle_GP_exception (struct entry_regs *regs)
{
	panic ("x86: General Protection in userspace!  (rip=0x%lx  ss:rsp=%lu:0x%lx)",
		regs->rip, regs->ss, regs->rsp);
}

void
handle_GP_exception_k (struct entry_regs *regs)
{
	panic ("x86: General Protection in kernelspace!  (rip=0x%lx  ss:rsp=%lu:0x%lx)",
		regs->rip, regs->ss, regs->rsp);
}

static void
note_pagefault (unsigned long addr, unsigned int error_code)
{
	const char *by = (error_code & PF_US) ? "usermode" : "kernelmode";
	const char *id = (error_code & PF_SS) ? "shadow stack" :
		(error_code & PF_ID) ? "instruction" : "data";
	const char *tp = (error_code & PF_WR) ? "write" : "read";
	const char *cause = (error_code & PF_RSVD) ? "reserved bit set in PTE" :
		!(error_code & PF_P) ? "nonpresent PTE" :
		(error_code & PF_PK) ? "protection-key rights disallow access" :
		(error_code & PF_WR) ? "readonly PTE" : "unknown";

	printk (PR_NOTICE "x86: Page fault on address 0x%lx.  Cause: %s %s %s with %s.\n",
			addr, by, id, tp, cause);
}

static pagefault_status_t
handle_pagefault_in_vm (unsigned long addr, unsigned int error_code)
{
	struct pagefault_info fault;
	fault.addr = addr;
	fault.access_write = (error_code & PF_WR) ? 1 : 0;
	fault.access_exec = (error_code & PF_ID) ? 1 : 0;

	irq_enable ();
	pagefault_status_t status = vm_handle_pagefault (&fault);
	irq_disable ();
	return status;
}

void
handle_PF_exception (struct entry_regs *regs)
{
	unsigned long addr = read_cr2 ();
	note_pagefault (addr, regs->error_code);
	panic ("x86: Page Fault in userspace!  (rip=0x%lx  ss:rsp=%lu:0x%lx)",
		regs->rip, regs->ss, regs->rsp);
}

extern char __usercopy_pagefault_address[];
extern char __userstrcpy_pagefault_address[];
extern char __usercopy_fixup_address[];

static inline bool
is_usercopy_addr (void *rip)
{
	return rip == __usercopy_pagefault_address
			|| rip == __userstrcpy_pagefault_address;
}

static bool
handle_usercopy_fault (unsigned long addr, struct entry_regs *regs)
{
	if (usercopy_ok ((void *) addr, 1) != ESUCCESS)
		return false;
	if (regs->error_code & PF_ID)
		return false;

	if (handle_pagefault_in_vm (addr, regs->error_code) != FAULT_HANDLED)
		regs->rip = (unsigned long) __usercopy_fixup_address;
	return true;
}

void
handle_PF_exception_k (struct entry_regs *regs)
{
	unsigned long addr = read_cr2 ();
	note_pagefault (addr, regs->error_code);
	if (is_usercopy_addr ((void *) regs->rip)) {
		if (handle_usercopy_fault (addr, regs))
			return;
	}
	panic ("x86: Page Fault in kernelspace!  (rip=0x%lx  ss:rsp=%lu:0x%lx)",
		regs->rip, regs->ss, regs->rsp);
}
