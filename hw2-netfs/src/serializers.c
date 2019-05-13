#include <serialization.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <rfsnet/net.h>
#include <rfscore/manage.h>
#include <operation_defines.h>

void catch(const char* s) {
    warn(s);
}

_SLZ_SERIALIZER_RAW(NetFsOperation, slz_NetFsOperation, {
    _SLZ_FIELD(enum FsOpType, type, catch);
    if (!strcmp("read", context)) {
        _FSOP_readers;
    } else if (!strcmp("write", context)) {
        _FSOP_writers;
    }
})

_SLZ_SERIALIZER(FsOpRead_args, slz_FsOpRead, catch,
        _SLZ_ARRAY(path.string, path.length, char, catch))
_SLZ_SERIALIZER(FsOpWrite_args, slz_FsOpWrite, catch,
        _SLZ_ARRAY(path.string, path.length, char, catch)
        _SLZ_ARRAY(data, length, char, catch))

_SLZ_SERIALIZER(FsOpCreate_args, slz_FsOpCreate, catch, _SLZ_ARRAY(path.string, path.length, char, catch))
_SLZ_SERIALIZER(FsOpRemove_args, slz_FsOpRemove, catch, _SLZ_ARRAY(path.string, path.length, char, catch))
_SLZ_SERIALIZER(FsOpStat_args, slz_FsOpStat, catch, _SLZ_ARRAY(path.string, path.length, char, catch))
_SLZ_SERIALIZER(FsOpReadDir_args, slz_FsOpReadDir, catch, _SLZ_ARRAY(path.string, path.length, char, catch))

_SLZ_SERIALIZER(String, slz_String, catch, _SLZ_ARRAY(string, length, char, catch))

_SLZ_SERIALIZER(FsOpStat_response, slz_FsOpStatResponse, catch, _SLZ_ARRAY(blocks, blocks_count, size_t, catch))

_SLZ_SERIALIZER(DirectoryContent, slz_DirectoryContent, catch, _SLZ_ARRAY(items, items_count, DirectoryItem, catch))
