/************************************************

  dir.c -

  $Author$
  $Date$
  created at: Wed Jan  5 09:51:01 JST 1994

  Copyright (C) 1993-1998 Yukihiro Matsumoto

************************************************/

#include "ruby.h"

#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#else
# define MAXPATHLEN 1024
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_DIRENT_H
# include <dirent.h>
# define NAMLEN(dirent) strlen((dirent)->d_name)
#elif HAVE_DIRECT_H
# include <direct.h>
# define NAMLEN(dirent) strlen((dirent)->d_name)
#else
# define dirent direct
# define NAMLEN(dirent) (dirent)->d_namlen
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif
# if HAVE_NDIR_H
#  include <ndir.h>
# endif
# if defined(NT) && defined(_MSC_VER)
#  include "missing/dir.h"
# endif
#endif

#include <errno.h>

#ifndef NT
char *getenv();
#endif

#ifdef USE_CWGUSI
# include <sys/errno.h>
#endif

VALUE rb_cDir;

static void
free_dir(dir)
    DIR *dir;
{
    if (dir) closedir(dir);
}

static VALUE dir_close _((VALUE));

static VALUE
dir_s_open(dir_class, dirname)
    VALUE dir_class, dirname;
{
    VALUE obj;
    DIR *dirp;

    Check_SafeStr(dirname);

    dirp = opendir(RSTRING(dirname)->ptr);
    if (dirp == NULL) {
	if (errno == EMFILE || errno == ENFILE) {
	    rb_gc();
	    dirp = opendir(RSTRING(dirname)->ptr);
	}
	if (dirp == NULL) {
	    rb_sys_fail(RSTRING(dirname)->ptr);
	}
    }

    obj = Data_Wrap_Struct(dir_class, 0, free_dir, dirp);

    if (rb_iterator_p()) {
	return rb_ensure(rb_yield, obj, dir_close, obj);
    }

    return obj;
}

static void
dir_closed()
{
    rb_raise(rb_eIOError, "closed directory");
}

#define GetDIR(obj, dirp) {\
    Data_Get_Struct(obj, DIR, dirp);\
    if (dirp == NULL) dir_closed();\
}

static VALUE
dir_read(dir)
    VALUE dir;
{
    DIR *dirp;
    struct dirent *dp;

    GetDIR(dir, dirp);
    errno = 0;
    dp = readdir(dirp);
    if (dp)
	return rb_tainted_str_new(dp->d_name, NAMLEN(dp));
    else if (errno == 0) {	/* end of stream */
	return Qnil;
    }
    else {
	rb_sys_fail(0);
    }
    return Qnil;		/* not reached */
}

static VALUE
dir_each(dir)
    VALUE dir;
{
    DIR *dirp;
    struct dirent *dp;
    VALUE file;

    GetDIR(dir, dirp);
    for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
	file = rb_tainted_str_new(dp->d_name, NAMLEN(dp));
	rb_yield(file);
    }
    return dir;
}

static VALUE
dir_tell(dir)
    VALUE dir;
{
    DIR *dirp;
    int pos;

#if !defined(__CYGWIN32__) && !defined(__BEOS__)
    GetDIR(dir, dirp);
    pos = telldir(dirp);
    return rb_int2inum(pos);
#else
    rb_notimplement();
#endif
}

static VALUE
dir_seek(dir, pos)
    VALUE dir, pos;
{
    DIR *dirp;

#if !defined(__CYGWIN32__) && !defined(__BEOS__)
    GetDIR(dir, dirp);
    seekdir(dirp, NUM2INT(pos));
    return dir;
#else
    rb_notimplement();
#endif
}

static VALUE
dir_rewind(dir)
    VALUE dir;
{
    DIR *dirp;

    GetDIR(dir, dirp);
    rewinddir(dirp);
    return dir;
}

