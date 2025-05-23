/**
 * IRQL utilities.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#include <asm/irql.h>

class scoped_dpc {
public:
	inline
	scoped_dpc (void)
	{
		disable_dpc ();
	}

	inline
	~scoped_dpc (void)
	{
		enable_dpc ();
	}
};

class scoped_irq {
public:
	inline
	scoped_irq (void)
	{
		disable_irq ();
	}

	inline
	~scoped_irq (void)
	{
		enable_irq ();
	}
};
