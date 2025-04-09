# Buildsystem for the Davix kernel.
# Copyright (C) 2025-present  dbstream
#
# This file is included by build.GNUmakefile.  It defines the rules used for
# compilation and linking of kernel sources.

cflags = $(CPPFLAGS) $(COMMONFLAGS) $(CFLAGS) $(CFLAGS-$(reltarget))
cxxflags = $(CPPFLAGS) $(COMMONFLAGS) $(CXXFLAGS) $(CXXFLAGS-$(reltarget))
aflags = $(CPPFLAGS) $(AFLAGS) $(AFLAGS-$(reltarget))
ldflags = $(LDFLAGS) $(LDFLAGS-$(reltarget))
ldlibs = $(LDLIBS) $(LDLIBS-$(reltarget))
objcopyflags = $(OBJCOPYFLAGS) $(OBJCOPYFLAGS-$(reltarget))

# Invoke the archiver.
cmd_ar = if [ -f $@ ]; then rm $@; fi; ar cDPr --thin $@ $(filter-out FORCE,$^)

# Invoke objcopy.
pri_objcopy = OBJCOPY $(target)
cmd_objcopy = $(OBJCOPY) $(objcopyflags) $< $@

# Invoke the linker.
pri_ld = LD      $(target)
cmd_ld = $(LD) -o $@ $(ldflags) $(filter-out FORCE,$^) $(ldlibs)

# Invoke the C preprocessor.
pri_cpp = CPP     $(target)
cmd_cpp = $(CC) -E -o $@ $(CPPFLAGS) $< -MD -MP -MF $@.d

# Invoke the assembler.
pri_as = AS      $(target)
cmd_as = $(AS) -c -o $@ $(aflags) $< -MD -MP -MF $@.d

# Invoke the C compiler.
pri_cc = CC      $(target)
cmd_cc = $(CC) -c -o $@ $(cflags) $< -MD -MP -MF $@.d

# Invoke the C++ compiler.
pri_cxx = CXX     $(target)
cmd_cxx = $(CXX) -c -o $@ $(cxxflags) $< -MD -MP -MF $@.d

$(obj)%.o: $(src)%.S FORCE
	$(call cond_cmd,as)

$(obj)%.o: $(src)%.c FORCE
	$(call cond_cmd,cc)

$(obj)%.o: $(src)%.cc FORCE
	$(call cond_cmd,cxx)

# If $(kobjs) is not empty, declare the $(obj)kernel.a target.
ifneq ($(kobjs),)
kobjs := $(addprefix $(obj),$(sort $(patsubst %/,%/kernel.a,$(kobjs))))
targets += $(obj)kernel.a $(kobjs)
depfiles += $(addsuffix .d,$(kobjs))

$(obj)kernel.a: $(kobjs) FORCE
	$(call cond_cmd,ar)

$(obj)%/kernel.a: FORCE
	@mkdir -p -- $(obj)$*
	@$(MAKE) -f $(BUILD_MAKEFILE) subdir=$(subdir)$*/ $@
endif
