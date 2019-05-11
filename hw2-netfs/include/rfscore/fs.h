#pragma once

#include <stddef.h>

#define FILE_NAME_LENGTH 256

#pragma pack(push, 1)

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

typedef struct PortabilityControlValues {
	unsigned int size_type_size;
	unsigned int superblock_type_size;
	unsigned int inode_type_size;
	unsigned int inodemain_type_size;
	unsigned int directoryitem_type_size;
} PortabilityControlValues;

typedef struct Superblock {
	size_t block_size;
	size_t blocks_count;
	PortabilityControlValues portability_control;
} Superblock;

#pragma pack(pop)

char is_platform_compatible(PortabilityControlValues);
PortabilityControlValues get_platform_values();
