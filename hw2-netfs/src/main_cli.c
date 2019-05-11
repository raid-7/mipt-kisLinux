#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>

#include <rfsnet/net.h>
#include <myutil.h>
#include <operation_defines.h>


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
if (argc == (arg_count) + 3 && strcasecmp(#cmd_name, argv[2]) == 0) {\
	{\
		const char* address = argv[1];\
		const char** args = argv + 3;\
		char command_accepted = 1;\
		executor;\
		if (command_accepted) \
			return 0;\
	}\
}

void read_error(int sockd) {
    String err;
    if (!slz_String_read(sockd, &err)) {
        die_fatal("Connection problems");
    }

    if (err.length - 1) {
        die_fatal(err.string);
    }

    free_string(err);
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

void make_item(int sockd, const char* s_path, unsigned char flags) {
    NetFsOperation op;
    op.type = FSOP_Create;
    op.Create_args.path = to_string(s_path);
    op.Create_args.flags = flags;
    if (!slz_NetFsOperation_write(sockd, &op)) {
        die_fatal("Connection problems");
    }
    _FSOP_clear(&op);

    read_error(sockd);
}

int main(int argc, const char* argv[]) {
	CMD(mkfile, 1, {
		int sockd = initialize_client(address);
		make_item(sockd, args[0], FLG_FILE);
		destroy_client(sockd);
	});

	CMD(mkdir, 1, {
        int sockd = initialize_client(address);
		make_item(sockd, args[0], FLG_DIRECTORY);
		destroy_client(sockd);
	});

	CMD(ls, 1, {
        NetFsOperation op;
        op.type = FSOP_ReadDir;
        op.ReadDir_args.path = to_string(args[0]);

        int sockd = initialize_client(address);
        if (!slz_NetFsOperation_write(sockd, &op))
            die_fatal("Connection problems");

        read_error(sockd);
        DirectoryContent content;
        if (!slz_DirectoryContent_read(sockd, &content)) {
            die_fatal("Connection problems");
        }

        for (size_t i = 0; i < content.items_count; ++i) {
            printf("%lu %s\n", content.items[i].inode, content.items[i].name);
        }

        free_directory(content);
        destroy_client(sockd);
	});

//	CMD(cat, 1, {
//		Path path = split_path(args[0]);
//        int sockd = initialize_client(address);
//
//		size_t inode = locate_path(fs, path);
//
//		const size_t buf_size = 1024*1024*2;
//		void* buffer = malloc(buf_size);
//		size_t read_bytes;
//		size_t total_read_bytes = 0;
//		while (read_bytes = read_file(fs, inode, total_read_bytes, buf_size, buffer)) {
//			fwrite(buffer, 1, read_bytes, stdout);
//			total_read_bytes += read_bytes;
//		}
//
//		free(buffer);
//		destroy_client(sockd);
//		free_path(path);
//	});
//
//	CMD(write, 1, {
//        int sockd = initialize_client(address);
//
//		perform_write(fs, args[0], 0);
//
//		destroy_client(sockd);
//	});
//
//	CMD(write, 2, {
//		unsigned long long offset;
//		if (!sscanf(args[1], "%llu", &offset)) {
//			die_fatal("Invalid argument");
//		}
//		die("Internal error");
//
//        int sockd = initialize_client(address);
//
//		perform_write(fs, args[0], (size_t) offset);
//
//		destroy_client(sockd);
//	});

	CMD(rm, 1, {
        NetFsOperation op;
        op.type = FSOP_Remove;
        op.Remove_args.path = to_string(args[0]);

        int sockd = initialize_client(address);
        if (!slz_NetFsOperation_write(sockd, &op))
            die_fatal("Connection problems");

        read_error(sockd);

		destroy_client(sockd);
	});

	CMD(stat, 1, {
        NetFsOperation op;
        op.type = FSOP_Stat;
        op.Stat_args.path = to_string(args[0]);

        int sockd = initialize_client(address);
        if (!slz_NetFsOperation_write(sockd, &op))
            die_fatal("Connection problems");

        FsOpStat_response response;
        if (!slz_FsOpStatResponse_read(sockd, &response)) {
            die_fatal("Connection problems");
        }

        printf("%lu bytes, %lu blocks\n", response.size, response.blocks_count);
        for (size_t i = 0; i < response.blocks_count; ++i)
            printf("%lu ", response.blocks[i]);
        printf("\n");

        free(response.blocks);
		destroy_client(sockd);
	});

	die_with_help();

	return 0;
}
