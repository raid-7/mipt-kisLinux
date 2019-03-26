#pragma once

#include <stddef.h>

typedef struct Path {
	const char* line;
	const char* const* parts;
	size_t count;
} Path;

Path split_path(const char*);
void free_path(Path path);
void die(const char*);
void die_fatal(const char*);
size_t get_page_size();

#define STRING(...) #__VA_ARGS__
