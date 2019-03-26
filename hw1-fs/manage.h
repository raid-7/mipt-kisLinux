#pragma once

#include <stddef.h>
#include <fs.h>
#include <myutil.h>

typedef struct FsDescriptors {
	void* container;
	Superblock* superblock;
	unsigned char* lookup_table;
	size_t root_inode;
} FsDescriptors;

typedef struct Directory {
	size_t items_count;
	DirectoryItem* items;
} Directory;

FsDescriptors open_fs(const char*);
FsDescriptors init_fs(const char*, size_t);
void close_fs(FsDescriptors);
size_t init_new_file(FsDescriptors);
size_t read_file(FsDescriptors, size_t, size_t, size_t, void*);
size_t read_entire_file(FsDescriptors, size_t, void*);
void truncate_file(FsDescriptors, size_t, size_t);
void purge_file(FsDescriptors, size_t);
