#pragma once

#include <string.h>


char safe_read(int fd, void* buf, size_t len, void (*error_handler) (const char*));
char safe_write(int fd, void* buf, size_t len, void (*error_handler) (const char*));


#define _if0(...)
#define _if1(...) __VA_ARGS__
#define _if(val, ...) _if ## val(__VA_ARGS__)
#define _ifnot0(...) __VA_ARGS__
#define _ifnot1(...)
#define _ifnot(val, ...) _ifnot ## val(__VA_ARGS__)

#define _SLZ_ARRAY_READER_IMPL(field, count_field, struct_size, error_handler) {\
    if (!safe_read(fd, &(count_field), sizeof(count_field), error_handler)) return 0;\
    if (count_field) {\
        field = malloc(struct_size * (count_field));\
        error_handler("Memory allocation failed");\
        if (!(field)) return 0;\
        if (!safe_read(fd, (field), struct_size * (count_field), error_handler)) return 0;\
    }\
}

#define _SLZ_ARRAY_WRITER_IMPL(field, count_field, struct_size, error_handler) {\
    if (!(field) && (count_field)) return 0;\
    if (!safe_write(fd, &(count_field), sizeof(count_field), error_handler)) return 0;\
    if (count_field)\
        if (!safe_write(fd, (field), struct_size * (count_field), error_handler)) return 0;\
}


#define _SLZ_ARRAY(field_name, count_field_name, elem_type, error_handler) {\
    if (!strcmp("read", context)) {\
        _SLZ_ARRAY_READER_IMPL(value->field_name, value->count_field_name, sizeof(elem_type), error_handler)\
    } else if (!strcmp("write", context)) {\
        _SLZ_ARRAY_WRITER_IMPL(value->field_name, value->count_field_name, sizeof(elem_type), error_handler)\
    }\
}

#define _SLZ_SERIALIZER_READER_IMPL(type_name, value, error_handler, raw, ...) \
    _ifnot(raw, {\
        if (!safe_read(fd, value, sizeof(type_name), error_handler)) return 0;\
    })\
    __VA_ARGS__

#define _SLZ_SERIALIZER_READER(type_name, func_name, error_handler, raw, body, ...) char func_name(int fd, type_name* value)\
_if(body, {\
    const char* context = "read";\
    _SLZ_SERIALIZER_READER_IMPL(type_name, value, error_handler, raw, __VA_ARGS__)\
    return 1;\
});

#define _SLZ_SERIALIZER_WRITER_IMPL(type_name, value, error_handler, raw, ...) \
    _ifnot(raw, {\
        if (!safe_write(fd, value, sizeof(type_name), error_handler)) return 0;\
    })\
    __VA_ARGS__

#define _SLZ_SERIALIZER_WRITER(type_name, func_name, error_handler, raw, body, ...) char func_name(int fd, type_name* value)\
_if(body, {\
    const char* context = "write";\
    _SLZ_SERIALIZER_WRITER_IMPL(type_name, value, error_handler, raw)\
    return 1;\
});

#define _SLZ_FIELD(field_type_name, field_name, error_handler, ...) {\
    if (!strcmp("read", context)) {\
        _SLZ_SERIALIZER_READER_IMPL(field_type_name, &value->field_name, error_handler, 0, __VA_ARGS__)\
    } else if (!strcmp("write", context)) {\
        _SLZ_SERIALIZER_WRITER_IMPL(field_type_name, &value->field_name, error_handler, 0, __VA_ARGS__)\
    }\
}

#define _SLZ_FIELD_RAW(field_type_name, field_name, ...) {\
    if (!strcmp("read", context)) {\
        _SLZ_SERIALIZER_READER_IMPL(field_type_name, &value->field_name, _unused, 1, __VA_ARGS__)\
    } else if (!strcmp("write", context)) {\
        _SLZ_SERIALIZER_WRITER_IMPL(field_type_name, &value->field_name, _unused, 1, __VA_ARGS__)\
    }\
}

#define _SLZ_SERIALIZER(type_name, func_name_prefix, error_handler, ...) \
_SLZ_SERIALIZER_READER(type_name, func_name_prefix ## _read, error_handler, 0, 1, __VA_ARGS__)\
_SLZ_SERIALIZER_WRITER(type_name, func_name_prefix ## _write, error_handler, 0, 1, __VA_ARGS__)

#define _SLZ_SERIALIZER_RAW(type_name, func_name_prefix, ...) \
_SLZ_SERIALIZER_READER(type_name, func_name_prefix ## _read, _unused, 1, 1, __VA_ARGS__)\
_SLZ_SERIALIZER_WRITER(type_name, func_name_prefix ## _write, _unused, 1, 1, __VA_ARGS__)

#define _SLZ_SERIALIZER_DEF(type_name, func_name_prefix) \
_SLZ_SERIALIZER_READER(type_name, func_name_prefix ## _read, _unused, _unused, 0)\
_SLZ_SERIALIZER_WRITER(type_name, func_name_prefix ## _write, _unused, _unused, 0)

