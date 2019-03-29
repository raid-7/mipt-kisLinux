#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <manage.h>
#include <myutil.h>


const char* HELP_STRING = STRING(
Usage: ./rfs <path_to_fs> <command> <arguments> \n\n\
Commands with arguments: \n
  init <size> \n
  ls <path> \n
  cat <path> \n
  stat <path> \n
  mkdir <path> \n
  mkfile <path> \n
  write <path> [offset] \n
  rm <path> \n
);

void die_with_help() {
	printf("%s\n", HELP_STRING);
	exit(0);
}


#define CMD(cmd_name, arg_count, executor) \
if (argc == arg_count + 3 && strcasecmp(#cmd_name, argv[2]) == 0) {\
	{\
		const char* filename = argv[1];\
		const char** args = argv + 3;\
		char command_accepted = 1;\
		executor;\
		if (command_accepted) \
			return 0;\
	}\
}

void perform_write(FsDescriptors fs, const char* s_path, size_t offset) {
	Path path = split_path(s_path);
	const size_t inode = locate_path(fs, path);
	const size_t buf_size = 1024*1024*2;
	void* buffer = malloc(buf_size);

	while (!feof(stdin)) {
		size_t read_bytes = fread(buffer, 1, buf_size, stdin);
		if (ferror(stdin)) {
			die_fatal("Error occured while reading from input stream");
		}

		write_file(fs, inode, offset, read_bytes, buffer);
		for (size_t i = 0; i < read_bytes; ++i)
			if (!((char*)buffer)[i])
				printf("%lu\n", read_bytes);
		offset += read_bytes;
	}

	free(buffer);
	free_path(path);
}

void make_item(FsDescriptors fs, const char* s_path, unsigned char flags) {
	Path path = split_path(s_path);

	--path.count;
	size_t dir_inode = locate_path(fs, path);
	const char* name = path.parts[path.count];
	++path.count;

	if (strlen(name) >= FILE_NAME_LENGTH)
		die_fatal("Too long file name");

	size_t file_inode = init_new_file(fs, flags);
	DirectoryItem item = {.inode = file_inode};
	for (size_t i = 0; *name; ++name)
		item.name[i++] = *name;
	append_directory(fs, dir_inode, item);

	free_path(path);
}

int main(int argc, const char* argv[]) {
	CMD(init, 1, {
		unsigned long long size;
		if (!sscanf(args[0], "%llu", &size)) {
			die_fatal("Invalid argument");
		}
		die("Internal error");
		FsDescriptors fs = init_fs(filename, size);
		printf("Ready: %lu blocks of %lu bytes\n", fs.superblock->blocks_count, fs.superblock->block_size);
		close_fs(fs);
	});

	CMD(mkfile, 1, {
		FsDescriptors fs = open_fs(filename);
		make_item(fs, args[0], FLG_FILE);
		close_fs(fs);
	});

	CMD(mkdir, 1, {
		FsDescriptors fs = open_fs(filename);
		make_item(fs, args[0], FLG_DIRECTORY);
		close_fs(fs);
	});

	CMD(ls, 1, {
		Path path = split_path(args[0]);
		FsDescriptors fs = open_fs(filename);

		size_t dir_inode = locate_path(fs, path);
		DirectoryContent content = read_directory(fs, dir_inode);
		for (size_t i = 0; i < content.items_count; ++i) {
			printf("%lu %s\n", content.items[i].inode, content.items[i].name);
		}

		free_directory(content);
		close_fs(fs);
		free_path(path);
	});

	CMD(cat, 1, {
		Path path = split_path(args[0]);
		FsDescriptors fs = open_fs(filename);

		size_t inode = locate_path(fs, path);
		
		const size_t buf_size = 1024*1024*2;
		void* buffer = malloc(buf_size);
		size_t read_bytes;
		size_t total_read_bytes = 0;
		while (read_bytes = read_file(fs, inode, total_read_bytes, buf_size, buffer)) {
			fwrite(buffer, 1, read_bytes, stdout);
			total_read_bytes += read_bytes;
		}

		free(buffer);
		close_fs(fs);
		free_path(path);
	});

	CMD(write, 1, {
		FsDescriptors fs = open_fs(filename);

		perform_write(fs, args[0], 0);

		close_fs(fs);
	});

	CMD(write, 2, {
		unsigned long long offset;
		if (!sscanf(args[1], "%llu", &offset)) {
			die_fatal("Invalid argument");
		}
		die("Internal error");

		FsDescriptors fs = open_fs(filename);

		perform_write(fs, args[0], (size_t) offset);

		close_fs(fs);
	});

	CMD(rm, 1, {
		FsDescriptors fs = open_fs(filename);
		Path path = split_path(args[0]);

		--path.count;
		size_t dir_inode = locate_path(fs, path);
		const char* name = path.parts[path.count];
		++path.count;

		size_t file_inode = remove_from_directory(fs, dir_inode, name);
		purge_file(fs, file_inode);

		free_path(path);
		close_fs(fs);
	});

	CMD(stat, 1, {
		FsDescriptors fs = open_fs(filename);
		Path path = split_path(args[0]);

		size_t inode = locate_path(fs, path);
		size_t size = get_file_size(fs, inode);
		size_t blocks = get_blocks_required(fs, size);

		printf("%lu bytes, %lu blocks\n", size, blocks);
		const size_t* trace = trace_file_blocks(fs, inode);
		for (size_t i = 0; i < blocks; ++i)
			printf("%lu ", trace[i]);
		printf("\n");

		free((void*) trace);
		free_path(path);
		close_fs(fs);
	});

	die_with_help();

	return 0;
}
