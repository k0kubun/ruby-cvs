/*
 * $Id$
 * Copyright (C) 1999  Network Applied Communication Laboratory, Inc.
 */

#ifndef ERUBY_H
#define ERUBY_H

#define ERUBY_MIME_TYPE "application/x-httpd-eruby"

enum eruby_compile_status {
    ERUBY_OK = 0,
    ERUBY_MISSING_END_DELIMITER,
    ERUBY_SYSTEM_ERROR
};

extern int eruby_noheader;
extern VALUE eruby_charset;
#define ERUBY_CHARSET RSTRING(eruby_charset)->ptr

int eruby_compile(FILE *in, FILE *out);
VALUE eruby_compile_file(char *filename);
VALUE eruby_load(char *filename, int wrap, int *state);
void eruby_init();

#endif /* ERUBY_H */

/*
 * Local variables:
 * mode: C
 * tab-width: 8
 * End:
 */
