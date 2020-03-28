#pragma once

#include "../kernel/phonedir.h"

long phonedir_add(struct phonedir_record* record);

long phonedir_del(const char* surname, unsigned int length);

long phonedir_find(const char* surname, unsigned int length, struct phonedir_record* record);
