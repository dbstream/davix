/* SPDX-License-Identifier: MIT */
#ifndef __PRINTK_H
#define __PRINTK_H

struct boot_struct;

void uart_init(void);
void uart_fillin_boot_struct(struct boot_struct *boot_struct);

void printk(char loglevel, const char *fmt, ...);

#define KERN_DEBUG '0'
#define KERN_INFO '1'
#define KERN_WARN '2'
#define KERN_ERROR '3'
#define KERN_CRITICAL '4'

#define debug(...) printk(KERN_DEBUG, __VA_ARGS__)
#define info(...) printk(KERN_INFO, __VA_ARGS__)
#define warn(...) printk(KERN_WARN, __VA_ARGS__)
#define error(...) printk(KERN_ERROR, __VA_ARGS__)
#define critical(...) printk(KERN_CRITICAL, __VA_ARGS__)

#endif /* __PRINTK_H */
