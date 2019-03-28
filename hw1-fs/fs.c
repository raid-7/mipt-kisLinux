#include <fs.h>

PortablilityControlValues get_platform_values() {
	return (PortablilityControlValues) {
		.size_type_size = sizeof(size_t),
		.superblock_type_size = sizeof(Superblock),
		.inode_type_size = sizeof(INode),
		.inodemain_type_size = sizeof(INodeMain),
		.directoryitem_type_size = sizeof(DirectoryItem)
	};
}

char is_platform_compatible(const PortablilityControlValues vals) {
	PortablilityControlValues mine = get_platform_values();
	return mine.size_type_size == vals.size_type_size &&
			mine.superblock_type_size == vals.superblock_type_size &&
			mine.inode_type_size == vals.inode_type_size &&
			mine.inodemain_type_size == vals.inodemain_type_size &&
			mine.directoryitem_type_size == vals.directoryitem_type_size;
}
