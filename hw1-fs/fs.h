#pragma once

#include <stddef.h>

#define FILE_NAME_LENGTH 256

typedef struct Superblock {
	size_t block_size;
	size_t blocks_count;
} Superblock;

typedef struct INode {
	size_t continuation_inode;
} INode;

typedef struct INodeMain {
	INode node;
	size_t size;
	size_t links_count;
	size_t last_inode;
} INodeMain;

typedef struct DirectoryItem {
	size_t inode;
	char name[FILE_NAME_LENGTH];
} DirectoryItem;
