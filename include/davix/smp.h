/* SPDX-License-Identifier: MIT */
#ifndef __DAVIX_SMP_H
#define __DAVIX_SMP_H

#include <davix/types.h>
#include <asm/smp.h>

struct logical_cpu {
	unsigned long cpulocal_offset;
	unsigned id;
	bool online;
	bool possible;
	bool present;
};

/*
 * An array of logical CPUs, allocated by init_smp().
 */
extern struct logical_cpu *cpu_slots;

/*
 * This is the maximum number of logical CPUs, and the length of the above
 * array, determined at init_smp()-time.
 */
extern unsigned num_cpu_slots;

/*
 * Helpful macro.
 */
#define for_each_logical_cpu(cpu) \
	for(struct logical_cpu *cpu = &cpu_slots[0]; \
		cpu < &cpu_slots[num_cpu_slots]; cpu++)

void init_smp(void);

void smp_boot_cpu(struct logical_cpu *cpu);

#define cpulocal_address(cpu, var) \
	((force typeof(var))((force unsigned long) (var) + (cpu)->cpulocal_offset))

#define rdwr_cpulocal_on(cpu, var) (*(cpulocal_address((cpu), &(var))))

#define rdwr_cpulocal(var) rdwr_cpulocal_on(smp_self(), (var))

/*
 * Nicely ask a CPU to deal with interprocessor function calls. This should
 * cause the other CPU to enter an interrupt context and call smp_notified(),
 * defined below.
 */
void arch_smp_notify(struct logical_cpu *cpu);

/*
 * Callback for 'arch_smp_notify()' on the other CPU.
 */
void smp_notified(void);

/*
 * Call a function on another CPU.
 *
 * The function will be called in an interrupt-disabled context. This function
 * will wait for the function call to return.
 *
 * This function mustn't be called from an interrupt-disabled context, as that
 * would lead to a race condition when two CPUs try to 'smp_call_on()' each
 * other.
 */
void smp_call_on(struct logical_cpu *cpu, void (*function)(void *), void *arg);

/*
 * Call a function on every onlined CPU of the system. The same rules as for
 * 'smp_call_on()' apply.
 */
void smp_on_each_cpu(void (*function)(void *), void *arg);

/*
 * Send a NMI to another CPU.
 */
void smp_send_nmi(struct logical_cpu *cpu);

#endif /* __DAVIX_SMP_H */
