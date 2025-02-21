/**
 * Kernel stack unwinding.
 * Copyright (C) 2025-present  dbstream
 */
#include <davix/context.h>
#include <davix/printk.h>
#include <davix/sched_types.h>
#include <davix/stdbool.h>
#include <davix/stdint.h>
#include <asm/gdt.h>
#include <asm/sections.h>
#include <asm/trap.h>
#include <asm/unwind.h>

/**
 * The format of the embedded information is documented in:
 *	arch/x86/tools/gen_embed.cc
 */

static inline const char *
get_embedded_name (unsigned long offset)
{
	return (const char *) &__kernel_end[offset];
}

static inline unsigned long
read_embed_data (unsigned long offset)
{
	return *(unsigned long *) &__kernel_end[offset];
}

struct symbol_data {
	const char *name;
	unsigned long start;
	unsigned long size;
};

static struct symbol_data
find_symbol (unsigned long addr)
{
	unsigned long a, b, m, o, start, size;

	o = read_embed_data (8);

	a = 0;
	b = read_embed_data (16) - 1;
	while (a != b) {
		m = (a + b + 1) / 2;
		if (read_embed_data (o + 24 * m) > addr)
			b = m - 1;
		else
			a = m;
	}

	start = read_embed_data (o + 24 * a + 0);
	size = read_embed_data (o + 24 * a + 8);
	const char *name = get_embedded_name (read_embed_data (o + 24 * a + 16));

	if (addr < start || (start + size && addr >= start + size)) {
		start = 0;
		size = 0;
		name = "?";
	}

	return (struct symbol_data) { name, start, size };
}

struct cfi_data {
	uint16_t offset;
	bool is_signal_frame;
};

static bool
find_cfi_data (unsigned long addr, struct cfi_data *out)
{
	unsigned long a, b, m, o, x;

	o = read_embed_data (24);

	a = 0;
	b = read_embed_data (32) - 1;
	while (a != b) {
		m = (a + b + 1) / 2;
		if (read_embed_data (o + 16 * m) > addr)
			b = m - 1;
		else
			a = m;
	}

	if (!a && read_embed_data (o) > addr)
		return false;

	x = read_embed_data (o + 16 * a + 8);
	if (!(x & 0x80UL))
		return false;

	out->is_signal_frame = (x & 0x40UL) ? true : false;
	out->offset = x >> 48;
	return true;
}

struct unwind_state {
	int num_stacks;
	unsigned long stack_start[2];
	unsigned long stack_end[2];
	unsigned long current_sp;
	unsigned long unwound_sp;
	unsigned long current_ip;
	unsigned long next_sp;
};

/**
 * Find the boundaries for the stack(s) we are unwinding.  Returns false if we
 * failed, e.g. because state->current_sp is garbage.
 */
static inline bool
find_stacks (struct unwind_state *state)
{
	struct task *task = get_current_task ();
	if (!task)
		return false;

	state->stack_start[0] = (unsigned long) task->arch.stack_mem;
	state->stack_end[0] = state->stack_start[0] + TASK_STK_SIZE;
	state->num_stacks = 1;
	if (state->current_sp >= state->stack_start[0] && state->current_sp < state->stack_end[0])
		return true;

	for (int i = 0; i < TRAP_STK_NUM; i++) {
		unsigned long start, end;
		start = (unsigned long) this_cpu_read (&x86_ist_stacks[i]);
		end = start + TRAP_STK_SIZE;
		if (state->current_sp >= start && state->current_sp < end) {
			state->stack_start[1] = start;
			state->stack_end[1] = end;
			state->num_stacks = 2;
			if (!state->stack_start[0]) {
				state->stack_start[0] = start;
				state->stack_end[0] = end;
				state->num_stacks = 1;
			}
			return true;
		}
	}

	return false;
}

/**
 * Initialize the unwind_state.  Returns false if we fail.
 */
static __attribute__ ((noinline)) bool
init_unwind_state (struct unwind_state *state)
{
	asm ("movq %%rsp, %0; leaq 0(%%rip), %1"
			: "=r" (state->current_sp),
			"=r" (state->current_ip));
	state->unwound_sp = 0;
	state->next_sp = state->current_sp;

	preempt_off ();
	bool ret = find_stacks (state);
	preempt_on ();
	return ret;
}

#define UNWIND_END 0
#define UNWIND_CONT 1
#define UNWIND_BAD 2

