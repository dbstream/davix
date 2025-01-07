/**
 * Pagefault handler.
 * Copyright (C) 2025-present  dbstream
 *
 * We also implement the Stack-Segment (SS) Fault and General Protection (GP)
 * handlers here.
 */
#include <davix/panic.h>
#include <davix/printk.h>
#include <asm/cregs.h>
#include <asm/entry.h>

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

void
handle_PF_exception (struct entry_regs *regs)
{
	note_pagefault (read_cr2 (), regs->error_code);
	panic ("x86: Page Fault in userspace!  (rip=0x%lx  ss:rsp=%lu:0x%lx)",
		regs->rip, regs->ss, regs->rsp);
}

void
handle_PF_exception_k (struct entry_regs *regs)
{
	note_pagefault (read_cr2 (), regs->error_code);
	panic ("x86: Page Fault in kernelspace!  (rip=0x%lx  ss:rsp=%lu:0x%lx)",
		regs->rip, regs->ss, regs->rsp);
}
