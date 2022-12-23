# SPDX-License-Identifier: MIT
# Top-level Makefile for the kernel

MAKEFLAGS += -Rrj

PHONY := __all
__all: all

ifndef objtree
objtree := build
PHONY += clean
clean:
	@rm -r build
else
PHONY += clean
clean:
	$(error Cannot define a 'clean' target when building out-of-tree.)
endif

PHONY += htmldocs
htmldocs:
	@echo -e SPHINX-BUILD\\thtmldocs
	@sphinx-build -b html -a doc $(objtree)/htmldocs

PHONY += pdfdocs
pdfdocs:
	@echo -e SPHINX-BUILD\\tpdfdocs
	@sphinx-build -b latex -a doc $(objtree)/pdfdocs
	@$(MAKE) -C $(objtree)/pdfdocs all-pdf

ifndef ARCH
ARCH := x86
endif

__obj :=
dep :=

obj:=
include kernel/Makefile
__obj += $(patsubst %.o,$(objtree)/kernel/%.o,$(obj))

# Keep this the last thing that is included! (__obj needs to be complete)
include arch/$(ARCH)/Makefile

.PHONY: $(PHONY)

dep += $(patsubst %.o,%.o.d,$(__obj))
-include $(dep)
