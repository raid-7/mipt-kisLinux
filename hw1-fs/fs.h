#pragma once

#include <stddef.h>

#define FILE_NAME_LENGTH 256

typedef struct INode {
	size_t continuation_inode;
} INode;

typedef struct INodeMain {
	INode node;
	size_t size;
	unsigned int links_count;
	unsigned char flags;
	size_t last_inode;
} INodeMain;

typedef struct DirectoryItem {
	size_t inode;
	char name[FILE_NAME_LENGTH];
} DirectoryItem;

typedef struct PortablilityControlValues {
	unsigned int size_type_size;
	unsigned int superblock_type_size;
	unsigned int inode_type_size;
	unsigned int inodemain_type_size;
	unsigned int directoryitem_type_size;
} PortablilityControlValues;

typedef struct Superblock {
	size_t block_size;
	size_t blocks_count;
	PortablilityControlValues portability_control;
} Superblock;

char is_platform_compatible(const PortablilityControlValues);
PortablilityControlValues get_platform_values();
