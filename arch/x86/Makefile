# SPDX-License-Identifier: MIT
# x86-specific Makefile

CC := x86_64-elf-gcc
CFLAGS_BASE := \
	-std=gnu17 \
	-nostdinc \
	-Iinclude \
	-Iarch/x86/include \
	-I \
	-Wall \
	-Werror \
	-O2 -g \
	-D__KERNEL__ \
	-D__X86__ \
	-ffreestanding \
	-mno-red-zone \
	-mno-80387 \
	-mno-mmx \
	-mno-sse \
	-mno-sse2 \
	-mgeneral-regs-only
CFLAGS := $(CFLAGS_BASE) -mcmodel=kernel
CFLAGS_LOADER := $(CFLAGS_BASE) -mcmodel=large

ASFLAGS := -Iinclude -Iarch/x86/include -D__KERNEL__ -D__X86__ -g

ifndef SPARSE
ifeq (,$(shell which sparse))
$(warning No sparse in PATH, skipping...)
else
SPARSE := sparse
endif
endif

ifdef SPARSE
SPARSE_FLAGS := \
	-std=gnu17 \
	-nostdinc \
	-Iinclude \
	-Iarch/x86/include \
	-Wall \
	-Wsparse-all \
	-Wno-declaration-after-statement \
	-Werror \
	-Wsparse-error \
	-D__KERNEL__ \
	-D__X86__ \
	-ffreestanding \
	-mno-red-zone \
	-mno-80387 \
	-mno-mmx \
	-mno-sse \
	-mno-sse2 \
	-mgeneral-regs-only
endif

__loader_obj :=

obj :=
include arch/x86/kernel/Makefile
__obj += $(patsubst %,$(objtree)/arch/x86/kernel/%,$(obj))

loader_obj :=
include arch/x86/loader/Makefile
__loader_obj += $(patsubst %,$(objtree)/arch/x86/loader/%,$(loader_obj))

$(filter $(objtree)/%.c.o,$(__obj)): $(objtree)/%.c.o: %.c
	@mkdir -p $(dir $@)
	@echo -e CC\\t$<
ifdef SPARSE
	@$(SPARSE) $(SPARSE_FLAGS) -c $<
endif
	@$(CC) $(CFLAGS) -c -o $@ $< -MD -MP -MF $@.d

$(filter $(objtree)/%.c.o,$(__loader_obj)): $(objtree)/%.c.o: %.c
	@mkdir -p $(dir $@)
	@echo -e CC\\t$<
ifdef SPARSE
	@$(SPARSE) $(SPARSE_FLAGS) -c $<
endif
	@$(CC) $(CFLAGS_LOADER) -c -o $@ $< -MD -MP -MF $@.d

$(objtree)/%.S.o: %.S
	@mkdir -p $(dir $@)
	@echo -e AS\\t$<
	@$(CC) $(ASFLAGS) -c -o $@ $<

$(objtree)/mainkernel: $(__obj)
	@mkdir -p $(dir $@)
	@echo -e LINK\\t$@
	@$(CC) \
		-zmax-page-size=0x1000 \
		-nostdlib \
		-Tarch/x86/kernel.lds \
		-o $@ $(__obj)

$(objtree)/davix: $(__loader_obj)
	@mkdir -p $(dir $@)
	@echo -e LINK\\t$@
	@$(CC) \
		-zmax-page-size=0x1000 \
		-nostdlib \
		-Tarch/x86/loader.lds \
		-o $@ $(__loader_obj)

PHONY += all
all: $(objtree)/davix

dep += $(patsubst %.o,%.o.d,$(__loader_obj))
