#include <myutil.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>

void (*global_interceptor) (const char* );

void die(const char* string) {
	if (errno) {
		syslog(LOG_ERR, "%s: %m", string);
		if (global_interceptor) {
		    const char* err_str = strerror(errno);
		    size_t len = strlen(err_str) + strlen(string) + 2;
		    char* full_str = malloc(len);
		    if (!full_str)
		        exit(1);
		    sprintf(full_str, "%s: %s", string, err_str);
		    global_interceptor(full_str);
		    free(full_str);
		} else
		    exit(1);
	}
}

void die_fatal(const char* string) {
	syslog(LOG_ERR, "%s\n", string);
	if (global_interceptor) {
	    global_interceptor(string);
	} else
	    exit(1);
}

char warn(const char* string) {
    if (errno) {
        syslog(LOG_WARNING, "%s: %m", string);
        errno = 0;
        return 1;
    }
    return 0;
}

void intercept_errors(void (*interceptor) (const char* )) {
    global_interceptor = interceptor;
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


void free_string(String s) {
    free(s.string);
}

String to_string(const char* s) {
    String res;
    res.length = strlen(s) + 1;
    res.string = malloc(res.length);
    die("Memory allocation failed");
    memcpy(res.string, s, res.length);
    return res;
}
