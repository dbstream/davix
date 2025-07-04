# GNUmakefile for the Davix operating system kernel.
# Copyright (C) 2025-present  dbstream

# Silence "make[n]: Entering directory..." and remove builtin rules and vars.
MAKEFLAGS += -R -r --no-print-directory

# Prevent out-of-tree invocations (make -f davix/GNUmakefile).
ifneq ($(abspath $(dir $(lastword $(MAKEFILE_LIST)))),$(CURDIR))
$(error Out-of-tree invocations are unsupported.)
endif

COMPILE_USER := $(shell whoami)
COMPILE_HOST := $(shell uname -n)
export COMPILE_USER COMPILE_HOST

# Build directory: use make O=... option if specified, otherwise build/.
ifeq ("$(origin O)","command line")
objtree := $(O)
else
objtree := build
endif

export objtree

# Verbosity: print commands if make V=... (not 0, n, or N) is specified.
Q := @
ifeq ("$(origin V)","command line")
ifneq ($(filter-out 0 n N,$(V)),)
Q :=
endif
endif

export Q

# Decide the target architecture. If the user does not supply an architecture,
# autodetect it.
ifeq ($(ARCH),)
override ARCH := $(shell uname -m)
endif

# x86_64 means x86.
ifeq ($(ARCH),x86_64)
override ARCH := x86
endif

export ARCH

include arch/$(ARCH)/toolchain_defs.GNUmakefile

# Toolchain definitions.
AS := gcc
CC := gcc
CXX := g++
LD := gcc
OBJCOPY := objcopy

ifneq ($(TOOLCHAIN_PREFIX),)
ifneq ($(shell which $(TOOLCHAIN_PREFIX)gcc),)

AS := $(TOOLCHAIN_PREFIX)gcc
CC := $(TOOLCHAIN_PREFIX)gcc
CXX := $(TOOLCHAIN_PREFIX)g++
LD := $(TOOLCHAIN_PREFIX)gcc
OBJCOPY := $(TOOLCHIAN_PREFIX)objcopy

else
$(warning No $(TOOLCHAIN_PREFIX) cross-toolchain detected.  Builds may fail.)
endif
endif

export AS CC CXX LD OBJCOPY

CC_VERSION := $(shell $(CC) --version | head -n 1)
export CC_VERSION

# C preprocessor flags.
CPPFLAGS :=					\
	-Iinclude				\
	-Iarch/$(ARCH)/include			\
	$(ARCH_CPPFLAGS)			\
	-include include/davix-predef.h		\

# Flags common to both C and C++ sources.
COMMONFLAGS :=				\
	-Wall				\
	-Wextra				\
	-Werror				\
	-O2				\
	-flto=auto			\
	-g				\
	-ffreestanding			\
	-fvisibility=hidden		\
	-nostdlib			\
	-nostdinc			\
	$(ARCH_COMMONFLAGS)		\

# Assembler flags.
AFLAGS := $(ARCH_AFLAGS)

# C compiler flags.
CFLAGS :=				\
	-std=gnu23			\
	$(ARCH_CFLAGS)			\

# C++ compiler flags.
CXXFLAGS :=				\
	-std=gnu++23			\
	-fno-exceptions			\
	-fno-rtti			\
	$(ARCH_CXXFLAGS)		\

# Linker flags.
LDFLAGS :=				\
	-ffreestanding			\
	-nostdlib			\
	-O2				\
	-flto=auto			\
	-Wl,--whole-archive		\
	$(ARCH_LDFLAGS)			\

LDLIBS := $(ARCH_LDLIBS)

# Flags for objcopy.
OBJCOPYFLAGS := $(ARCH_OBJCOPYFLAGS)

include config.GNUmakefile

export CPPFLAGS COMMONFLAGS AFLAGS CFLAGS CXXFLAGS LDFLAGS LDLIBS OBJCOPYFLAGS

# Passthrough everything except clean to build.GNUmakefile.
target_passthrough := $(or $(filter-out clean,$(MAKECMDGOALS)),all)
.PHONY: do_target_passthrough
do_target_passthrough:
	$(Q)mkdir -p -- $(objtree)
	$(Q)$(MAKE) -f tools/build.GNUmakefile subdir= $(MAKECMDGOALS)

$(target_passthrough): do_target_passthrough

ifeq ($(objtree),build)
.PHONY: clean
clean:
	$(Q)if [ -d build ]; then rm -r build; fi
endif
