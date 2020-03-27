#pragma once

#include <linux/list.h>

#include "phonedir.h"

struct directory_entry {
    struct phonedir_record record;
    struct list_head list;
};


struct directory_entry* phonedir_alloc(void);

void phonedir_add(struct list_head *list, struct directory_entry *new_entry);

int phonedir_add_record(struct list_head *list, struct phonedir_record *new_record);

struct directory_entry* phonedir_entry(struct list_head *list_element);

struct list_head phonedir_find(struct list_head *list, const char *surname);

void phonedir_del(struct list_head *list, const char *surname);

void phonedir_free(struct list_head* list);
