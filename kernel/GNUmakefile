# Subdirectory Makefile.
# Copyright (C) 2024  dbstream

kobjs += lock/
kobjs += sched/
kobjs += context.o
kobjs += exec.o
kobjs += exec_elf.o
kobjs += exit.o
kobjs += interrupt.o
kobjs += main.o
kobjs += panic.o
kobjs += printk.o
kobjs += smp.o
kobjs += task_creation.o

FLAGS-main.o := "-DKERNELVERSION=\"$(KERNELVERSION)\""
