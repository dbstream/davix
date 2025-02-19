/**
 * Kernel stack unwinding.
 * Copyright (C) 2025-present  dbstream
 *
 * Ok, unwinding is still TODO... for now, just demonstrate our ability to
 * read the embed data.
 */
#include <davix/printk.h>
#include <asm/sections.h>
#include <asm/unwind.h>

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

void
dump_backtrace (void)
{
	unsigned long addr = (unsigned long) &dump_backtrace + 10;
	struct symbol_data sym = find_symbol (addr);
	printk ("symbol lookup test:  0x%lx  (%s + 0x%lx)\n",
			addr, sym.name, addr - sym.start);
}