static VALUE
dir_close(dir)
    VALUE dir;
{
    DIR *dirp;

    Data_Get_Struct(dir, DIR, dirp);
    if (dirp == NULL) dir_closed();
    closedir(dirp);
    DATA_PTR(dir) = NULL;

    return Qnil;
}

static VALUE
dir_s_chdir(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    VALUE path;
    char *dist = "";

    rb_secure(2);
    if (rb_scan_args(argc, argv, "01", &path) == 1) {
	Check_SafeStr(path);
	dist = RSTRING(path)->ptr;
    }
    else {
	dist = getenv("HOME");
	if (!dist) {
	    dist = getenv("LOGDIR");
	}
    }

    if (chdir(dist) < 0)
	rb_sys_fail(dist);

    return INT2FIX(0);
}

static VALUE
dir_s_getwd(dir)
    VALUE dir;
{
    char path[MAXPATHLEN];

#ifdef HAVE_GETCWD
    if (getcwd(path, sizeof(path)) == 0) rb_sys_fail(path);
#else
    extern char *getwd();
    if (getwd(path) == 0) rb_sys_fail(path);
#endif

    return rb_tainted_str_new2(path);
}

static VALUE
dir_s_chroot(dir, path)
    VALUE dir, path;
{
#if !defined(DJGPP) && !defined(NT) && !defined(__human68k__) && !defined(USE_CWGUSI) && !defined(__BEOS__)
    rb_secure(2);
    Check_SafeStr(path);

    if (chroot(RSTRING(path)->ptr) == -1)
	rb_sys_fail(RSTRING(path)->ptr);

    return INT2FIX(0);
#else
    rb_notimplement();
    return Qnil;		/* not reached */
#endif
}

static VALUE
dir_s_mkdir(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    VALUE path, vmode;
    int mode;

    rb_secure(2);
    if (rb_scan_args(argc, argv, "11", &path, &vmode) == 2) {
	mode = NUM2INT(vmode);
    }
    else {
	mode = 0777;
    }

    Check_SafeStr(path);
#if !defined(NT) && !defined(USE_CWGUSI)
    if (mkdir(RSTRING(path)->ptr, mode) == -1)
	rb_sys_fail(RSTRING(path)->ptr);
#else
    if (mkdir(RSTRING(path)->ptr) == -1)
	rb_sys_fail(RSTRING(path)->ptr);
#endif

    return INT2FIX(0);
}

static VALUE
dir_s_rmdir(obj, dir)
    VALUE obj, dir;
{
    rb_secure(2);
    Check_SafeStr(dir);
    if (rmdir(RSTRING(dir)->ptr) < 0)
	rb_sys_fail(RSTRING(dir)->ptr);

    return Qtrue;
}

#define isdelim(c) ((c)==' '||(c)=='\t'||(c)=='\n'||(c)=='\0')

char **glob_filename();
extern char *glob_error_return;

static void
push_globs(ary, s)
    VALUE ary;
    char *s;
{
    char **fnames, **ff;

    fnames = glob_filename(s);
    if (fnames == (char**)-1) rb_sys_fail(s);
    ff = fnames;
    while (*ff) {
	rb_ary_push(ary, rb_tainted_str_new2(*ff));
	free(*ff);
	ff++;
    }
    if (fnames != &glob_error_return) {
        free(fnames);
    }
}

static void
push_braces(ary, s)
    VALUE ary;
    char *s;
{
    char buf[MAXPATHLEN];
    char *p, *t, *b;
    char *lbrace, *rbrace;

    p = s;
    lbrace = rbrace = 0;
    while (*p) {
	if (*p == '{') {
	    lbrace = p;
	    break;
	}
	p++;
    }
    while (*p) {
	if (*p == '}' && lbrace) {
	    rbrace = p;
	    break;
	}
	p++;
    }

    if (lbrace) {
	memcpy(buf, s, lbrace-s);
	b = buf + (lbrace-s);
	p = lbrace;
	while (*p != '}') {
	    t = p + 1;
	    for (p = t; *p!='}' && *p!=','; p++) {
		/* skip inner braces */
		if (*p == '{') while (*p!='}') p++;
	    }
	    memcpy(b, t, p-t);
	    strcpy(b+(p-t), rbrace+1);
	    push_braces(ary, buf);
	}
    }
    else {
	push_globs(ary, s);
    }
}

