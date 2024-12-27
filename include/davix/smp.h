/**
 * Support for Simultaneous Multiprocessing (SMP).
 * Copyright (C) 2024  dbstream
 */
#ifndef _DAVIX_SMP_H
#define _DAVIX_SMP_H 1

extern void
smp_init (void);

extern void
smpboot_init (void);

/**
 * Bring other CPUs online.
 */
extern void
smp_boot_cpus (void);

/**
 * Architectures call this function on freshly-booted CPUs after initializing
 * architecture-specific state and marking the CPU as online.
 */
extern void
smp_start_additional_cpu (void);

/**
 * Called by architecture-specific code on target CPUs of the call_on_cpu IPI.
 */
extern void
smp_do_call_on_cpu_work (void);

struct smp_call_on_cpu_work {
	struct smp_call_on_cpu_work *next;
	void (*func) (void *);
	void *arg;
	int done;
};

/**
 * Perform @work on @cpu. This function returns before @work has finished;
 * callers must wait on @work using smp_wait_for_call_on_cpu.
 */
extern void
smp_call_on_cpu_async (unsigned int cpu, struct smp_call_on_cpu_work *work);

/**
 * Wait for the completion of @work.
 */
extern void
smp_wait_for_call_on_cpu (struct smp_call_on_cpu_work *work);

/**
 * Call @func with @arg on @cpu, and wait for it to finish execution.
 */
static inline void
smp_call_on_cpu (unsigned int cpu, void (*func) (void *), void *arg)
{
	struct smp_call_on_cpu_work work = {
		.func = func,
		.arg = arg
	};

	smp_call_on_cpu_async (cpu, &work);
	smp_wait_for_call_on_cpu (&work);
}

#endif /* _DAVIX_SMP_H */
