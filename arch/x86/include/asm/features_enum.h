/**
 * CPU features enumeration.
 * Copyright (C) 2024  dbstream
 *
 * This file defines the CPU feature bits.
 */
#ifndef _FEATURES_ENUM_H
#define _FEATURES_ENUM_H 1

/* Feature word 0: CPUID 01h ecx */
#define FEATURE_X2APIC		(32 * 0		+ 1)

/* Feature word 1: CPUID 01h edx */
#define FEATURE_TSC		(32 * 1		+ 4)
#define FEATURE_PAT		(32 * 1		+ 16)

/* Feature word 2: CPUID 07h ebx */
#define FEATURE_RDSEED		(32 * 2		+ 18)

/* Feature word 3: CPUID 07h ecx */
#define FEATURE_LA57		(32 * 3		+ 16)

/* Feature word 4: CPUID extended 01h edx */
#define FEATURE_NX		(32 * 4		+ 20)
#define FEATURE_PDPE1GB		(32 * 4		+ 26)

/* Feature word 5: CPUID extended 07h edx */
#define FEATURE_TSCINV		(32 * 5		+ 8)

/**
 * Invoke the macro _X with all features and a feature name as arguments.
 */
#define _FEATURES(_X)					\
	_X(FEATURE_X2APIC,	"x2apic")		\
	_X(FEATURE_TSC,		"tsc")			\
	_X(FEATURE_PAT,		"pat")			\
	_X(FEATURE_RDSEED,	"rdseed")		\
	_X(FEATURE_LA57,	"la57")			\
	_X(FEATURE_NX,		"nx")			\
	_X(FEATURE_PDPE1GB,	"pdpe1gb")		\
	_X(FEATURE_TSCINV,	"tscinv")

#endif /* _FEATURES_ENUM */
