/**
 * Filesystem API.
 * Copyright (C) 2025-present  dbstream
 */
#pragma once

struct DEntry;
struct Filesystem;
struct INode;
struct Mount;

struct File;

#include <stddef.h>

/*
 * Reference counting operations:
 */

DEntry *dget (DEntry *);
void dput (DEntry *);
INode *iget (INode *);
void iput (INode *);
Mount *mnt_get (Mount *);
void mnt_put (Mount *);
Filesystem *fs_get (Filesystem *);
void fs_put (Filesystem *);

/*
 * File system initialization:
 */

void
init_fs_caches (void);

DEntry *
allocate_root_dentry (Filesystem *fs);

DEntry *
d_lookup (DEntry *parent, const char *name, size_t name_len);

int
d_ensure_inode (DEntry *dentry);

void
d_set_inode (DEntry *dentry, INode *inode);

void
d_set_inode_nocache (DEntry *dentry, INode *inode);

void
d_set_nocache (DEntry *dentry);

void
d_unlink (DEntry *dentry);

void
d_rename (DEntry *from, DEntry *to, unsigned int rename_flags);

void
d_set_overmounted (DEntry *dentry);

void
d_clear_overmounted (DEntry *dentry);

void
d_trim_lru_full (void);

void
d_trim_lru_partial (void);

