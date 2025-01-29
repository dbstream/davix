/**
 * Macros for defining system call handlers.
 * Copyright (C) 2025-present  dbstream
 */
#ifndef _DAVIX_SYSCALL_H
#define _DAVIX_SYSCALL_H 1

#include <davix/errno.h>
#include <asm/syscall.h>

#define syscall_return_void		__SYSCALL_RETURN_VOID(__syscall_regs)
#define syscall_return(value)		__SYSCALL_RETURN_SUCCESS(__syscall_regs, (value))
#define syscall_return_error(value)	__SYSCALL_RETURN_ERROR(__syscall_regs, (value))

#define SYSCALL_PROTO0(type, name)					\
static int								\
name (syscall_regs_t __syscall_regs)

#define SYSCALL_PROTO1(type, name, type1, arg1)				\
static int								\
name (type1 arg1, syscall_regs_t __syscall_regs)

#define SYSCALL_PROTO2(type, name, type1, arg1, type2, arg2)		\
static int								\
name (type1 arg1, type2 arg2, syscall_regs_t __syscall_regs)

#define SYSCALL_PROTO3(type, name, type1, arg1, type2, arg2, type3, arg3)		\
static int								\
name (type1 arg1, type2 arg2, type3 arg3, syscall_regs_t __syscall_regs)

#define SYSCALL_PROTO4(type, name, type1, arg1, type2, arg2, type3, arg3, type4, arg4)		\
static int								\
name (type1 arg1, type2 arg2, type3 arg3, type4 arg4, syscall_regs_t __syscall_regs)

#define SYSCALL_PROTO5(type, name, type1, arg1, type2, arg2, type3, arg3, type4, arg4, type5, arg5)		\
static int								\
name (type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, syscall_regs_t __syscall_regs)

#define SYSCALL_PROTO6(type, name, type1, arg1, type2, arg2, type3, arg3, type4, arg4, type5, arg5, type6, arg6)		\
static int								\
name (type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6, syscall_regs_t __syscall_regs)

#define SYSCALL0(type, name)						\
SYSCALL_PROTO0(type, __sys_##name);					\
int									\
__wrap_sys_##name (syscall_regs_t __syscall_regs)			\
{									\
	return __sys_##name (__syscall_regs);				\
}									\
SYSCALL_PROTO0(type, __sys_##name)

#define SYSCALL1(type, name, type1, arg1)				\
SYSCALL_PROTO1(type, __sys_##name, type1, arg1);			\
int									\
__wrap_sys_##name (syscall_regs_t __syscall_regs)			\
{									\
	return __sys_##name (	(type1) SYSCALL_ARG1(__syscall_regs),	\
			__syscall_regs);				\
}									\
SYSCALL_PROTO1(type, __sys_##name, type1, arg1)

#define SYSCALL2(type, name, type1, arg1, type2, arg2)			\
SYSCALL_PROTO2(type, __sys_##name, type1, arg1, type2, arg2);		\
int									\
__wrap_sys_##name (syscall_regs_t __syscall_regs)			\
{									\
	return __sys_##name (	(type1) SYSCALL_ARG1(__syscall_regs),	\
				(type2) SYSCALL_ARG2(__syscall_regs),	\
			__syscall_regs);				\
}									\
SYSCALL_PROTO2(type, __sys_##name, type1, arg1, type2, arg2)

#define SYSCALL3(type, name, type1, arg1, type2, arg2, type3, arg3);	\
SYSCALL_PROTO3(type, __sys_##name, type1, arg1, type2, arg2, type3, arg3);	\
int									\
__wrap_sys_##name (syscall_regs_t __syscall_regs)			\
{									\
	return __sys_##name (	(type1) SYSCALL_ARG1(__syscall_regs),	\
				(type2) SYSCALL_ARG2(__syscall_regs),	\
				(type3) SYSCALL_ARG3(__syscall_regs),	\
			__syscall_regs);				\
}									\
SYSCALL_PROTO3(type, __sys_##name, type1, arg1, type2, arg2, type3, arg3)

#define SYSCALL4(type, name, type1, arg1, type2, arg2, type3, arg3, type4, arg4);	\
SYSCALL_PROTO4(type, __sys_##name, type1, arg1, type2, arg2, type3, arg3, type4, arg4);	\
int									\
__wrap_sys_##name (syscall_regs_t __syscall_regs)			\
{									\
	return __sys_##name (	(type1) SYSCALL_ARG1(__syscall_regs),	\
				(type2) SYSCALL_ARG2(__syscall_regs),	\
				(type3) SYSCALL_ARG3(__syscall_regs),	\
				(type4) SYSCALL_ARG4(__syscall_regs),	\
			__syscall_regs);				\
}									\
SYSCALL_PROTO4(type, __sys_##name, type1, arg1, type2, arg2, type3, arg3, type4, arg4)

#define SYSCALL5(type, name, type1, arg1, type2, arg2, type3, arg3, type4, arg4, type5, arg5);	\
SYSCALL_PROTO5(type, __sys_##name, type1, arg1, type2, arg2, type3, arg3, type4, arg4, type5, arg5);	\
int									\
__wrap_sys_##name (syscall_regs_t __syscall_regs)			\
{									\
	return __sys_##name (	(type1) SYSCALL_ARG1(__syscall_regs),	\
				(type2) SYSCALL_ARG2(__syscall_regs),	\
				(type3) SYSCALL_ARG3(__syscall_regs),	\
				(type4) SYSCALL_ARG4(__syscall_regs),	\
				(type5) SYSCALL_ARG5(__syscall_regs),	\
			__syscall_regs);				\
}									\
SYSCALL_PROTO5(type, __sys_##name, type1, arg1, type2, arg2, type3, arg3, type4, arg4, type5, arg5)

#define SYSCALL6(type, name, type1, arg1, type2, arg2, type3, arg3, type4, arg4, type5, arg5, type6, arg6);	\
SYSCALL_PROTO6(type, __sys_##name, type1, arg1, type2, arg2, type3, arg3, type4, arg4, type5, arg5, type6, arg6);	\
int									\
__wrap_sys_##name (syscall_regs_t __syscall_regs)			\
{									\
	return __sys_##name (	(type1) SYSCALL_ARG1(__syscall_regs),	\
				(type2) SYSCALL_ARG2(__syscall_regs),	\
				(type3) SYSCALL_ARG3(__syscall_regs),	\
				(type4) SYSCALL_ARG4(__syscall_regs),	\
				(type5) SYSCALL_ARG5(__syscall_regs),	\
				(type6) SYSCALL_ARG6(__syscall_regs),	\
			__syscall_regs);				\
}									\
SYSCALL_PROTO6(type, __sys_##name, type1, arg1, type2, arg2, type3, arg3, type4, arg4, type5, arg5, type6, arg6)

extern void
trace_syscall_enter (syscall_regs_t regs);

extern void
trace_syscall_exit (syscall_regs_t regs, int is_error);

#endif /**  _DAVIX_SYSCALL_H   */
