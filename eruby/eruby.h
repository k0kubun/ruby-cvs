/*
 * $Id$
 * Copyright (C) 1999  Network Applied Communication Laboratory, Inc.
 */

#ifndef ERUBY_H
#define ERUBY_H

#define ERUBY_VERSION "0.0.2"

#define ERUBY_MIME_TYPE "application/x-httpd-eruby"

enum eruby_compile_status {
    COMPILE_OK = 0,
    MISSING_END_DELIMITER,
    SYSTEM_ERROR
};

extern char eruby_begin_delimiter1;
extern char eruby_begin_delimiter2;
extern char eruby_end_delimiter1;
extern char eruby_end_delimiter2;
extern char eruby_expr_char;
extern char eruby_comment_char;

int eruby_compile(FILE *in, FILE *out);
VALUE eruby_compile_file(char *filename);
VALUE eruby_load(char *filename, int wrap, int *state);

#endif /* ERUBY_H */

/*
 * Local variables:
 * mode: C
 * tab-width: 8
 * End:
 */
