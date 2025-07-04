/**
 * refstr - Simple refcounted strings.
 * Copyright (C) 2025-present  dbstream
 */
#include <davix/kmalloc.h>
#include <davix/refstr.h>
#include <string.h>

void
put_refstr (RefStr *str)
{
	if (refcount_dec (&str->refcount))
		kfree (str);
}

RefStr *
make_refstr (const char *name)
{
	size_t length = strlen (name) + 1;
	RefStr *str = (RefStr *) kmalloc (sizeof (RefStr) + length, ALLOC_KERNEL);

	if (str)
		memcpy (&str->string, &name, length);
	return str;
}
