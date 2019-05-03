#include <myutil.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void die(const char* string) {
	if (errno) {
		perror(string);
		exit(1);
	}
}

void die_fatal(const char* string) {
	fprintf(stderr, "%s\n", string);
	exit(1);
}

size_t get_page_size() {
	long res = sysconf(_SC_PAGESIZE);
	die("Internal error");
	return (size_t) res;
}

Path split_path(const char* path) {
	if (*path != '/')
		die_fatal("Invalid path");

	size_t k = 0, len = 0;
	for (const char* p = path; *p; ++p) {
		if (*p == '/' && *(p+1) == '/')
			die_fatal("Invalid path");
		if (*p == '/' && *(p + 1))
			++k;
		if (*p != '/')	
			++len;
	}

	const char** parts = calloc(sizeof(char*), k + 1);
	char* line = calloc(1, len + k);
	const char** res_pointer = parts;
	char* line_pointer = line;

	*res_pointer = line_pointer;
	for (const char* p = path; *p; ++p) {
		if (*p != '/') {
			*(line_pointer++) = *p;
		} else if (p != path && *(p + 1)) {
			++line_pointer;
			*(++res_pointer) = line_pointer;
		}
	}

	Path res = {line, parts, k};
	return res;
}

void free_path(Path path) {
	free((void*) path.parts);
	free((void*) path.line);
}
