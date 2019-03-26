#include <manage.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/param.h>
#include <stdlib.h>
#include <myutil.h>


void* open_and_map(const char* filename, size_t size) {
	int fd = open(filename, O_RDWR | (size ? O_CREAT : 0), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
	die("Error occurd while opening file");
	if (size) {
		ftruncate(fd, size);
	}
	die("Error occurd while preparing file");
	void* container = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	die("Error occurd while opening file");
	close(fd);
	die("Internal error");
	return container;
}

void mark_block_in_lookup_table(FsDescriptors fs, const size_t block_index, char occupied) {
	size_t byte_index = block_index / 8;
	size_t bit_index = block_index % 8;
	if (block_index >= fs.superblock->blocks_count)
		die_fatal("Internal error");
	if (occupied)
		fs.lookup_table[byte_index] |= (1 << bit_index);
	else
		fs.lookup_table[byte_index] &= ~(1 << bit_index);
}

size_t get_next_free_block_index(FsDescriptors fs) {
	size_t lookup_table_size = (fs.superblock->blocks_count + 7) / 8;
	for (size_t i = 0; i < lookup_table_size; ++i) {
		unsigned char cur_byte = fs.lookup_table[i];
		if (~cur_byte == 0)
			continue;
		for (size_t k = 0; k < 8; ++k) {
			if (cur_byte & (1 << k))
				return i * 8 + k;
		}
	}
	die_fatal("No space");
}

FsDescriptors prepare_descriptors(void* container) {
	Superblock* sblock = (Superblock*) container;
	size_t lookup_table_size = (sblock->blocks_count + 7) / 8;
	size_t metadata_size = sizeof(Superblock) + lookup_table_size;
	size_t metadata_pages_count = (metadata_size + sblock->block_size - 1) / sblock->block_size;

	FsDescriptors res = {
		container,
		sblock,
		(unsigned char*) (container + sizeof(Superblock)),
		metadata_pages_count * sblock->block_size
	};

	memset(res.lookup_table, 0, lookup_table_size);

	for (size_t i = 0; i < metadata_pages_count; ++i)
		mark_block_in_lookup_table(res, i, 1);

	return res;
}

FsDescriptors init_fs(const char* filename, size_t size) {
	if (size < sizeof(Superblock))
		size = sizeof(Superblock);
	size_t page_size = get_page_size();
	size_t pages_count = (size + page_size - 1) / page_size;
	size = pages_count * page_size;

	void* container = open_and_map(filename, size);
	Superblock* superblock = (Superblock*) container;
	superblock->block_size = page_size;
	superblock->blocks_count = pages_count;

	FsDescriptors fs = prepare_descriptors(container);
	size_t root_inode = init_new_file(fs);
	if (root_inode != fs.root_inode)
		die_fatal("Internal error");
	return fs;
}

FsDescriptors open_fs(const char* filename) {
	void* container = open_and_map(filename, 0);
	return prepare_descriptors(container);
}

void close_fs(FsDescriptors fs) {
	munmap(fs.container, fs.superblock->blocks_count * fs.superblock->block_size);
	die("Error occured while saving data");
}

size_t init_new_file(FsDescriptors fs) {
	size_t block_index = get_next_free_block_index(fs);
	mark_block_in_lookup_table(fs, block_index, 1);
	void* block = fs.container + block_index * fs.superblock->block_size;
	INodeMain* inode = (INodeMain*) block;
	inode->size = 0;
	inode->node.continuation_inode = 0;
	inode->links_count = 0;
	inode->last_inode = block_index;
	return block_index;
}

INode* get_inode(FsDescriptors fs, const size_t block_index) {
	if (!block_index)
		return NULL;
	if (block_index >= fs.superblock->blocks_count)
		die_fatal("Internal error");

	void* block = fs.container + block_index * fs.superblock->block_size;
	return (INode*) block;
}

INode* get_next_inode(FsDescriptors fs, INode* inode) {
	return get_inode(fs, inode->continuation_inode);
}

void purge_file(FsDescriptors fs, size_t block_index) {
	INode* inode = get_inode(fs, block_index);
	while (inode) {
		mark_block_in_lookup_table(fs, block_index, 0);
		block_index = inode->continuation_inode;
		inode = get_next_inode(fs, inode);
	}
}

size_t get_blocks_required(FsDescriptors fs, size_t size) {
	const size_t first_block_size = fs.superblock->block_size - sizeof(INodeMain);
	const size_t common_block_size = fs.superblock->block_size - sizeof(INode);
	if (size <= first_block_size)
		return 1;
	return (size - first_block_size + common_block_size - 1) / common_block_size + 1;
}

void truncate_file(FsDescriptors fs, size_t block_index, size_t new_size) {
	INodeMain* inode = (INodeMain*) get_inode(fs, block_index);

	const size_t old_size = inode->size;
	const size_t old_blocks_required = get_blocks_required(fs, old_size);
	const size_t new_blocks_required = get_blocks_required(fs, new_size);

	inode->size = new_size;

	if (new_blocks_required > old_blocks_required) {
		// extend
		block_index = inode->last_inode;
		for (size_t i = old_blocks_required; i < new_blocks_required; ++i) {
			size_t next_block = get_next_free_block_index(fs);
			mark_block_in_lookup_table(fs, next_block, 1);
			INode* current_inode = get_inode(fs, block_index);
			current_inode->continuation_inode = next_block;
			block_index = next_block;
		}
		inode->last_inode = block_index;
	} else if (new_blocks_required < old_blocks_required) {
		// shrink
		INode* current_inode = (INode*) inode;
		for (size_t i = 1; i < new_blocks_required; ++i)
			current_inode = get_next_inode(fs, current_inode);
		purge_file(fs, current_inode->continuation_inode);
	}
}

size_t read_file(FsDescriptors fs, size_t block_index, size_t offset, size_t length, void* buffer) {
	if (!length)
		return 0;

	INodeMain* inode = (INodeMain*) get_inode(fs, block_index);
	const size_t size = inode->size;
	if (offset == size)
		return 0;
	if (offset > size) 
		die_fatal("Invalid position in file");
	if (offset + length > size)
		length = size - offset;
	
	const size_t first_block_size = fs.superblock->block_size - sizeof(INodeMain);
	const size_t common_block_size = fs.superblock->block_size - sizeof(INode);
	size_t current_read_offset = sizeof(INodeMain);
	INode* current_inode = (INode*) inode;

	if (offset > first_block_size) {
		offset -= first_block_size;
		block_index = current_inode->continuation_inode;
		current_inode = get_next_inode(fs, current_inode);
		current_read_offset = sizeof(INode);
	}

	while (offset > common_block_size) {
		offset -= common_block_size;
		block_index = current_inode->continuation_inode;
		current_inode = get_next_inode(fs, current_inode);	
	}

	while (length) {
		const size_t copy_len = MIN(common_block_size, length);
		memcpy(buffer, fs.container + block_index * fs.superblock->block_size + offset + current_read_offset, copy_len);

		current_read_offset = sizeof(INode);
		offset = 0;
		length -= copy_len;

		if (length) {
			block_index = current_inode->continuation_inode;
			current_inode = get_next_inode(fs, current_inode);	
		}
	}

	return length;
}

size_t read_entire_file(FsDescriptors fs, size_t block_index, void* buffer) {
	INodeMain* inode = (INodeMain*) get_inode(fs, block_index);
	return read_file(fs, block_index, 0, inode->size, buffer);
}

void write_file(FsDescriptors fs, size_t block_index, size_t offset, size_t length, void* buffer) {
	if (!length)
		return;

	INodeMain* inode = (INodeMain*) get_inode(fs, block_index);
	const size_t size = inode->size;
	if (offset > size) 
		die_fatal("Invalid position in file");

	if (length + offset > size)
		truncate_file(fs, block_index, length + offset);
	
	const size_t first_block_size = fs.superblock->block_size - sizeof(INodeMain);
	const size_t common_block_size = fs.superblock->block_size - sizeof(INode);
	// TODO
}

Directory read_directory(FsDescriptors fs, size_t dir_block_index) {
	INodeMain* inode = (INodeMain*) get_inode(fs, dir_block_index);
	const size_t cur_dir_size = inode->size;
	if (cur_dir_size % sizeof(DirectoryItem))
		die_fatal("Internal error");

	void* buffer = malloc(cur_dir_size);
	read_file(fs, dir_block_index, 0, cur_dir_size, buffer);

	Directory res = {cur_dir_size / sizeof(DirectoryItem), (DirectoryItem*) buffer};
	return res;
}
