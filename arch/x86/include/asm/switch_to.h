/**
 * Architecture-specific task switch code.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

struct Task;

Task *
arch_context_switch (Task *me, Task *next);

void
arch_send_reschedule_IPI (unsigned int target);
