/* SPDX-License-Identifier: MIT */
#ifndef __ASM_IO_H
#define __ASM_IO_H

#include <davix/types.h>

static inline void io_outb(u16 port, u8 data)
{
        asm volatile("outb %0, %1" : : "a"(data), "Nd"(port) : "memory");
}

static inline void io_outw(u16 port, u16 data)
{
        asm volatile("outw %0, %1" : : "a"(data), "Nd"(port) : "memory");
}

static inline void io_outl(u16 port, u32 data)
{
        asm volatile("outl %0, %1" : : "a"(data), "Nd"(port) : "memory");
}

static inline u8 io_inb(u16 port)
{
        u8 r;
        asm volatile("inb %1, %0" : "=a"(r) : "Nd"(port) : "memory");
        return r;
}

static inline u16 io_inw(u16 port)
{
        u16 r;
        asm volatile("inw %1, %0" : "=a"(r) : "Nd"(port) : "memory");
        return r;
}

static inline u32 io_inl(u16 port)
{
        u32 r;
        asm volatile("inl %1, %0" : "=a"(r) : "Nd"(port) : "memory");
        return r;
}

static inline void io_wait(void)
{
        io_outb(0x80, 0);
}

#endif /* __ASM_IO_H */
