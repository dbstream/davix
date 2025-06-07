/**
 * Interrupt Descriptor Table (IDT) management.
 * Copyright (C) 2025-present  dbstream
 */
#include <asm/asm.h>
#include <asm/entry.h>
#include <asm/gdt.h>
#include <asm/idt.h>
#include <asm/interrupt.h>
#include <asm/trap.h>
#include <davix/panic.h>
#include <davix/printk.h>
#include <stdint.h>
#include <vsnprintf.h>

struct [[gnu::packed]] idt_entry {
	uint16_t offset0;
	uint16_t cs;
	uint8_t ist;
	uint8_t flags;
	uint16_t offset1;
	uint32_t offset2;
	uint32_t reserved;
};

static volatile idt_entry idt_table alignas(0x1000) [256];

static void
set_idt_entry (int idx, void *handler, int ist, int dpl)
{
	idt_table[idx].offset0 = (uintptr_t) handler;
	idt_table[idx].cs = __KERNEL_CS;
	idt_table[idx].ist = ist;
	idt_table[idx].flags = 0x8e | (dpl << 5);
	idt_table[idx].offset1 = (uintptr_t) handler >> 16;
	idt_table[idx].offset2 = (uintptr_t) handler >> 32;
	idt_table[idx].reserved = 0;
}

struct [[gnu::packed]] segment_ptr {
	uint16_t limit;
	uint64_t address;
};

static void
load_idt (void)
{
	segment_ptr idt_ptr = { 0xfff, (uintptr_t) idt_table };
	asm volatile ("lidt %0" :: "m" (idt_ptr) : "memory");
}

extern "C" char asm_handle_GP[];
extern "C" char asm_handle_PF[];

extern "C" void *asm_idtentry_vector_array[];

void
x86_setup_idt (void)
{
	set_idt_entry (X86_TRAP_GP, asm_handle_GP, 0, 0);
	set_idt_entry (X86_TRAP_PF, asm_handle_PF, 0, 0);

	for (int i = 0; i < IRQ_VECTOR_NUM; i++)
		set_idt_entry (IRQ_VECTOR_OFFSET + i, asm_idtentry_vector_array[i], 0, 0);

	load_idt ();
}

void
x86_ap_setup_idt (void)
{
	load_idt ();
}

static void
panic_with_regs (const char *msg, entry_regs *regs)
{
	panic ("%s\n"
		"  RIP: %016lx  RFLAGS: %016lx\n"
		"  RAX: %016lx  RBX: %016lx  RCX: %016lx  RDX: %016lx\n"
		"  RDI: %016lx  RSI: %016lx  RBP: %016lx  RSP: %016lx\n"
		"  R8:  %016lx  R9:  %016lx  R10: %016lx  R11: %016lx\n"
		"  R12: %016lx  R13: %016lx  R14: %016lx  R15: %016lx",
		msg,
		regs->rip, regs->rflags,
		regs->saved_rax, regs->saved_rbx, regs->saved_rcx, regs->saved_rdx,
		regs->saved_rdi, regs->saved_rsi, regs->saved_rbp, regs->rsp,
		regs->saved_r8, regs->saved_r9, regs->saved_r10, regs->saved_r11,
		regs->saved_r12, regs->saved_r13, regs->saved_r14, regs->saved_r15);
}

extern "C"
void
handle_GP_exception (entry_regs *regs)
{
	panic_with_regs ("General-Protection fault in userspace!", regs);
}

extern "C"
void
handle_GP_exception_k (entry_regs *regs)
{
	panic_with_regs ("General-Protection fault in kernel space!", regs);
}

enum : uint32_t {
	PF_P			= 0x00000001U,
	PF_WR			= 0x00000002U,
	PF_US			= 0x00000004U,
	PF_RSVD			= 0x00000008U,
	PF_ID			= 0x00000010U,
	PF_PK			= 0x00000020U,
	PF_SS			= 0x00000040U,
	PF_HLAT			= 0x00000080U,
	PF_SGX			= 0x00001000U,
};

static void
panic_on_page_fault (uintptr_t addr, entry_regs *regs)
{
	uint32_t error_code = regs->error_code;
	const char *by = (error_code & PF_US) ? "usermode" : "kernelmode";
	const char *id = (error_code & PF_SS) ? "shadow stack" :
		(error_code & PF_ID) ? "instruction" : "data";
	const char *tp = (error_code & PF_WR) ? "write" : "read";
	const char *cause = (error_code & PF_RSVD) ? "reserved bit set in PTE" :
		!(error_code & PF_P) ? "nonpresent PTE" :
		(error_code & PF_PK) ? "protection-key rights disallow access" :
		(error_code & PF_WR) ? "readonly PTE" : "unknown";

	char buf[128];
	snprintf (buf, sizeof(buf), "Page fault on address 0x%lx! cause: %s %s %s with %s.",
			addr, by, id, tp, cause);
	panic_with_regs (buf, regs);
}

extern "C"
void
handle_PF_exception (entry_regs *regs)
{
	uintptr_t fault_addr = read_cr2 ();
	panic_on_page_fault (fault_addr, regs);
}

extern "C"
void
handle_PF_exception_k (entry_regs *regs)
{
	uintptr_t fault_addr = read_cr2 ();
	panic_on_page_fault (fault_addr, regs);
}
