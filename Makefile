# SPDX-License-Identifier: MIT
MAKEFLAGS += -Rr --no-print-directory

export LC_ALL = C.UTF-8

.PHONY: __all
__all: all

.PHONY: help
help:
	@echo 'To configure the kernel, run ``make nconfig``.'
	@echo 'To build the kernel, run ``make``.'
	@echo 'To build the kernel out-of-tree, run ``make O=...``.'
	@echo 'To remove the in-tree build directory, run ``make clean``.'
	@echo 'To print the commands that are executed, run ``make V=1``.'

this-makefile := $(realpath $(lastword $(MAKEFILE_LIST)))
srctree := $(realpath $(dir $(this-makefile)))
workdir := $(realpath $(CURDIR))

ifeq ($(srctree),$(workdir))
current-subdir :=
else
current-subdir := $(workdir:$(srctree)/%=%)/
endif

ifndef objtree
# By specifying O=..., the user can choose an alterntive output directory.
ifeq ("$(origin O)", "command line")
objtree := $(realpath $(O))
else
objtree := $(srctree)/build
endif
endif

export srctree
export objtree
export workdir

# By specifying V=1, the user indicates that it wants make to print all
# commands that get executed.
ifeq ("$(origin V)", "command line")
ifeq ($(V),0)
Q := @
else
Q :=
endif
else
Q := @
endif

export V Q

ifndef ARCH
ARCH := $(shell uname -m)
endif
ifeq ($(ARCH),x86_64)
ARCH := x86
endif
export ARCH

ifneq ($(filter %config,$(MAKECMDGOALS)),)
config-build := 1
ifneq ($(filter-out %config,$(MAKECMDGOALS)),)
mixed-build := 1
endif
endif

ifdef mixed-build

ifneq ($(srctree),$(workdir))
$(error Cannot invoke configuration targets from a subdirectory.)
endif

.PHONY: $(MAKECMDGOALS)
$(MAKECMDGOALS):
	$(Q)set -e; \
	for goal in $(MAKECMDGOALS); do \
		$(MAKE) -f $(this-makefile) $$goal; \
	done
else # mixed-build

ifdef config-build

ifneq ($(srctree),$(workdir))
$(error Cannot invoke configuration targets from a subdirectory.)
endif

$(objtree)/tools/kconfig/nconf:
	$(Q)$(MAKE) -f $(srctree)/tools/kconfig/Makefile $@

.PHONY: nconfig
nconfig: $(objtree)/tools/kconfig/nconf
	$(Q)$< $(srctree)/Kconfig

else # config-build

ifndef has_done_syncconfig

has_done_syncconfig := 1
export has_done_syncconfig

$(objtree)/tools/kconfig/conf:
	$(Q)$(MAKE) -f $(srctree)/tools/kconfig/Makefile $@

$(srctree)/include/generated/autoconf.h $(srctree)/include/config/auto.conf: $(srctree)/.config $(objtree)/tools/kconfig/conf
	$(Q)$(objtree)/tools/kconfig/conf --syncconfig Kconfig

.PHONY: all $(MAKECMDGOALS)
all $(MAKECMDGOALS): $(srctree)/include/generated/autoconf.h $(srctree)/include/config/auto.conf
	$(Q)$(MAKE) -f $(this-makefile) $(MAKECMDGOALS)

else # !has_done_syncconfig

include $(srctree)/include/config/auto.conf

.PHONY: clean
ifeq ($(objtree),$(srctree)/build)
clean:
	$(Q)rm -r $(srctree)/build
else
clean:
	$(error ``clean`` is not defined for out-of-tree kernel builds.)
endif

DAVIXINCLUDE := \
	-I$(srctree)/include \
	-I$(srctree)/arch/$(ARCH)/include \
	-include $(srctree)/include/generated/autoconf.h

CC := $(CONFIG_CROSS_COMPILE)gcc
AR := $(CONFIG_CROSS_COMPILE)ar

CFLAGS := \
	-std=gnu17 \
	-nostdinc \
	$(DAVIXINCLUDE) \
	-Wall \
	-Werror \
	-O2 -g \
	-D__KERNEL__ \
	-ffreestanding

AFLAGS := $(DAVIXINCLUDE) -D__KERNEL__
LDFLAGS :=

extra-targets-y :=

include $(srctree)/arch/$(ARCH)/toolchain.include

export CC CFLAGS AFLAGS

obj-y :=

ifeq ($(srctree),$(workdir))

obj-y := arch/$(ARCH)/built-in.a kernel/built-in.a mm/built-in.a

all: $(objtree)/davix-kernel $(extra-targets-y)

$(objtree)/davix-kernel: $(objtree)/built-in.a
	@echo -e LINK\\t$(notdir $@)
	$(Q)$(CC) $(LDFLAGS) -o $@ $<

else

all: $(objtree)/$(current-subdir)built-in.a

include $(srctree)/$(current-subdir)Makefile

endif

real-obj-y := $(addprefix $(objtree)/$(current-subdir),$(obj-y))
built-ins := $(filter $(objtree)/%/built-in.a,$(real-obj-y))

$(objtree)/%.c.o: $(srctree)/%.c
	@mkdir -p $(dir $@)
	@echo -e CC\\t$(current-subdir)$(notdir $<)
	$(Q)$(CC) $(CFLAGS) $(CFLAGS-$(notdir $@)) -c -o $@ $< -MD -MP -MF $@.d

$(objtree)/%.S.o: $(srctree)/%.S
	@mkdir -p $(dir $@)
	@echo -e AS\\t$(current-subdir)$(notdir $<)
	$(Q)$(CC) $(AFLAGS) $(AFLAGS-$(notdir $@)) -c -o $@ $< -MD -MP -MF $@.d

$(objtree)/$(current-subdir)built-in.a: $(real-obj-y)
	@echo -e AR\\t$(current-subdir)built-in.a
	$(Q)$(AR) cDPrT $@ $^

.PHONY: $(built-ins)
$(built-ins): $(objtree)/%/built-in.a: $(srctree)/%
	$(Q)$(MAKE) -C $< -f $(this-makefile)

-include $(filter %.d,$(patsubst %.o, %.o.d, $(real-obj-y)))

endif # has_done_syncconfig

endif # !config-build
endif # !mixed-build
