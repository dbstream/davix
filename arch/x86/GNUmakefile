# x86-specific Makefile.
# Copyright (C) 2024  dbstream

ifeq ($(subdir),)

CC := x86_64-elf-gcc
AS := x86_64-elf-gcc
LD := x86_64-elf-ld
OBJCOPY := x86_64-elf-objcopy

CFLAGS-init := $(CFLAGS)	\
	-march=x86-64		\
	-mabi=sysv		\
	-mcmodel=small		\
	-mno-80387		\
	-mno-mmx		\
	-mno-sse		\
	-mno-sse2		\
	-mgeneral-regs-only	\
	-fno-stack-check	\
	-fno-lto		\
	-fno-PIC		\
	-fno-stack-protector

LDFLAGS-init := \
	-nostdlib		\
	-zmax-page-size=0x1000	\
	--no-dynamic-linker	\
	-static

CFLAGS += \
	-march=x86-64		\
	-mabi=sysv		\
	-mcmodel=kernel		\
	-mno-red-zone		\
	-mno-80387		\
	-mno-mmx		\
	-mno-sse		\
	-mno-sse2		\
	-mgeneral-regs-only	\
	-fno-stack-check	\
	-fno-lto		\
	-fno-PIC		\
	-fstack-protector	\
	-mstack-protector-guard=tls		\
	-mstack-protector-guard-reg=gs		\
	-mstack-protector-guard-offset=40

AFLAGS += \
	-march=x86-64		\
	-mabi=sysv		\
	-fno-lto		\
	-fno-PIC

LDFLAGS-davix := \
	-nostdlib		\
	-zmax-page-size=0x1000	\
	--no-dynamic-linker	\
	-static			\
	--emit-relocs		\
	-T

LDLIBS-davix :=

targets += $(obj)kernel.lds
LDSCRIPT-davix := $(obj)kernel.lds
$(obj)kernel.lds: $(src)arch/$(ARCH)/kernel.lds.S FORCE
	$(call cond_cmd,cpp)

# FIXME: This is really annoying.  It also doesn't automatically re-run if
# davix.elf changes.
$(obj)kernel_end: $(obj)davix.elf
	$(Q)printf '0x%x' $$(($$(nm $(obj)/davix.elf | awk '/[0-9a-f]+ . __kernel_end/ { print "0x"$$1 }') - 0xffffffff80000000)) > $@

targets += $(obj)davix.intermediary
pri_objcopy = OBJCOPY $(target)
cmd_objcopy = $(OBJCOPY) -O binary --remove-section=.eh_frame --pad-to=$(shell cat $(obj)kernel_end) $< $@
$(obj)davix.intermediary: $(obj)davix.elf $(obj)kernel_end FORCE
	$(call cond_cmd,objcopy)

$(obj)arch/x86/tools/gen_embed: FORCE
	$(Q)mkdir -p -- $(obj)arch/x86/tools
	$(Q)$(MAKE) -f $(BUILD_MAKEFILE) subdir=$(subdir)arch/x86/tools/ $(obj)arch/x86/tools/gen_embed

targets += $(obj)davix.embed
pri_embed = EMBED   $(target)
cmd_embed = $(obj)arch/x86/tools/gen_embed $< > $@
$(obj)davix.embed: $(obj)davix.elf $(obj)arch/x86/tools/gen_embed FORCE
	$(call cond_cmd,embed)

targets += $(obj)davix
pri_cat = CAT     $(target)
cmd_cat = cat $(filter-out FORCE,$^) > $@
$(obj)davix: $(obj)davix.intermediary $(obj)davix.embed FORCE
	$(call cond_cmd,cat)

all: $(obj)davix

else

kobjs += kernel/
kobjs += mm/
kobjs += util/

endif
