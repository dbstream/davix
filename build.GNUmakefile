# Kernel build Makefile
# Copyright (C) 2024  dbstream

BUILD_MAKEFILE := $(lastword $(MAKEFILE_LIST))

.PHONY: all
all:

ifeq ($(objtree),)
$(error Cannot invoke tools/build.GNUmakefile directly.)
endif

# Setup the following state:
#	- $(obj)	Directory we emit to
#	- $(src)	Source directory
obj := $(objtree)/$(subdir)
src := $(subdir)

# We need to be able to rebuild things when the command for building
# them changes. This is not something that Make does by default; we
# will have to implement it ourselves. The following thirty or so
# lines are a very hacky implementation of this.
#
# Suggested usage:
#	pri_foo = FOO     $@
#	cmd_foo = foo -o $@ $<
#	$(obj)%.bar: $(src)%.baz FORCE
#		$(call cond_cmd,foo)

dquote := "
quote := '
pound := \#
esc_cmd_1 = $(subst $$,$$$$,$(cmd_$1))
esc_cmd_2 = $(subst $(pound),$$(pound),$(esc_cmd_1))
esc_cmd   = $(subst $(quote),\$(quote),$(esc_cmd_2))

empty :=
space := $(empty) $(empty)
esc_spc = $(subst $(space),$$(space),$(strip $1))
nequal = $(filter-out $(call esc_spc,$1),$(call esc_spc,$2))

new_cmd		= $(call nequal,$(cmd_$@),$(esc_cmd))
new_prereqs	= $(filter-out FORCE,$?)
not_in_targets	= $(if $(filter $@,$(targets)),,$(warning $@: not in targets))
not_forced	= $(if $(filter FORCE,$^),,$(warning $@: no FORCE dependency))

need_remake = $(new_cmd)$(new_prereqs)$(not_in_targets)$(not_forced)
ifeq ("$(origin R)","command line")
ifneq ($(filter-out 0 n N,$(R)),)
need_remake = 1
endif
endif

pri = $(if $(pri_$1),echo '  $(pri_$1)';)
ifeq ("$(origin V)","command line")
ifneq ($(filter-out 0 n N,$(V)),)
pri = echo '$(subst $(quote),$$(quote),$(cmd_$1))';
endif
endif

runcmd = $(pri)$(cmd_$1);printf '%s\n' 'cmd_$@ := $(esc_cmd)' > $@.makecmd
cond_cmd = $(if $(need_remake),@set -e;$(runcmd),@:)

# We use the following Makefile variables to keep track of several things:
#	- $(targets)		All subdirectory targets that can be built.
#	- $(depfiles)		All '.d' files that we should include.
#	- $(kobjs)		Objects for 'kernel.a'.
targets :=
depfiles :=
kobjs :=

# $@ without the objtree prefix.
target = $(patsubst $(objtree)/%,%,$@)
reltarget = $(patsubst $(subdir)%,%,$(target))

# Commonly used rules and variables. 
cflags = $(CPPFLAGS) $(CFLAGS) $(FLAGS-$(reltarget))
aflags = $(CPPFLAGS) $(AFLAGS) $(FLAGS-$(reltarget))
ldflags =
ldlibs =

# Invoke ar.
cmd_ar = if [ -f $@ ]; then rm $@; fi; ar cDPrT $@ $(filter-out FORCE,$^)

# Call the linker.
pri_ld = LD      $(target)
cmd_ld = $(LD) -o $@ $(ldflags) $(filter-out FORCE,$^) $(ldlibs)

# Call the C preprocessor.
pri_cpp = CPP     $(target)
cmd_cpp = $(CC) -E -o $@ $(CPPFLAGS) $< -MD -MP -MF $@.d

# Call the assembler.
pri_as = AS      $(target)
cmd_as = $(AS) -c -o $@ $(aflags) $< -MD -MP -MF $@.d
$(obj)%.o: $(src)%.S FORCE
	$(call cond_cmd,as)

# Call the C compiler.
pri_cc = CC      $(target)
cmd_cc = $(CC) -c -o $@ $(cflags) $< -MD -MP -MF $@.d
$(obj)%.o: $(src)%.c FORCE
	$(call cond_cmd,cc)

# Include the subdirectory Makefile.
include $(subdir)GNUmakefile

# Declare a target, kernel.a, when $(kobjs)) is not empty.
ifneq ($(kobjs),)

$(obj)%/kernel.a: FORCE
	$(Q)mkdir -p -- $(obj)$*
	$(Q)$(MAKE) -f $(BUILD_MAKEFILE) subdir=$(subdir)$*/ $(obj)$*/kernel.a

kobjs := $(addprefix $(obj),$(sort $(patsubst %/,%/kernel.a,$(kobjs))))

targets += $(obj)kernel.a $(kobjs)
depfiles += $(addsuffix .d,$(kobjs))
$(obj)kernel.a: $(kobjs) FORCE
	$(call cond_cmd,ar)

endif

# Include the .makecmd files.
-include $(addsuffix .makecmd,$(sort $(targets)))

# Include dependency files.
-include $(sort $(depfiles))

.PHONY: FORCE
FORCE:

.SECONDARY:
