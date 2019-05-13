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
char warn(const char*);
void die_fatal(const char*);
size_t get_page_size();

void intercept_errors(void (*interceptor) (const char*));
void configure_error_logging(char to_stderr, char to_syslog);

#define STRING(...) #__VA_ARGS__


typedef struct String {
    size_t length;
    char* string;
} String;

String to_string(const char* s);
void free_string(String string);
