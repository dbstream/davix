# SPDX-License-Identifier: MIT

.PHONY: all
all:

CFLAGS := \
	-std=gnu17 \
	-nostdinc \
	$(DAVIXINCLUDE) \
	-Wall \
	-Werror \
	-O2 -g \
	-D__KERNEL__ \
	-ffreestanding \
	-mno-red-zone \
	-mno-80387 \
	-mno-mmx \
	-mno-sse \
	-mno-sse2 \
	-mgeneral-regs-only \
	-mcmodel=large

AFLAGS := \
	$(DAVIXINCLUDE) \
	-D__KERNEL__ \
	-g

obj-y :=
obj-y += header.S.o
obj-y += include_kernel.S.o
obj-y += main.c.o
obj-y += memalloc.c.o
obj-y += memmap.c.o
obj-y += stdlib.c.o
obj-y += uart.c.o

AFLAGS-include_kernel.S.o := '-DPATH_TO_MAINKERNEL="$(objtree)/davix-kernel"'

real-obj-y := $(addprefix $(objtree)/arch/x86/loader/,$(obj-y))

$(objtree)/%.c.o: $(srctree)/%.c
	@mkdir -p $(dir $@)
	@echo -e CC\\tarch/x86/loader/$(notdir $<)
	$(Q)$(CC) $(CFLAGS) $(CFLAGS-$(notdir $@)) -c -o $@ $< -MD -MP -MF $@.d

$(objtree)/%.S.o: $(srctree)/%.S
	@mkdir -p $(dir $@)
	@echo -e AS\\tarch/x86/loader/$(notdir $<)
	$(Q)$(CC) $(AFLAGS) $(AFLAGS-$(notdir $@)) -c -o $@ $< -MD -MP -MF $@.d

$(objtree)/arch/x86/loader/include_kernel.S.o: $(objtree)/davix-kernel

$(objtree)/davix-multiboot2: $(real-obj-y)
	@echo -e LINK\\t$(notdir $@)
	$(Q)$(CC) -zmax-page-size=0x1000 -nostdlib -T$(srctree)/arch/x86/loader.lds -o $@ $(real-obj-y)

-include $(filter %.d,$(patsubst %.o,%.o.d,$(real-obj-y)))
