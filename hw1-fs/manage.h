#pragma once

#include <stddef.h>
#include <fs.h>
#include <myutil.h>

#define FLG_DIRECTORY (1 << 0)
#define FLG_FILE (1 << 1)

typedef struct FsDescriptors {
	void* container;
	Superblock* superblock;
	unsigned char* lookup_table;
	size_t root_inode;
} FsDescriptors;

typedef struct DirectoryContent {
	size_t items_count;
	DirectoryItem* items;
} DirectoryContent;

FsDescriptors open_fs(const char*);
FsDescriptors init_fs(const char*, size_t);
void close_fs(FsDescriptors);
size_t init_new_file(FsDescriptors, const unsigned char);
size_t read_file(FsDescriptors, size_t, size_t, size_t, void*);
size_t read_entire_file(FsDescriptors, size_t, void*);
// void truncate_file(FsDescriptors, size_t, size_t);
void purge_file(FsDescriptors, size_t);
void write_file(FsDescriptors, size_t, size_t, size_t, void*);
void append_file(FsDescriptors, size_t, size_t, void*);
DirectoryContent read_directory(FsDescriptors, size_t);
void free_directory(DirectoryContent);
size_t locate_path(FsDescriptors, Path);
void append_directory(FsDescriptors, size_t, DirectoryItem);
size_t remove_from_directory(FsDescriptors fs, size_t, const char*);
