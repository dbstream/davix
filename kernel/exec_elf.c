/**
 * Loading ELF executables.
 * Copyright (C) 2025-present  dbstream
 */
#include <davix/align.h>
#include <davix/exec.h>
#include <davix/file.h>
#include <davix/kmalloc.h>
#include <davix/mm.h>
#include <davix/printk.h>
#include <asm/page_defs.h>

#include <asm/elf.h>
#include ARCH_ELF_HEADER

static inline bool
read_file (struct file *file, void *buf, size_t size, off_t offset)
{
	ssize_t nread;
	errno_t e = kernel_read_file (file, buf, size, offset, &nread);
	if (e != ESUCCESS)
		return false;
	if ((size_t) nread != size)
		return false;
	return true;
}

/**
 * Verify that this is an ELF file we support.
 */
static inline bool
perform_basic_checks (ElfXX_Ehdr hdr)
{
	if (hdr.e_ident[EI_MAG0] != ELFMAG0 || hdr.e_ident[EI_MAG1] != ELFMAG1
		|| hdr.e_ident[EI_MAG2] != ELFMAG2 || hdr.e_ident[EI_MAG3] != ELFMAG3)
			// not an ELF file:  don't print anything
			return false;

	if (hdr.e_ident[EI_CLASS] != ELFCLASSXX) {
		printk (PR_INFO "exec_elf: e_ident[EI_CLASS]=%d is not supported on this architecture\n",
				hdr.e_ident[EI_CLASS]);
		return false;
	}

	if (!elf_is_suppported_data (hdr.e_ident[EI_DATA])) {
		printk (PR_INFO "exec_elf: e_ident[EI_DATA]=%d is not supported on this architecture\n",
				hdr.e_ident[EI_DATA]);
		return false;
	}

	if (hdr.e_type != ET_EXEC && hdr.e_type != ET_DYN) {
		printk (PR_INFO "exec_elf: e_type=%d is not ET_EXEC nor ET_DYN\n",
				hdr.e_type);
		return false;
	}

	if (!elf_is_supported_machine (hdr.e_machine)) {
		printk (PR_INFO "exec_elf: e_machine=%d is not supported on this architecture\n",
				hdr.e_machine);
		return false;
	}

	if (hdr.e_version != EV_CURRENT) {
		printk (PR_INFO "exec_elf: e_version=%d != EV_CURRENT\n",
				hdr.e_version);
		return false;
	}

	if (hdr.e_phentsize != sizeof (ElfXX_Phdr)) {
		printk (PR_INFO "exec_elf: e_phentsize=%d != sizeof (ElfXX_Phdr)\n",
				hdr.e_phentsize);
		return false;
	}

	return true;
}

static bool
should_load_phdr (ElfXX_Word p_type)
{
	return p_type == PT_LOAD;
}

static errno_t
load_elf_sections (struct file *file, ElfXX_Ehdr *hdr)
{
	for (ElfXX_Half i = 0; i < hdr->e_phnum; i++) {
		ElfXX_Phdr phdr;
		if (!read_file (file, &phdr, sizeof (phdr),
				hdr->e_phoff + i * hdr->e_phentsize)) {
			printk (PR_WARN "exec_elf: read_file() failed\n");
			return EIO;
		}

		if (!should_load_phdr (phdr.p_type))
			continue;

		if (!phdr.p_memsz)
			continue;

		int section_prot = 0;
		if (phdr.p_flags & PF_R)	section_prot |= PROT_READ;
		if (phdr.p_flags & PF_W)	section_prot |= PROT_WRITE;
		if (phdr.p_flags & PF_X)	section_prot |= PROT_EXEC;

		unsigned long file_mmap_size = ALIGN_UP (phdr.p_filesz, PAGE_SIZE);
		if (file_mmap_size) {
			errno_t e = exec_mmap_file (phdr.p_paddr,
					phdr.p_filesz, section_prot, file, phdr.p_offset);
			if (e != ESUCCESS) {
				printk (PR_WARN "exec_elf: exec_mmap_file() failed\n");
				return e;
			}
		}

		if (file_mmap_size < phdr.p_memsz) {
			errno_t e = ksys_mmap ((void *) (phdr.p_paddr + file_mmap_size),
					phdr.p_memsz - file_mmap_size, section_prot,
					MAP_ANON | MAP_PRIVATE, NULL, 0, NULL);
			if (e != ESUCCESS) {
				printk (PR_WARN "exec_elf: ksys_mmap() failed\n");
				return e;
			}
		}
	}

	printk (PR_INFO "exec_elf: load_elf_sections is successful\n");
	return ESUCCESS;
}

static errno_t
load_elf (struct exec_state *state, ElfXX_Ehdr *hdr, int stack_prot, char *interp)
{
	(void) stack_prot;
	if (interp)
		return ENOTSUP;

	errno_t e = load_elf_sections (state->file, hdr);
	if (e != ESUCCESS)
		return e;

	state->entry_point_addr = hdr->e_entry;
	state->stack_grows_down = ARCH_STACK_GROWS_DOWN;
	state->user_stack_reserved = 0x8000UL;
	e = exec_mmap_stack (state, 0x10000UL, stack_prot);
	if (e != ESUCCESS)
		return e;

	// TODO: push argv, envp, and auxvals onto the stack

	return ESUCCESS;
}

static errno_t
elf_do_execve (struct exec_state *state)
{
	(void) state;

	ElfXX_Ehdr hdr;
	if (!read_file (state->file, &hdr, sizeof (hdr), 0))
		return ENOEXEC;

	if (!perform_basic_checks (hdr))
		return ENOEXEC;

	/**
	 * From https://docs.kernel.org/next/ELF/ELF.html:
	 * - The first PT_INTERP program header is used to locate the filename
	 *   of the ELF interpreter.  Other PT_INTERP headers are ignored.
	 * - The last PT_GNU_STACK program header defines userspace stack
	 *   executability.  Other PT_GNU_STACK headers are ignored.
	 */

	int stack_prot = PROT_READ | PROT_WRITE;
	char *interp = NULL;

	for (ElfXX_Half i = 0; i < hdr.e_phnum; i++) {
		ElfXX_Phdr phdr;
		if (!read_file (state->file, &phdr, sizeof (phdr),
				hdr.e_phoff + i * hdr.e_phentsize)) {

			kfree (interp);
			return ENOEXEC;
		}

		if (phdr.p_type == PT_GNU_STACK) {
			stack_prot = PROT_READ | PROT_WRITE | ((
					phdr.p_flags & PF_X) ? PROT_EXEC : 0);
			continue;
		}

		if (phdr.p_type == PT_INTERP) {
			if (interp)
				continue;
			if (phdr.p_filesz > 4096) {
				printk (PR_WARN "exec_elf: PT_INTERP is really large\n");
				return ENOEXEC;
			}
			interp = kmalloc (phdr.p_filesz + 1);
			if (!interp)
				return ENOMEM;

			interp[phdr.p_filesz] = 0;
			if (!read_file (state->file, interp, phdr.p_filesz, phdr.p_offset)) {
				kfree (interp);
				return ENOEXEC;
			}

			printk (PR_INFO "exec_elf: PT_INTERP=\"%s\"\n",
					interp);

			continue;
		}
	}

	errno_t e = exec_point_of_no_return (state);
	if (e == ESUCCESS)
		e = load_elf (state, &hdr, stack_prot, interp);

	kfree (interp);
	return e;
}

const struct exec_driver exec_driver_elf = {
	.do_execve = elf_do_execve
};
