# Subdirectory Makefile
# Copyright (C) 2025-present  dbstream

# FIXME: don't hardcode HOSTCXX and the build rules here.
HOSTCXX := g++

hostcxxflags =
hostcxxlibs =
pri_hostcxx = HOSTCXX $(target)
cmd_hostcxx = $(HOSTCXX) -o $@ $(hostcxxflags) $< $(hostcxxlibs)

targets += $(obj)gen_embed
$(obj)gen_embed: hostcxxlibs = -lelf
$(obj)gen_embed: $(src)gen_embed.cc FORCE
	$(call cond_cmd,hostcxx)