static VALUE
dir_s_glob(dir, str)
    VALUE dir, str;
{
    char *p, *pend;
    char buf[MAXPATHLEN];
    char *t, *t0;
    int nest;
    VALUE ary;

    Check_SafeStr(str);
    if (RSTRING(str)->len > MAXPATHLEN) {
	rb_raise(rb_eArgError, "pathname too long (%d bytes)",
		 RSTRING(str)->len);
    }
    ary = rb_ary_new();

    p = RSTRING(str)->ptr;
    pend = p + RSTRING(str)->len;

    while (p < pend) {
	t = buf;
	while (p < pend && isdelim(*p)) p++;
	while (p < pend && !isdelim(*p)) {
	    *t++ = *p++;
	}
	*t = '\0';
	t0 = buf;
	nest = 0;
	while (t0 < t) {
	    if (*t0 == '{') nest+=2;
	    if (*t0 == '}') nest+=3;
	    t0++;
	}
	if (nest == 0) {
	    push_globs(ary, buf);
	}
	else if (nest % 5 == 0) {
	    push_braces(ary, buf);
	}
	/* else unmatched braces */
    }
    return ary;
}

static VALUE
dir_foreach(io, dirname)
    VALUE io, dirname;
{
    VALUE dir;

    dir = rb_funcall(rb_cDir, rb_intern("open"), 1, dirname);
    return rb_ensure(dir_each, dir, dir_close, dir);
}

static VALUE
dir_entries(io, dirname)
    VALUE io, dirname;
{
    VALUE dir;

    dir = rb_funcall(rb_cDir, rb_intern("open"), 1, dirname);
    return rb_ensure(rb_Array, dir, dir_close, dir);
}

void
Init_Dir()
{
    rb_cDir = rb_define_class("Dir", rb_cObject);

    rb_include_module(rb_cDir, rb_mEnumerable);

    rb_define_singleton_method(rb_cDir, "new", dir_s_open, 1);
    rb_define_singleton_method(rb_cDir, "open", dir_s_open, 1);
    rb_define_singleton_method(rb_cDir, "foreach", dir_foreach, 1);
    rb_define_singleton_method(rb_cDir, "entries", dir_entries, 1);

    rb_define_method(rb_cDir,"read", dir_read, 0);
    rb_define_method(rb_cDir,"each", dir_each, 0);
    rb_define_method(rb_cDir,"rewind", dir_rewind, 0);
    rb_define_method(rb_cDir,"tell", dir_tell, 0);
    rb_define_method(rb_cDir,"seek", dir_seek, 1);
    rb_define_method(rb_cDir,"close", dir_close, 0);

    rb_define_singleton_method(rb_cDir,"chdir", dir_s_chdir, -1);
    rb_define_singleton_method(rb_cDir,"getwd", dir_s_getwd, 0);
    rb_define_singleton_method(rb_cDir,"pwd", dir_s_getwd, 0);
    rb_define_singleton_method(rb_cDir,"chroot", dir_s_chroot, 1);
    rb_define_singleton_method(rb_cDir,"mkdir", dir_s_mkdir, -1);
    rb_define_singleton_method(rb_cDir,"rmdir", dir_s_rmdir, 1);
    rb_define_singleton_method(rb_cDir,"delete", dir_s_rmdir, 1);
    rb_define_singleton_method(rb_cDir,"unlink", dir_s_rmdir, 1);

    rb_define_singleton_method(rb_cDir,"glob", dir_s_glob, 1);
    rb_define_singleton_method(rb_cDir,"[]", dir_s_glob, 1);
}
