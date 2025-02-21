/**
 * x86 exception handling.
 * Copyright (C) 2024  dbstream
 */
#include <davix/printk.h>
#include <asm/entry.h>
#include <asm/smp.h>
#include <asm/unwind.h>

static void
dump_registers (struct entry_regs *regs)
{
	printk (PR_INFO "pc=%016lx\n", regs->rip);
	printk (PR_INFO "RAX=%016lx RBX=%016lx RCX=%016lx RDX=%016lx\n",
		regs->saved_rax, regs->saved_rbx, regs->saved_rcx, regs->saved_rdx);
	printk (PR_INFO "RSI=%016lx RDI=%016lx RBP=%016lx RSP=%016lx\n",
		regs->saved_rsi, regs->saved_rdi, regs->saved_rbp, regs->rsp);
	printk (PR_INFO "R8 =%016lx R9 =%016lx R10=%016lx R11=%016lx\n",
		regs->saved_r8, regs->saved_r9, regs->saved_r10, regs->saved_r11);
	printk (PR_INFO "R12=%016lx R13=%016lx R14=%016lx R15=%016lx\n",
		regs->saved_r12, regs->saved_r13, regs->saved_r14, regs->saved_r15);
}

void
handle_BP_interrupt (struct entry_regs *regs)
{
	printk (PR_INFO "INT3 exception from userspace\n");
	dump_registers (regs);
}

void
handle_BP_interrupt_k (struct entry_regs *regs)
{
	printk (PR_INFO "INT3 exception from kernelspace\n");
	dump_registers (regs);
	dump_backtrace ();
}
