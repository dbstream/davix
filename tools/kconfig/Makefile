# SPDX-License-Identifier: GPL-2.0
# This is a modified Linux Makefile that compiles (some of) the Kconfig
# utilities for use with non-Linux software.
# ===========================================================================

# object files used by all kconfig flavours
common-objs	:= confdata.o expr.o lexer.lex.o menu.o parser.tab.o \
		   preprocess.o symbol.o util.o

$(objtree)/tools/kconfig/lexer.lex.o: $(objtree)/tools/kconfig/parser.tab.h
HOSTCFLAGS_lexer.lex.o := -I $(srctree)/tools/kconfig
HOSTCFLAGS_parser.tab.o := -I $(srctree)/tools/kconfig

# conf: Used for defconfig, oldconfig and related targets
hostprogs	+= conf
conf-objs	:= conf.o $(common-objs)

# nconf: Used for the nconfig target based on ncurses
hostprogs	+= nconf
nconf-objs	:= nconf.o nconf.gui.o $(common-objs)

HOSTLDLIBS_nconf	= -lncurses -lmenu -lpanel

$(objtree)/tools/kconfig/conf: $(addprefix $(objtree)/tools/kconfig/,$(conf-objs))
	@mkdir -p $(dir $@)
	$(Q)gcc -o $@ $^

$(objtree)/tools/kconfig/nconf: $(addprefix $(objtree)/tools/kconfig/,$(nconf-objs))
	@mkdir -p $(dir $@)
	$(Q)gcc -o $@ $^ $(HOSTLDLIBS_nconf)

$(objtree)/tools/kconfig/%.o: $(srctree)/tools/kconfig/%.c
	@mkdir -p $(dir $@)
	$(Q)gcc -c -o $@ $<

$(objtree)/tools/kconfig/%.o: $(objtree)/tools/kconfig/%.c
	$(Q)gcc -c -o $@ $< -I$(srctree)/tools/kconfig

$(objtree)/tools/kconfig/%.lex.c: $(srctree)/tools/kconfig/%.l
	@mkdir -p $(dir $@)
	$(Q)flex -o $@ -L $<

$(objtree)/tools/kconfig/parser.tab.c $(objtree)/tools/kconfig/parser.tab.h: \
	$(srctree)/tools/kconfig/parser.y
	@mkdir -p $(objtree)/tools/kconfig
	$(Q)yacc -Wno-yacc -o $(objtree)/tools/kconfig/parser.tab.c \
		--defines=$(objtree)/tools/kconfig/parser.tab.h -t -l $<
