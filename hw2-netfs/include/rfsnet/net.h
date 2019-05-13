#pragma once

#include <rfscore/manage.h>
#include <serialization.h>
#include <myutil.h>

#include <sys/types.h>
#include <netinet/in.h>
#include <stdatomic.h>
#include <string.h>

#include <operation_defines.h>


_SLZ_SERIALIZER_DEF(String, slz_String)

typedef struct NetServerDescriptors {
    struct sockaddr_in addr;
    int sockd;
    atomic_bool* termination_flag;
    pthread_mutex_t* mutex;
} NetServerDescriptors;

enum FsOpType {
    _FSOP_list
};

#pragma pack(push, 1)

typedef struct FsOpRead {
    String path;
    size_t offset;
    size_t length;
} FsOpRead_args;

typedef struct FsOpWrite {
    String path;
    size_t offset;
    size_t length;
    char* data;
} FsOpWrite_args;

typedef struct FsOpCreate {
    String path;
    unsigned char flags;
} FsOpCreate_args;

typedef struct FsOpRemove {
    String path;
} FsOpRemove_args;

typedef struct FsOpStat {
    String path;
} FsOpStat_args;

typedef struct FsOpReadDir {
    String path;
} FsOpReadDir_args;


typedef struct NetFsOperation {
    enum FsOpType type;
    union {
        _FSOP_args;
    };
} NetFsOperation;

typedef struct FsOpStatResponse {
    size_t size;
    size_t blocks_count;
    size_t* blocks;
} FsOpStat_response;
_SLZ_SERIALIZER_DEF(FsOpStat_response, slz_FsOpStatResponse)

_SLZ_SERIALIZER_DEF(DirectoryContent, slz_DirectoryContent)

#pragma pack(pop)

_FSOP_funcs;
_FSOP_serializer_definitions;
_SLZ_SERIALIZER_DEF(NetFsOperation, slz_NetFsOperation)

NetServerDescriptors initialize_server(int port);
void server_listen_connections(NetServerDescriptors, FsDescriptors);
void server_destroy(NetServerDescriptors);


int initialize_client(const char* address);
void destroy_client(int);
