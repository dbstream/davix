/**
 * Generate the contents of the "embedded" data.
 * Copyright (C) 2025-present  dbstream
 *
 * The final kernel binary is appended with the embed data generated by this
 * file.  This is used for kernel access to the kernel symbols and for stack
 * unwind information.
 */
#include <endian.h>
#include <fcntl.h>
#include <gelf.h>
#include <libelf.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <string>
#include <vector>

static Elf *kernel;

static std::vector<uint8_t> output;

static inline uint64_t
add_cstring (const char *s)
{
	uint64_t offset = output.size ();
	do {
		output.push_back (*s);
	} while (*(s++));
	return offset;
}

static inline uint64_t
align_to_8 (void)
{
	for (size_t i = output.size(); i & 7; i++)
		output.push_back (0);
	return output.size ();
}

static inline uint64_t
add_zeroes (uint64_t n)
{
	uint64_t offset = output.size ();
	while (n--) output.push_back (0);
	return offset;
}

static inline uint64_t
set_u64 (uint64_t offset, uint64_t value)
{
	output[offset + 0] = value >> (8 * 0);
	output[offset + 1] = value >> (8 * 1);
	output[offset + 2] = value >> (8 * 2);
	output[offset + 3] = value >> (8 * 3);
	output[offset + 4] = value >> (8 * 4);
	output[offset + 5] = value >> (8 * 5);
	output[offset + 6] = value >> (8 * 6);
	output[offset + 7] = value >> (8 * 7);
	return offset;
}

static inline uint64_t
add_output (uint64_t x)
{
	return set_u64 (add_zeroes (8), x);
}

struct Symbol {
	uint64_t value;
	uint64_t size;
	uint64_t name_offset;
	std::string name;
};

static std::vector<Symbol> symbols;

int
main (int argc, char **argv)
{
	if (argc != 2) {
		fprintf (stderr, "usage:  gen_embed path/to/davix.elf > output\n");
		return 1;
	}

	if (elf_version (EV_CURRENT) != EV_CURRENT) {
		fprintf (stderr, "failed to initialize libelf: %s\n", elf_errmsg (-1));
		return 1;
	}

	int fd = open (argv[1], O_RDONLY);
	if (fd == -1) {
		perror ("failed to open kernel ELF binary");
		return 1;
	}

	kernel = elf_begin (fd, ELF_C_READ, NULL);
	if (!kernel) {
		fprintf (stderr, "elf_begin() failed: %s\n", elf_errmsg(-1));
		return 1;
	}

	if (elf_kind (kernel) != ELF_K_ELF) {
		fprintf (stderr, "elf_kind() is not ELF_K_ELF\n");
		return 1;
	}

	/**
	 * embed[0...7] = sizeof embed
	 * embed[8...15] = symbol offset
	 * embed[16...23] = symbol count
	 */
	add_zeroes (24);

	Elf_Scn *scn = NULL;
	GElf_Shdr shdr;
	while ((scn = elf_nextscn (kernel, scn))) {
		if (gelf_getshdr (scn, &shdr) != &shdr) {
			fprintf (stderr, "gelf_getshdr() failed: %s\n", elf_errmsg (-1));
			return 1;
		}

		if (shdr.sh_type == SHT_SYMTAB)
			break;
	}

	if (!scn) {
		fprintf (stderr, "couldn't find SHT_SYMTAB in kernel ELF binary\n");
		return 1;
	}

	Elf_Data *data = elf_getdata (scn, NULL);
	if (!data) {
		fprintf (stderr, "elf_getdata() failed: %s\n", elf_errmsg (-1));
		return 1;
	}

	const auto count = shdr.sh_size / shdr.sh_entsize;
	for (auto i = 0; i < count; i++) {
		GElf_Sym sym;
		if (gelf_getsym (data, i, &sym) != &sym) {
			fprintf (stderr, "gelf_getsym() failed: %s\n", elf_errmsg (-1));
			return 1;
		}

		unsigned char type = ELF64_ST_TYPE (sym.st_info);
		if (type != STT_FUNC)
			continue;

		const char *p = elf_strptr (kernel, shdr.sh_link, sym.st_name);
		if (!p) {
			fprintf (stderr, "elf_strptr() failed: %s\n", elf_errmsg (-1));
			return 1;
		}

		Symbol s = { sym.st_value, sym.st_size, 0, p };
		symbols.push_back (std::move (s));
	}

	std::sort (std::begin (symbols), std::end (symbols), [](const Symbol &lhs, const Symbol &rhs){
		return lhs.value < rhs.value;
	});

	for (Symbol &sym : symbols)
		sym.name_offset = add_cstring (sym.name.c_str ());

	set_u64 (8, align_to_8 ());
	set_u64 (16, symbols.size ());
	for (Symbol &sym : symbols) {
		add_output (sym.value);
		add_output (sym.size);
		add_output (sym.name_offset);
	}

	set_u64 (0, output.size ());
	if (write (STDOUT_FILENO, output.data (), output.size()) != output.size ()) {
		fprintf (stderr, "failed to write output\n");
		return 1;
	}

	return 0;
}

