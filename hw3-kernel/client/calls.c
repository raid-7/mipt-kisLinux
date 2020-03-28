#include <unistd.h>
#include "calls.h"

#define __NR_extspecified_ptr 436
#define __NR_extspecified_ptr_uint 437
#define __NR_extspecified_ptr_uint_ptr 438


long phonedir_add(struct phonedir_record* record) {
    return syscall(__NR_extspecified_ptr, record);
}

long phonedir_del(const char* surname, unsigned int length) {
    return syscall(__NR_extspecified_ptr_uint, surname, length);
}

long phonedir_find(const char* surname, unsigned int length, struct phonedir_record* record) {
    return syscall(__NR_extspecified_ptr_uint_ptr, surname, length, record);
}
