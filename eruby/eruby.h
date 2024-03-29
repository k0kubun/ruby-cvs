/*
 * $Id$
 * Copyright (C) 2000  ZetaBITS, Inc.
 * Copyright (C) 2000  Information-technology Promotion Agency, Japan
 */

#ifndef ERUBY_H
#define ERUBY_H

#define ERUBY_VERSION "0.9.8"
#define ERUBY_VERSION_CODE 98

#define ERUBY_MIME_TYPE "application/x-httpd-eruby"

enum eruby_compile_status {
    ERUBY_OK = 0,
    ERUBY_MISSING_END_DELIMITER,
    ERUBY_INVALID_OPTION,
    ERUBY_SYSTEM_ERROR
};

enum eruby_mode {
    MODE_UNKNOWN,
    MODE_FILTER,
    MODE_CGI,
    MODE_NPHCGI
};

extern char *eruby_filename;
extern int eruby_mode;
extern int eruby_noheader;
extern VALUE eruby_charset;
extern VALUE eruby_default_charset;
#define ERUBY_CHARSET RSTRING(eruby_charset)->ptr

const char *eruby_version();
int eruby_parse_options(int argc, char **argv, int *optind);
VALUE eruby_compiler_new();
VALUE eruby_compiler_compile_file(VALUE self, VALUE file);
VALUE eruby_compiler_compile_string(VALUE self, VALUE s);
VALUE eruby_load(char *filename, int wrap, int *state);
void eruby_init();

#endif /* ERUBY_H */

/*
 * Local variables:
 * mode: C
 * tab-width: 8
 * End:
 */
