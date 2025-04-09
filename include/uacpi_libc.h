/**
 * Overriding uACPI libc definitions.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#include <string.h>

#define uacpi_memset memset
#define uacpi_memcpy memcpy
#define uacpi_mempcpy mempcpy
#define uacpi_memmove memmove
#define uacpi_memcmp memcmp
#define uacpi_strlen strlen
#define uacpi_strcpy strcpy
#define uacpi_stpcpy stpcpy
#define uacpi_strcmp strcmp
#define uacpi_strnlen strnlen
#define uacpi_strncpy strncpy
#define uacpi_stpncpy stpncpy
#define uacpi_strncmp strncmp
