# Toolchain definitions for x86.
# Copyright (C) 2025-present  dbstream

TOOLCHAIN_PREFIX := x86_64-elf-

ARCH_COMMONFLAGS :=				\
	-march=x86-64				\
	-mabi=sysv				\
	-mcmodel=kernel				\
	-mno-red-zone				\
	-mno-80387				\
	-mno-mmx				\
	-mno-sse				\
	-mno-sse2				\
	-mgeneral-regs-only			\
	-fno-PIC				\
	-fstack-protector			\
	-mstack-protector-guard=tls		\
	-mstack-protector-guard-reg=gs		\
	-mstack-protector-guard-offset=40	\

ARCH_AFLAGS :=					\
	-march=x86-64				\
	-mabi=sysv				\
	-fno-PIC				\

ARCH_OBJCOPYFLAGS :=

ARCH_LDFLAGS :=					\
	-march=x86-64				\
	-mabi=sysv				\
	-mcmodel=kernel				\
	-mno-red-zone				\
	-mno-80387				\
	-mno-mmx				\
	-mno-sse				\
	-mno-sse2				\
	-mgeneral-regs-only			\
	-fno-PIC				\
	-fstack-protector			\
	-mstack-protector-guard=tls		\
	-mstack-protector-guard-reg=gs		\
	-mstack-protector-guard-offset=40	\
	-nostdlib				\

ARCH_LDLIBS :=
