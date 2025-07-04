/**
 * refstr - Simple refcounted strings.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#include <davix/refcount.h>

/**
 * RefStr - reference-counted string.
 */
struct RefStr {
	refcount_t refcount;
	char string[];
};

static inline RefStr *
get_refstr (RefStr *str)
{
	refcount_inc (&str->refcount);
	return str;
}

void
put_refstr (RefStr *str);

RefStr *
make_refstr (const char *name);

