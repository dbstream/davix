/* SPDX-License-Identifier: MIT */
#include <davix/smp.h>
#include <davix/sched.h>
#include <davix/printk.h>
#include <davix/list.h>

struct smpcall_list {
	struct list list;
	spinlock_t smpcall_lock;
};

struct smpcall_info {
	struct list list;
	void (*function)(void *);
	void *arg;
	atomic bool flag;
};

static struct smpcall_list smpcalls cpulocal;

void smp_call_on(struct logical_cpu *cpu, void (*function)(void *), void *arg)
{
	struct smpcall_list *list = cpulocal_address(cpu, &smpcalls);
	struct smpcall_info info = {
		.function = function,
		.arg = arg,
		.flag = 0
	};

	int flag = spin_acquire_irq(&list->smpcall_lock);
	list_add(&list->list, &info.list);
	spin_release_irq(&list->smpcall_lock, flag);

	arch_smp_notify(cpu);

	while(!atomic_load(&info.flag, memory_order_acquire))
		relax();
}

void smp_on_each_cpu(void (*function)(void *), void *arg)
{
	preempt_disable();
	struct logical_cpu *me = smp_self();

	for_each_logical_cpu(cpu) {
		if(cpu == me || !cpu->online)
			continue;
		smp_call_on(cpu, function, arg);
	}

	int flag = interrupts_enabled();
	disable_interrupts();

	function(arg);

	if(flag)
		enable_interrupts();
	preempt_enable();
}

void smp_notified(void)
{
	struct smpcall_list *list = cpulocal_address(smp_self(), &smpcalls);
	_spin_acquire(&list->smpcall_lock);
	struct smpcall_info *entry, *tmp;
	list_for_each_safe(entry, &list->list, list, tmp) {
		list_del(&entry->list);
		entry->function(entry->arg);
		atomic_store(&entry->flag, 1, memory_order_release);
	}
	_spin_release(&list->smpcall_lock);
}

/*
 * arch_init_smp(): architecture-specific function that initializes the
 * 'cpu_slots' array and the 'num_cpu_slots' constant.
 */
void arch_init_smp(void);

void init_smp(void)
{
	arch_init_smp();
	for_each_logical_cpu(cpu) {
		if(!cpu->possible)
			return;
		if(cpu->online) {
			info("CPU#%u online\n", cpu->id);
		} else {
			info("CPU#%u %s\n", cpu->id,
				cpu->present ? "present" : "hot-pluggable");
		}
		struct smpcall_list *cpu_smpcalls = cpulocal_address(cpu, &smpcalls);
		list_init(&cpu_smpcalls->list);
		spinlock_init(&cpu_smpcalls->smpcall_lock);
	}
}
