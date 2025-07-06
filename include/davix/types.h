/**
 * Types used by various kernel interfaces.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

#include <stdint.h>

typedef uint16_t mode_t;
typedef uint64_t dev_t;
typedef int32_t pid_t;
typedef uint32_t uid_t;
typedef uint32_t gid_t;
typedef int64_t off_t;
typedef uint64_t nlink_t;
typedef uint64_t ino_t;

constexpr static inline dev_t
makedev (unsigned int major, unsigned int minor)
{
	return ((uint64_t) major << 32) | (uint64_t) minor;
}

constexpr static inline unsigned int
dev_major (dev_t dev)
{
	return dev >> 32;
}

constexpr static inline unsigned int
dev_minor (dev_t dev)
{
	return dev & 0xffffffffU;
}

