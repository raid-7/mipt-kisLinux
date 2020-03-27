#pragma once

#define PHONEDIR_NAME_LEN 128
#define PHONEDIR_EMAIL_LEN 64
#define PHONEDIR_PHONE_LEN 64

struct phonedir_record {
    char surname[PHONEDIR_NAME_LEN];
    char name[PHONEDIR_NAME_LEN];
    char email[PHONEDIR_EMAIL_LEN];
    char phone[PHONEDIR_PHONE_LEN];
    unsigned int age;
};

enum phonedir_op_type {
    PHONEDIR_ADD = 1,
    PHONEDIR_DELETE = 2,
    PHONEDIR_FIND = 3
};
