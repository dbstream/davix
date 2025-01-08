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
	--eh-frame-hdr		\
	-T

LDLIBS-davix :=

targets += $(obj)kernel.lds
LDSCRIPT-davix := $(obj)kernel.lds
$(obj)kernel.lds: $(src)arch/$(ARCH)/kernel.lds.S FORCE
	$(call cond_cmd,cpp)

targets += $(obj)davix
pri_objcopy = OBJCOPY $(target)
cmd_objcopy = $(OBJCOPY) -O binary $< $@
$(obj)davix: $(obj)davix.elf FORCE
	$(call cond_cmd,objcopy)

all: $(obj)davix

else

kobjs += kernel/
kobjs += mm/
kobjs += util/

endif
