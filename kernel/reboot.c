/**
 * SYS_reboot syscall.
 * Copyright (C) 2025-present  dbstream
 */
#include <davix/printk.h>
#include <davix/stdbool.h>
#include <davix/syscall.h>
#include <davix/uapi/reboot.h>
#include <davix/spinlock.h>
#include <asm/irq.h>

#if CONFIG_UACPI
#include <uacpi/sleep.h>
#include <uacpi/uacpi.h>
#endif

static inline bool
acpi_shutdown_supported (void)
{
#if CONFIG_UACPI
	uacpi_init_level level = uacpi_get_current_init_level ();
	return level == UACPI_INIT_LEVEL_NAMESPACE_INITIALIZED;
#else
	return false;
#endif
}

#if CONFIG_UACPI
static errno_t
acpi_enter_sleep_state (uacpi_sleep_state state)
{
	uacpi_status status = uacpi_prepare_for_sleep_state (state);
	if (status != UACPI_STATUS_OK) {
		printk (PR_ERR "uacpi_prepare_for_sleep_state(S%d) returned %s\n",
				state, uacpi_status_to_string (status));
		return EIO;
	}

	irq_disable ();
	status = uacpi_enter_sleep_state (state);
	irq_enable ();
	printk (PR_ERR "uacpi_enter_sleep_state(S%d) returned %s\n",
			state, uacpi_status_to_string (status));

	return status == UACPI_STATUS_OK ? ESUCCESS : EIO;
}
#endif

static errno_t
do_acpi_shutdown (void)
{
	errno_t e = ENOTSUP;

#if CONFIG_UACPI
	printk (PR_NOTICE "Shutting down the system...\n");
	preempt_off ();
	e = acpi_enter_sleep_state (UACPI_SLEEP_STATE_S5);
	preempt_on ();
#endif

	return e;
}

static errno_t
do_query_support (int arg)
{
	switch (arg) {
	case REBOOT_CMD_QUERY_SUPPORT:
		return ESUCCESS;
	case REBOOT_CMD_POWEROFF:
		if (!acpi_shutdown_supported ())
			return ENOTSUP;
		return ESUCCESS;
	default:
		return ENOTSUP;
	}
}

static spinlock_t shutdown_lock;

static errno_t
do_poweroff (int arg)
{
	if (arg)
		return ENOTSUP;

	/* TODO: stop other CPUs from executing work  */
	/* actually, that might be considered a userspace responsibility  */

	if (!acpi_shutdown_supported ())
		return ENOTSUP;

	if (!spin_trylock (&shutdown_lock))
		return EAGAIN;

	errno_t e = do_acpi_shutdown ();

	__spin_unlock (&shutdown_lock);
	return e;
}

SYSCALL4 (void, reboot, int, cmd, int, arg,
		unsigned int, mag0, unsigned int, mag1)
{
	/** TODO:  check that the task is running as the superuser.  */

	if (mag0 != DAVIX_REBOOT_MAG0 || mag1 != DAVIX_REBOOT_MAG1)
		syscall_return_error (EINVAL);

	errno_t e = EINVAL;
	switch (cmd) {
	case REBOOT_CMD_QUERY_SUPPORT:
		e = do_query_support (arg);
		break;
	case REBOOT_CMD_POWEROFF:
		e = do_poweroff (arg);
		break;
	}

	if (e != ESUCCESS)
		syscall_return_error (e);
	syscall_return_void;
}
