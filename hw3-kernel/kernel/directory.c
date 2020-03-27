#include "directory.h"

#include <linux/list.h>
#include <linux/string.h>
#include <linux/slab.h>


struct directory_entry* phonedir_alloc(void) {
    return kmalloc(sizeof(struct directory_entry), GFP_KERNEL);
}

void phonedir_add(struct list_head *list, struct directory_entry *new_entry) {
    list_add(&new_entry->list, list);
}

int phonedir_add_record(struct list_head *list, struct phonedir_record *new_record) {
    struct directory_entry *entry = phonedir_alloc();
    if (!entry) {
        return -ENOMEM;
    }

    memcpy(&entry->record, new_record, sizeof(struct phonedir_record));

    // ensure strings null terminated
    entry->record.surname[PHONEDIR_NAME_LEN - 1] = 0;
    entry->record.name[PHONEDIR_NAME_LEN - 1] = 0;
    entry->record.phone[PHONEDIR_PHONE_LEN - 1] = 0;
    entry->record.email[PHONEDIR_EMAIL_LEN - 1] = 0;

    phonedir_add(list, entry);
    return 0;
}

struct directory_entry* phonedir_entry(struct list_head *list_element) {
    return list_entry(list_element, struct directory_entry, list);
}

struct list_head phonedir_find(struct list_head *list, const char *surname) {
    struct list_head result;
    struct directory_entry *cursor;
    list_for_each_entry(cursor, list, list) {
        if (strcmp(surname, cursor->record.surname) == 0) {
            struct directory_entry *pointer = phonedir_alloc();
            if (!pointer)
                continue;

            memcpy(&pointer->record, &cursor->record, sizeof(struct phonedir_record));
            list_add(&pointer->list, &result);
        }
    }
    return result;
}

void phonedir_del(struct list_head *list, const char *surname) {
    struct directory_entry *cursor, *_n;
    list_for_each_entry_safe(cursor, _n, list, list) {
        if (strcmp(surname, cursor->record.surname) != 0)
            continue;
        __list_del_entry(&cursor->list);
        kfree(cursor);
    }
}

void phonedir_free(struct list_head *list) {
    struct directory_entry *cursor, *_n;
    list_for_each_entry_safe(cursor, _n, list, list) {
        kfree(cursor);
    }
    INIT_LIST_HEAD(list);
}
