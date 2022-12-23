# SPDX-License-Identifier: MIT
# x86 Makefile for the loader

loader_obj += header.S.o
loader_obj += main.c.o
loader_obj += memalloc.c.o
loader_obj += memmap.c.o
loader_obj += uart.c.o
loader_obj += stdlib.c.o

__loader_obj += $(objtree)/arch/x86/loader/include_kernel.S.o
$(objtree)/arch/x86/loader/include_kernel.S.o: arch/x86/loader/include_kernel.S.in $(objtree)/mainkernel
	@mkdir -p $(dir $@)
	@echo -e M4\\tarch/x86/loader/include_kernel.S.in
	@m4 -DOBJTREE=$(objtree) arch/x86/loader/include_kernel.S.in > $(objtree)/arch/x86/loader/include_kernel.S
	@echo -e AS\\t$(objtree)/arch/x86/loader/include_kernel.S
	@$(CC) $(ASFLAGS) -c -o $@ $(objtree)/arch/x86/loader/include_kernel.S