/**
 * Unwind intelligently to the next stack pointer.  This operates on next_sp.
 *
 * Modifications to unwind_state:
 *	state->current_ip is set to the return address that CFI unwind finds.
 *	state->unwound_sp is set to the SP at which state->current_ip was found.
 *	state->next_sp is set to the SP that unwind_next will continue from.
 */
static int
unwind_next (struct unwind_state *state)
{
	struct cfi_data cfi;
	if (!find_cfi_data (state->current_ip, &cfi))
		return UNWIND_END;

	unsigned long start = state->stack_start[state->num_stacks - 1];
	unsigned long end = state->stack_end[state->num_stacks - 1];
	unsigned long sp = state->next_sp + cfi.offset;
	if (cfi.is_signal_frame) {
		if (sp < start || sp + 40 > end || (sp & 7))
			return UNWIND_BAD;

		uint16_t cs = *(uint16_t *) (sp + 8);
		if (cs != __KERNEL_CS)
			return UNWIND_END;

		state->unwound_sp = sp;
		state->current_ip = *(unsigned long *) sp;
		state->next_sp = *(unsigned long *) (sp + 24);
		return UNWIND_CONT;
	} else {
		sp -= 8;
		if (sp < start || sp >= end || (sp & 7))
			return UNWIND_BAD;

		state->current_ip = *(unsigned long *) sp;
		state->unwound_sp = sp;
		state->next_sp = sp + 8;
		return UNWIND_CONT;
	}
}

void
dump_backtrace (void)
{
	if (0) {
		find_symbol (0);
		find_cfi_data (0, NULL);
	}

	struct unwind_state state;
	if (!init_unwind_state (&state)) {
		printk (PR_ERR "Failed to initialize unwind_state.  This is a kernel bug.\n");
		return;
	}

	printk (PR_INFO "Stack trace:\n");

	int num = 1;
	bool unwind_bad = false;
	for (;;) {
		unsigned long start = state.stack_start[state.num_stacks - 1];
		unsigned long end = state.stack_end[state.num_stacks - 1];
		int status;
		if ((state.next_sp & 7))
			status = UNWIND_BAD;
		else if (state.next_sp < start || state.next_sp >= end)
			status = UNWIND_BAD;
		else if (unwind_bad)
			status = UNWIND_BAD;
		else
			status = unwind_next (&state);

		if (status == UNWIND_END)
			break;

		if (status != UNWIND_BAD) {
			if (state.unwound_sp < state.current_sp
					&& state.unwound_sp >= start)
				status = UNWIND_BAD;
		}
		
		unsigned long frame_end;
		if (status == UNWIND_BAD) {
			if (!unwind_bad) {
				printk (PR_INFO "    (CFI-based unwind failed)\n");
				unwind_bad = true;
			}
			frame_end = end;
		} else if (state.unwound_sp < start || state.unwound_sp >= end)
			frame_end = end;
		else
			frame_end = state.unwound_sp + 8;

		while (state.current_sp < frame_end) {
			unsigned long addr = *(unsigned long *) state.current_sp;
			struct symbol_data sym = find_symbol (addr);
			if (state.current_sp == state.unwound_sp)
				printk (PR_INFO "  #%02d    0x%lx  (%s+0x%lx)\n",
						num++, addr,
						sym.name, addr - sym.start);
			else if (sym.size != 0)
				printk (PR_INFO "  #%02d  ? 0x%lx  (%s+0x%lx)\n",
						num++, addr,
						sym.name, addr - sym.start);
			state.current_sp += 8;
			if (num > 30) {
				printk (PR_INFO "  --- stack trace truncated ---\n");
				return;
			}
		}

		if (unwind_bad)
			state.next_sp = state.current_sp;
		if (state.next_sp < state.current_sp || state.next_sp >= end) {
			if (!--state.num_stacks)
				break;

			start = state.stack_start[state.num_stacks - 1];
			end = state.stack_end[state.num_stacks - 1];
			if (!unwind_bad && !(state.next_sp & 7)
					&& state.next_sp >= start
					&& state.unwound_sp < end)
				state.current_sp = state.next_sp - 8;
			else
				state.current_sp = start;
			printk ("alright, went up an interrupt stack.  start=%lx  end=%lx  current_ip=%lx  current_sp=%lx\n",
					start, end,
					state.current_ip, state.current_sp);
		}
	}
	printk (PR_INFO "  --- end of stack trace ---\n");
}
