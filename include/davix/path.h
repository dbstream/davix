/**
 * struct Path
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

struct DEntry;
struct Mount;

struct Path {
	Mount *mount;
	DEntry *dentry;
};

/*
Path
path_get (Path path);

void
path_put (Path path);
*/

