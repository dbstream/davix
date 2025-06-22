/**
 * Cache properties and definitions.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

/*
 * My AMD Ryzen 9 7950 X3D reports a cache_alignment of 64 in /proc/cpuinfo, so
 * that's what we're going with for now.
 */
#define CACHELINE_SIZE		64

