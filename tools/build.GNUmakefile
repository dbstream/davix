# Buildsystem for the Davix kernel.
# Copyright (C) 2025-present  dbstream

BUILD_MAKEFILE := $(lastword $(MAKEFILE_LIST))

.PHONY: all
all:

.PHONY: FORCE
FORCE:

.SECONDARY:

ifeq ($(objtree),)
$(error build.GNUmakefile was invoked without an objtree.)
endif

src := $(subdir)
obj := $(objtree)/$(subdir)

targets :=
depfiles :=
kobjs :=
kobjs-y :=

target = $(patsubst $(objtree)/%,%,$@)
reltarget = $(patsubst $(obj)%,%,$@)

include $(subdir)Sources

kobjs += $(kobjs-y)

dquote := "
squote := '
pound := \#
empty :=
space := $(empty) $(empty)

escape_spaces = $(subst $(space),$$(space),$(strip $1))
not_equal = $(filter-out $(call escape_spaces,$1),$(call escape_spaces,$2))

escaped_command = $(subst $(squote),\$(squote),$(subst $(pound),$$(pound),$(subst $$,$$$$,$(cmd_$1))))
cmd_differs = $(if $(call not_equal,$(cmd_$@),$(escaped_command)), cmd_differs,)
new_prereqs = $(if $(filter-out FORCE,$?), new_prereqs)
warn_not_in_targets = $(if $(filter $@,$(targets)),, not_in_targets$(warning $@: not in targets))
warn_not_forced = $(if $(filter FORCE,$^),, not_forced$(warning $@: no dependency on FORCE))

need_remake = $(cmd_differs)$(new_prereqs)$(warn_not_in_targets)$(warn_not_forced)

# Rebuild behavior: always rebuild everything if make R=... (not 0, n, or N) is specified.
ifeq ("$(origin R)","command line")
ifneq ($(filter-out 0 n N,$(R)),)
need_remake = $(empty) R=1
endif
endif

# Verbosity: print commands if make V=... (not 0, n, or N) is specified.
# Additionally, if make V=2 is specified, print the reason for rebuilding a
# particular target.
print_command = $(if $(pri_$1),echo '  $(pri_$1)';)
ifeq ("$(origin V)","command line")
ifneq ($(filter-out 0 n N,$(V)),)
print_command = echo '$(subst $(squote),$$(squote),$(cmd_$1))';
endif
ifeq ($(V),2)
print_command = echo 'Rebuilding $@ because of:$(need_remake).';echo '$(subst $(squote),$$(squote),$(cmd_$1))';
endif
endif

run_command = $(cmd_$1);printf '%s\n' 'cmd_$@ := $(escaped_command)' > $@.cmd
cond_cmd = $(if $(need_remake),@set -e;$(print_command)$(run_command),@:)

include tools/rules.GNUmakefile

-include $(addsuffix .cmd,$(sort $(targets)))
-include $(sort $(depfiles))
