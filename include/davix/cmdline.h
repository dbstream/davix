/**
 * Kernel command line
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

void
set_command_line (const char *cmdline);

const char *
get_early_param (const char *param, int idx);

bool
early_param_matches (const char *expected, const char *value);
