#include <rfsnet/net.h>
#include <rfscore/manage.h>
#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>


extern char error_intercepted;

void write_empty_error(int sockd) {
    if (!error_intercepted) {
        String empty = to_string("");
        if (!slz_String_write(sockd, &empty)) {
            syslog(LOG_WARNING, "Connection problems");
            shutdown(sockd, SHUT_RDWR);
        }
        free_string(empty);
    }
}

void fsop_Read(FsDescriptors fds, FsOpRead_args* args, int sockd) {
    Path path = split_path(args->path.string);
    size_t inode = locate_path(fds, path);

    char* data = malloc(args->length);
    die("Memory allocation failed");

    size_t read = read_file(fds, inode, args->offset, args->length, data);
    String result = {read, data};

    write_empty_error(sockd);
    if (!slz_String_write(sockd, &result)) {
        syslog(LOG_WARNING, "Connection problems");
        shutdown(sockd, SHUT_RDWR);
    }

    free_string(result);
    free_path(path);
}

void fsop_Write(FsDescriptors fds, FsOpWrite_args* args, int sockd) {
    Path path = split_path(args->path.string);
    size_t inode = locate_path(fds, path);

    write_file(fds, inode, args->offset, args->length, args->data);

    write_empty_error(sockd);
    free_path(path);
}

void fsop_Create(FsDescriptors fds, FsOpCreate_args* args, int sockd) {
    Path path = split_path(args->path.string);

    --path.count;
    size_t dir_inode = locate_path(fds, path);
    const char* name = path.parts[path.count];
    ++path.count;

    if (strlen(name) >= FILE_NAME_LENGTH)
        die_fatal("Too long file name");

    size_t file_inode = init_new_file(fds, args->flags);
    DirectoryItem item = {.inode = file_inode};
    for (size_t i = 0; *name; ++name)
        item.name[i++] = *name;
    append_directory(fds, dir_inode, item);

    write_empty_error(sockd);

    free_path(path);
}

void fsop_Remove(FsDescriptors fds, FsOpRemove_args* args, int sockd) {
    Path path = split_path(args->path.string);

    --path.count;
    size_t dir_inode = locate_path(fds, path);
    const char* name = path.parts[path.count];
    ++path.count;

    size_t file_inode = remove_from_directory(fds, dir_inode, name);
    purge_file(fds, file_inode);

    write_empty_error(sockd);

    free_path(path);
}

void fsop_Stat(FsDescriptors fds, FsOpStat_args* args, int sockd) {
    Path path = split_path(args->path.string);

    size_t inode = locate_path(fds, path);
    size_t size = get_file_size(fds, inode);
    size_t blocks = get_blocks_required(fds, size);

    FsOpStat_response response;

    response.size = size;
    response.blocks_count = blocks;
    response.blocks = trace_file_blocks(fds, inode);

    write_empty_error(sockd);
    if (!slz_FsOpStatResponse_write(sockd, &response)) {
        syslog(LOG_WARNING, "Connection problems");
        shutdown(sockd, SHUT_RDWR);
    }

    free((void*) response.blocks);
    free_path(path);
}

void fsop_ReadDir(FsDescriptors fds, FsOpReadDir_args* args, int sockd) {
    Path path = split_path(args->path.string);

    size_t dir_inode = locate_path(fds, path);
    DirectoryContent content = read_directory(fds, dir_inode);

    write_empty_error(sockd);
    if (!slz_DirectoryContent_write(sockd, &content)) {
        syslog(LOG_WARNING, "Connection problems");
        shutdown(sockd, SHUT_RDWR);
    }

    free_directory(content);
    free_path(path);
}
