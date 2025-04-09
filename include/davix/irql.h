/**
 * IRQL utilities.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#include <asm/irql.h>

class scoped_irql {
	irql_cookie_t m_cookie;

public:
	inline
	scoped_irql (irql_t irql)
		: m_cookie (raise_irql (irql))
	{}

	inline
	~scoped_irql (void)
	{
		lower_irql (m_cookie);
	}

	scoped_irql (void) = delete;
	scoped_irql (scoped_irql &) = delete;
	scoped_irql (scoped_irql &&) = delete;
	scoped_irql &operator= (scoped_irql &&) = delete;
};
