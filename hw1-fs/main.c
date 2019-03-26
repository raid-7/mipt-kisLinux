#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
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
  rm <path>\n
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

int main(int argc, const char* argv[]) {
	CMD(init, 1, {
		unsigned long long size;
		sscanf(args[0], "%llu", &size);
		die("Internal error");
		FsDescriptors fs = init_fs(filename, size);
		printf("Ready: %lu blocks of %lu bytes\n", fs.superblock->blocks_count, fs.superblock->block_size);
		close_fs(fs);
	});

	CMD(mkfile, 1, {
		Path path = split_path(args[0]);
		for (size_t i = 0; i < path.count; ++i)
			printf("%s\n", path.parts[i]);
		free_path(path);
	});

	die_with_help();

	return 0;
}
