/**
 * CPU feature enumeration.
 * Copyright (C) 2024  dbstream
 */
#include <davix/string.h>
#include <asm/cpuid.h>
#include <asm/features.h>
#include <asm/sections.h>
#include <asm/msr.h>

unsigned long __efer_state = _EFER_LME;
unsigned long __cr0_state;
unsigned long __cr4_state;

unsigned int bsp_feature_array[6];

unsigned int cpu_vendor = CPU_VENDOR_UNKNOWN;
char cpu_brand[13] = "(unknown)";
char cpu_model[49] = "(unknown)";

static void
load_features (unsigned int *features,
	unsigned int *vendor, char *brand, char *model)
{
	uint32_t a, b, c, d;

	for (int i = 0; i < 6; i++)
		features[i] = 0;

	*vendor = CPU_VENDOR_UNKNOWN;
	strcpy (brand, "(unknown)");
	strcpy (model, "(unknown)");

	a = 0;
	c = 0;
	__cpuid (&a, &b, &c, &d);
	brand[0] = b;
	brand[1] = b >> 8;
	brand[2] = b >> 16;
	brand[3] = b >> 24;
	brand[4] = d;
	brand[5] = d >> 8;
	brand[6] = d >> 16;
	brand[7] = d >> 24;
	brand[8] = c;
	brand[9] = c >> 8;
	brand[10] = c >> 16;
	brand[11] = c >> 24;
	brand[12] = 0;

	if (!strcmp (brand, "AuthenticAMD")) {
		*vendor = CPU_VENDOR_AMD;
	} else if (!strcmp (brand, "GenuineIntel")) {
		*vendor = CPU_VENDOR_INTEL;
	} else {
		*vendor = CPU_VENDOR_UNKNOWN;
	}

	switch (a) {
	default:
		a = 0x7U;
		c = 0;
		__cpuid (&a, &b, &c, &d);
		features[2] = b;
		features[3] = c;
		__attribute__ ((fallthrough));
	case 1 ... 6:
		a = 0x1U;
		c = 0;
		__cpuid (&a, &b, &c, &d);
		features[0] = c;
		features[1] = d;
		__attribute__ ((fallthrough));
	case 0:;
	}

	a = 0x80000000U;
	c = 0;
	__cpuid (&a, &b, &c, &d);

	switch (a) {
	default:
		a = 0x80000007U;
		c = 0;
		__cpuid (&a, &b, &c, &d);
		features[5] = d;
		__attribute__ ((fallthrough));
	case 0x80000004U ... 0x80000006U:
		for (unsigned int i = 0; i < 3; i++) {
			a = 0x80000002U + i;
			c = 0;
			__cpuid (&a, &b, &c, &d);
			model[16 * i + 0] = a;
			model[16 * i + 1] = a >> 8;
			model[16 * i + 2] = a >> 16;
			model[16 * i + 3] = a >> 24;
			model[16 * i + 4] = b;
			model[16 * i + 5] = b >> 8;
			model[16 * i + 6] = b >> 16;
			model[16 * i + 7] = b >> 24;
			model[16 * i + 8] = c;
			model[16 * i + 9] = c >> 8;
			model[16 * i + 10] = c >> 16;
			model[16 * i + 11] = c >> 24;
			model[16 * i + 12] = d;
			model[16 * i + 13] = d >> 8;
			model[16 * i + 14] = d >> 16;
			model[16 * i + 15] = d >> 24;
		}
		__attribute__ ((fallthrough));
	case 0x80000001U ... 0x80000003U:
		a = 0x80000001U;
		c = 0;
		__cpuid (&a, &b, &c, &d);
		features[4] = d;
		__attribute__ ((fallthrough));
	case 0 ... 0x80000000U:;
	}
}

__INIT_TEXT
void
bsp_features_init (void)
{
	load_features (bsp_feature_array, &cpu_vendor, cpu_brand, cpu_model);
}
