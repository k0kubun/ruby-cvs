/**********************************************************************

  array.c -

  $Author$
  $Date$
  created at: Fri Aug  6 09:46:12 JST 1993

  Copyright (C) 1993-2001 Yukihiro Matsumoto
  Copyright (C) 2000  Network Applied Communication Laboratory, Inc.
  Copyright (C) 2000  Information-technology Promotion Agency, Japan

**********************************************************************/

#include "ruby.h"
#include "util.h"
#include "st.h"

VALUE rb_cArray;
static ID id_cmp;

#define ARY_DEFAULT_SIZE 16

void
rb_mem_clear(mem, size)
    register VALUE *mem;
    register long size;
{
    while (size--) {
	*mem++ = Qnil;
    }
}

static void
memfill(mem, size, val)
    register VALUE *mem;
    register long size;
    register VALUE val;
{
    while (size--) {
	*mem++ = val;
    }
}

#define ARY_TMPLOCK  FL_USER1

static void
rb_ary_modify_check(ary)
    VALUE ary;
{
    if (OBJ_FROZEN(ary)) rb_error_frozen("array");
    if (FL_TEST(ary, ARY_TMPLOCK))
	rb_raise(rb_eTypeError, "can't modify array during sort");
    if (!OBJ_TAINTED(ary) && rb_safe_level() >= 4)
	rb_raise(rb_eSecurityError, "Insecure: can't modify array");
}

static void
rb_ary_modify(ary)
    VALUE ary;
{
    VALUE *ptr;

    rb_ary_modify_check(ary);
    if (!FL_TEST(ary, ELTS_SHARED)) return;
    ptr = ALLOC_N(VALUE, RARRAY(ary)->len);
    FL_UNSET(ary, ELTS_SHARED);
    RARRAY(ary)->aux.capa = RARRAY(ary)->len;
    MEMCPY(ptr, RARRAY(ary)->ptr, VALUE, RARRAY(ary)->len);
    RARRAY(ary)->ptr = ptr;
}

VALUE
rb_ary_freeze(ary)
    VALUE ary;
{
    return rb_obj_freeze(ary);
}

static VALUE
rb_ary_frozen_p(ary)
    VALUE ary;
{
    if (FL_TEST(ary, FL_FREEZE|ARY_TMPLOCK))
	return Qtrue;
    return Qfalse;
}

static VALUE
rb_ary_s_alloc(klass)
    VALUE klass;
{
    NEWOBJ(ary, struct RArray);
    OBJSETUP(ary, klass, T_ARRAY);

    ary->len = 0;
    ary->ptr = 0;
    ary->aux.capa = 0;

    return (VALUE)ary;
}

static VALUE
ary_new(klass, len)
    VALUE klass;
    long len;
{
    VALUE ary = rb_obj_alloc(klass);

    if (len < 0) {
	rb_raise(rb_eArgError, "negative array size (or size too big)");
    }
    if (len > 0 && len*sizeof(VALUE) <= 0) {
	rb_raise(rb_eArgError, "array size too big");
    }
    if (len == 0) len++;
    RARRAY(ary)->ptr = ALLOC_N(VALUE, len);
    RARRAY(ary)->aux.capa = len;

    return ary;
}

VALUE
rb_ary_new2(len)
    long len;
{
    return ary_new(rb_cArray, len);
}


VALUE
rb_ary_new()
{
    return rb_ary_new2(ARY_DEFAULT_SIZE);
}

#ifdef HAVE_STDARG_PROTOTYPES
#include <stdarg.h>
#define va_init_list(a,b) va_start(a,b)
#else
#include <varargs.h>
#define va_init_list(a,b) va_start(a)
#endif

VALUE
#ifdef HAVE_STDARG_PROTOTYPES
rb_ary_new3(long n, ...)
#else
rb_ary_new3(n, va_alist)
    long n;
    va_dcl
#endif
{
    va_list ar;
    VALUE ary;
    long i;

    if (n < 0) {
	rb_raise(rb_eIndexError, "negative number of items(%d)", n);
    }
    ary = rb_ary_new2(n<ARY_DEFAULT_SIZE?ARY_DEFAULT_SIZE:n);

    va_init_list(ar, n);
    for (i=0; i<n; i++) {
	RARRAY(ary)->ptr[i] = va_arg(ar, VALUE);
    }
    va_end(ar);

    RARRAY(ary)->len = n;
    return ary;
}

VALUE
rb_ary_new4(n, elts)
    long n;
    VALUE *elts;
{
    VALUE ary;

    ary = rb_ary_new2(n);
    if (n > 0 && elts) {
	MEMCPY(RARRAY(ary)->ptr, elts, VALUE, n);
    }
    RARRAY(ary)->len = n;

    return ary;
}

VALUE
rb_assoc_new(car, cdr)
    VALUE car, cdr;
{
    VALUE ary;

    ary = rb_ary_new2(2);
    RARRAY(ary)->ptr[0] = car;
    RARRAY(ary)->ptr[1] = cdr;
    RARRAY(ary)->len = 2;

    return ary;
}

static VALUE
to_ary(ary)
    VALUE ary;
{
    return rb_convert_type(ary, T_ARRAY, "Array", "to_ary");
}

static VALUE
to_ary_failed(failed)
    int *failed;
{
    *failed = Qtrue;
}

static VALUE rb_ary_replace _((VALUE, VALUE));

static VALUE
rb_ary_initialize(argc, argv, ary)
    int argc;
    VALUE *argv;
    VALUE ary;
{
    long len;
    VALUE size, val;

    rb_ary_modify(ary);
    if (rb_scan_args(argc, argv, "02", &size, &val) == 0) {
	RARRAY(ary)->len = 0;
	return ary;
    }

    if (argc == 1 && !FIXNUM_P(size) && rb_respond_to(size, rb_intern("to_ary"))) {
	rb_ary_replace(ary, rb_convert_type(size, T_ARRAY, "Array", "to_ary"));
	return ary;
    }

    len = NUM2LONG(size);
    if (len < 0) {
	rb_raise(rb_eArgError, "negative array size");
    }
    if (len > 0 && len*sizeof(VALUE) <= 0) {
	rb_raise(rb_eArgError, "array size too big");
    }
    if (len > RARRAY(ary)->aux.capa) {
	RARRAY(ary)->aux.capa = len;
	REALLOC_N(RARRAY(ary)->ptr, VALUE, RARRAY(ary)->aux.capa);
    }
    memfill(RARRAY(ary)->ptr, len, val);
    RARRAY(ary)->len = len;

    return ary;
}

static VALUE
rb_ary_s_create(argc, argv, klass)
    int argc;
    VALUE *argv;
    VALUE klass;
{
    VALUE ary = rb_obj_alloc(klass);

    if (argc != 0) {
	RARRAY(ary)->ptr = ALLOC_N(VALUE, argc);
	MEMCPY(RARRAY(ary)->ptr, argv, VALUE, argc);
    }
    RARRAY(ary)->len = RARRAY(ary)->aux.capa = argc;

    return ary;
}

void
rb_ary_store(ary, idx, val)
    VALUE ary;
    long idx;
    VALUE val;
{
    rb_ary_modify(ary);
    if (idx < 0) {
	idx += RARRAY(ary)->len;
	if (idx < 0) {
	    rb_raise(rb_eIndexError, "index %d out of array",
		     idx - RARRAY(ary)->len);
	}
    }

    if (idx >= RARRAY(ary)->aux.capa) {
	long capa_inc = RARRAY(ary)->aux.capa / 2;
	if (capa_inc < ARY_DEFAULT_SIZE) {
	    capa_inc = ARY_DEFAULT_SIZE;
	}
	RARRAY(ary)->aux.capa = idx + capa_inc;
	REALLOC_N(RARRAY(ary)->ptr, VALUE, RARRAY(ary)->aux.capa);
    }
    if (idx > RARRAY(ary)->len) {
	rb_mem_clear(RARRAY(ary)->ptr+RARRAY(ary)->len,
		     idx-RARRAY(ary)->len+1);
    }

    if (idx >= RARRAY(ary)->len) {
	RARRAY(ary)->len = idx + 1;
    }
    RARRAY(ary)->ptr[idx] = val;
}

VALUE
rb_ary_push(ary, item)
    VALUE ary;
    VALUE item;
{
    rb_ary_store(ary, RARRAY(ary)->len, item);
    return ary;
}

static VALUE
rb_ary_push_m(argc, argv, ary)
    int argc;
    VALUE *argv;
    VALUE ary;
{
    if (argc == 0) {
	rb_raise(rb_eArgError, "wrong number arguments(at least 1)");
    }
    if (argc > 0) {
	while (argc--) {
	    rb_ary_push(ary, *argv++);
	}
    }
    return ary;
}

VALUE
rb_ary_pop(ary)
    VALUE ary;
{
    rb_ary_modify_check(ary);
    if (RARRAY(ary)->len == 0) return Qnil;
    if (!FL_TEST(ary, ELTS_SHARED) &&
	RARRAY(ary)->len * 10 < RARRAY(ary)->aux.capa &&
	RARRAY(ary)->aux.capa > ARY_DEFAULT_SIZE) {
	RARRAY(ary)->aux.capa = RARRAY(ary)->len * 2;
	REALLOC_N(RARRAY(ary)->ptr, VALUE, RARRAY(ary)->aux.capa);
    }
    return RARRAY(ary)->ptr[--RARRAY(ary)->len];
}

static void
ary_make_shared(ary)
    VALUE ary;
{
    if (FL_TEST(ary, ELTS_SHARED)) return;
    else {
	NEWOBJ(shared, struct RArray);
	OBJSETUP(shared, rb_cArray, T_ARRAY);

	shared->len = RARRAY(ary)->len;
	shared->ptr = RARRAY(ary)->ptr;
	shared->aux.capa = RARRAY(ary)->aux.capa;
	RARRAY(ary)->aux.shared = (VALUE)shared;
	FL_SET(ary, ELTS_SHARED);
    }
}

VALUE
rb_ary_shift(ary)
    VALUE ary;
{
    VALUE top;

    rb_ary_modify_check(ary);
    if (RARRAY(ary)->len == 0) return Qnil;
    top = RARRAY(ary)->ptr[0];
    ary_make_shared(ary);
    RARRAY(ary)->ptr++;		/* shift ptr */
    RARRAY(ary)->len--;

    return top;
}

VALUE
rb_ary_unshift(ary, item)
    VALUE ary, item;
{
    rb_ary_modify(ary);
    if (RARRAY(ary)->len >= RARRAY(ary)->aux.capa) {
	long capa_inc = RARRAY(ary)->aux.capa / 2;
	if (capa_inc < ARY_DEFAULT_SIZE) {
	    capa_inc = ARY_DEFAULT_SIZE;
	}
	RARRAY(ary)->aux.capa+=capa_inc;
	REALLOC_N(RARRAY(ary)->ptr, VALUE, RARRAY(ary)->aux.capa);
    }

    /* sliding items */
    MEMMOVE(RARRAY(ary)->ptr+1, RARRAY(ary)->ptr, VALUE, RARRAY(ary)->len);

    RARRAY(ary)->len++;
    RARRAY(ary)->ptr[0] = item;

    return ary;
}

static VALUE
rb_ary_unshift_m(argc, argv, ary)
    int argc;
    VALUE *argv;
    VALUE ary;
{
    if (argc == 0) {
	rb_raise(rb_eArgError, "wrong number of arguments(at least 1)");
    }
    if (argc > 0) {
	long len = RARRAY(ary)->len;

	/* make rooms by setting the last item */
	rb_ary_store(ary, len + argc - 1, Qnil);

	/* sliding items */
	MEMMOVE(RARRAY(ary)->ptr + argc, RARRAY(ary)->ptr, VALUE, len);
	MEMCPY(RARRAY(ary)->ptr, argv, VALUE, argc);
    }
    return ary;
}

VALUE
rb_ary_entry(ary, offset)
    VALUE ary;
    long offset;
{
    if (RARRAY(ary)->len == 0) return Qnil;

    if (offset < 0) {
	offset += RARRAY(ary)->len;
    }
    if (offset < 0 || RARRAY(ary)->len <= offset) {
	return Qnil;
    }

    return RARRAY(ary)->ptr[offset];
}

static VALUE
rb_ary_subseq(ary, beg, len)
    VALUE ary;
    long beg, len;
{
    VALUE klass, ary2;

    if (beg > RARRAY(ary)->len) return Qnil;
    if (beg < 0 || len < 0) return Qnil;

    if (beg + len > RARRAY(ary)->len) {
	len = RARRAY(ary)->len - beg;
    }
    if (len < 0) {
	len = 0;
    }
    klass = rb_obj_class(ary);
    if (len == 0) return ary_new(klass,0);

    ary_make_shared(ary);
    ary2 = rb_obj_alloc(klass);
    RARRAY(ary2)->ptr = RARRAY(ary)->ptr+beg;
    RARRAY(ary2)->len = len;
    RARRAY(ary2)->aux.shared = RARRAY(ary)->aux.shared;
    FL_SET(ary2, ELTS_SHARED);

    return ary2;
}

VALUE
rb_ary_aref(argc, argv, ary)
    int argc;
    VALUE *argv;
    VALUE ary;
{
    VALUE arg;
    long beg, len;

    if (argc == 2) {
	beg = NUM2LONG(argv[0]);
	len = NUM2LONG(argv[1]);
	if (beg < 0) {
	    beg += RARRAY(ary)->len;
	}
	return rb_ary_subseq(ary, beg, len);
    }
    if (argc != 1) {
	rb_scan_args(argc, argv, "11", 0, 0);
    }
    arg = argv[0];
    /* special case - speeding up */
    if (FIXNUM_P(arg)) {
	return rb_ary_entry(ary, FIX2LONG(arg));
    }
    else if (TYPE(arg) == T_BIGNUM) {
	rb_raise(rb_eIndexError, "index too big");
    }
    else {
	/* check if idx is Range */
	switch (rb_range_beg_len(arg, &beg, &len, RARRAY(ary)->len, 0)) {
	  case Qfalse:
	    break;
	  case Qnil:
	    return Qnil;
	  default:
	    return rb_ary_subseq(ary, beg, len);
	}
    }
    return rb_ary_entry(ary, NUM2LONG(arg));
}

static VALUE
rb_ary_at(ary, pos)
    VALUE ary, pos;
{
    return rb_ary_entry(ary, NUM2LONG(pos));
}

static VALUE
rb_ary_first(ary)
    VALUE ary;
{
    if (RARRAY(ary)->len == 0) return Qnil;
    return RARRAY(ary)->ptr[0];
}

static VALUE
rb_ary_last(ary)
    VALUE ary;
{
    if (RARRAY(ary)->len == 0) return Qnil;
    return RARRAY(ary)->ptr[RARRAY(ary)->len-1];
}

static VALUE
rb_ary_fetch(argc, argv, ary)
    int argc;
    VALUE *argv;
    VALUE ary;
{
    VALUE pos, ifnone;
    long idx;

    rb_scan_args(argc, argv, "11", &pos, &ifnone);
    idx = NUM2LONG(pos);

    if (idx < 0) {
	idx +=  RARRAY(ary)->len;
    }
    if (idx < 0 || RARRAY(ary)->len <= idx) {
	return ifnone;
    }
    return RARRAY(ary)->ptr[idx];
}

static VALUE
rb_ary_index(ary, val)
    VALUE ary;
    VALUE val;
{
    long i;

    for (i=0; i<RARRAY(ary)->len; i++) {
	if (rb_equal(RARRAY(ary)->ptr[i], val))
	    return INT2NUM(i);
    }
    return Qnil;
}

static VALUE
rb_ary_rindex(ary, val)
    VALUE ary;
    VALUE val;
{
    long i = RARRAY(ary)->len;

    while (i--) {
	if (rb_equal(RARRAY(ary)->ptr[i], val))
	    return INT2NUM(i);
    }
    return Qnil;
}

static VALUE
rb_ary_indexes(argc, argv, ary)
    int argc;
    VALUE *argv;
    VALUE ary;
{
    VALUE new_ary;
    long i;

    rb_warn("Array#%s is deprecated; use Array#select",
	    rb_id2name(rb_frame_last_func()));
    new_ary = rb_ary_new2(argc);
    for (i=0; i<argc; i++) {
	rb_ary_push(new_ary, rb_ary_aref(1, argv+i, ary));
    }

    return new_ary;
}

VALUE
rb_ary_to_ary(obj)
    VALUE obj;
{
    if (NIL_P(obj)) return rb_ary_new2(0);
    if (TYPE(obj) == T_ARRAY) {
	return obj;
    }
    if (rb_respond_to(obj, rb_intern("to_ary"))) {
	return rb_convert_type(obj, T_ARRAY, "Array", "to_ary");
    }
    return rb_ary_new3(1, obj);
}

static void
rb_ary_update(ary, beg, len, rpl)
    VALUE ary;
    long beg, len;
    VALUE rpl;
{
    long rlen;

    rpl = rb_ary_to_ary(rpl);
    rlen = RARRAY(rpl)->len;

    if (len < 0) rb_raise(rb_eIndexError, "negative length %d", len);
    if (beg < 0) {
	beg += RARRAY(ary)->len;
    }
    if (beg < 0) {
	beg -= RARRAY(ary)->len;
	rb_raise(rb_eIndexError, "index %d out of array", beg);
    }
    if (beg + len > RARRAY(ary)->len) {
	len = RARRAY(ary)->len - beg;
    }

    rb_ary_modify(ary);
    if (beg >= RARRAY(ary)->len) {
	len = beg + rlen;
	if (len >= RARRAY(ary)->aux.capa) {
	    RARRAY(ary)->aux.capa=len;
	    REALLOC_N(RARRAY(ary)->ptr, VALUE, RARRAY(ary)->aux.capa);
	}
	rb_mem_clear(RARRAY(ary)->ptr+RARRAY(ary)->len, beg-RARRAY(ary)->len);
	MEMCPY(RARRAY(ary)->ptr+beg, RARRAY(rpl)->ptr, VALUE, rlen);
	RARRAY(ary)->len = len;
    }
    else {
	long alen;

	if (beg + len > RARRAY(ary)->len) {
	    len = RARRAY(ary)->len - beg;
	}

	alen = RARRAY(ary)->len + rlen - len;
	if (alen >= RARRAY(ary)->aux.capa) {
	    RARRAY(ary)->aux.capa = alen;
	    REALLOC_N(RARRAY(ary)->ptr, VALUE, RARRAY(ary)->aux.capa);
	}

	if (len != rlen) {
	    MEMMOVE(RARRAY(ary)->ptr+beg+rlen, RARRAY(ary)->ptr+beg+len,
		    VALUE, RARRAY(ary)->len-(beg+len));
	    RARRAY(ary)->len = alen;
	}
	MEMMOVE(RARRAY(ary)->ptr+beg, RARRAY(rpl)->ptr, VALUE, rlen);
    }
}

static VALUE
rb_ary_aset(argc, argv, ary)
    int argc;
    VALUE *argv;
    VALUE ary;
{
    long offset, beg, len;

    if (argc == 3) {
	rb_ary_update(ary, NUM2LONG(argv[0]), NUM2LONG(argv[1]), argv[2]);
	return argv[2];
    }
    if (argc != 2) {
	rb_raise(rb_eArgError, "wrong number of arguments(%d for 2)", argc);
    }
    if (FIXNUM_P(argv[0])) {
	offset = FIX2LONG(argv[0]);
	goto fixnum;
    }
    else if (rb_range_beg_len(argv[0], &beg, &len, RARRAY(ary)->len, 1)) {
	/* check if idx is Range */
	rb_ary_update(ary, beg, len, argv[1]);
	return argv[1];
    }
    if (TYPE(argv[0]) == T_BIGNUM) {
	rb_raise(rb_eIndexError, "index too big");
    }

    offset = NUM2LONG(argv[0]);
  fixnum:
    rb_ary_store(ary, offset, argv[1]);
    return argv[1];
}

static VALUE
rb_ary_insert(argc, argv, ary)
    int argc;
    VALUE *argv;
    VALUE ary;
{
    long pos;

    if (argc < 2) {
	rb_raise(rb_eArgError, "wrong number of arguments(at least 2)");
    }
    pos = NUM2LONG(argv[0]);
    if (pos == -1) {
	pos = RSTRING(ary)->len;
    }
    else if (pos < 0) {
	pos++;
    }

    rb_ary_update(ary, pos, 0, rb_ary_new4(argc-1, argv+1));
    return ary;
}

VALUE
rb_ary_each(ary)
    VALUE ary;
{
    long i;

    for (i=0; i<RARRAY(ary)->len; i++) {
	rb_yield(RARRAY(ary)->ptr[i]);
    }
    return ary;
}

static VALUE
rb_ary_each_index(ary)
    VALUE ary;
{
    long i;

    for (i=0; i<RARRAY(ary)->len; i++) {
	rb_yield(INT2NUM(i));
    }
    return ary;
}

static VALUE
rb_ary_reverse_each(ary)
    VALUE ary;
{
    long len = RARRAY(ary)->len;

    while (len--) {
	rb_yield(RARRAY(ary)->ptr[len]);
    }
    return ary;
}

static VALUE
rb_ary_length(ary)
    VALUE ary;
{
    return INT2NUM(RARRAY(ary)->len);
}

static VALUE
rb_ary_empty_p(ary)
    VALUE ary;
{
    if (RARRAY(ary)->len == 0)
	return Qtrue;
    return Qfalse;
}

static VALUE
ary_copy(ary, clone)
    VALUE ary;
    int clone;
{
    VALUE copy;

    ary_make_shared(ary);
    copy = rb_obj_alloc(rb_cArray);
    if (clone) CLONESETUP(copy, ary);
    else DUPSETUP(copy, ary);
    RARRAY(copy)->ptr = RARRAY(ary)->ptr;
    RARRAY(copy)->len = RARRAY(ary)->len;
    RARRAY(copy)->aux.shared = RARRAY(ary)->aux.shared;
    FL_SET(copy, ELTS_SHARED);

    return copy;
}

static VALUE
rb_ary_clone(ary)
    VALUE ary;
{
    return ary_copy(ary, Qtrue);
}

VALUE
rb_ary_dup(ary)
    VALUE ary;
{
    return ary_copy(ary, Qfalse);
}

static VALUE
ary_dup(ary)
    VALUE ary;
{
    VALUE dup = rb_ary_new2(RARRAY(ary)->len);

    DUPSETUP(dup, ary);
    MEMCPY(RARRAY(dup)->ptr, RARRAY(ary)->ptr, VALUE, RARRAY(ary)->len);
    RARRAY(dup)->len = RARRAY(ary)->len;
    return dup;
}

extern VALUE rb_output_fs;

static VALUE
inspect_join(ary, arg)
    VALUE ary;
    VALUE *arg;
{
    return rb_ary_join(arg[0], arg[1]);
}

VALUE
rb_ary_join(ary, sep)
    VALUE ary, sep;
{
    long len, i;
    int taint = 0;
    VALUE result, tmp;

    if (RARRAY(ary)->len == 0) return rb_str_new(0, 0);
    if (OBJ_TAINTED(ary)) taint = 1;
    if (OBJ_TAINTED(sep)) taint = 1;

    len = 1;
    for (i=0; i<RARRAY(ary)->len; i++) {
	if (TYPE(RARRAY(ary)->ptr[i]) == T_STRING) {
	    len += RSTRING(RARRAY(ary)->ptr[i])->len;
	}
	else {
	    len += 10;
	}
    }
    if (!NIL_P(sep)) {
	StringValue(sep);
	len += RSTRING(sep)->len * (RARRAY(ary)->len - 1);
    }
    result = rb_str_buf_new(len);
    for (i=0; i<RARRAY(ary)->len; i++) {
	tmp = RARRAY(ary)->ptr[i];
	switch (TYPE(tmp)) {
	  case T_STRING:
	    break;
	  case T_ARRAY:
	    if (rb_inspecting_p(tmp)) {
		tmp = rb_str_new2("[...]");
	    }
	    else {
		VALUE args[2];

		args[0] = tmp;
		args[1] = sep;
		tmp = rb_protect_inspect(inspect_join, ary, (VALUE)args);
	    }
	    break;
	  default:
	    tmp = rb_obj_as_string(tmp);
	}
	if (i > 0 && !NIL_P(sep))
	    rb_str_buf_append(result, sep);
	rb_str_buf_append(result, tmp);
	if (OBJ_TAINTED(tmp)) taint = 1;
    }

    if (taint) OBJ_TAINT(result);
    return result;
}

static VALUE
rb_ary_join_m(argc, argv, ary)
    int argc;
    VALUE *argv;
    VALUE ary;
{
    VALUE sep;

    rb_scan_args(argc, argv, "01", &sep);
    if (NIL_P(sep)) sep = rb_output_fs;
    return rb_ary_join(ary, sep);
}

VALUE
rb_ary_to_s(ary)
    VALUE ary;
{
    VALUE str, sep;

    if (RARRAY(ary)->len == 0) return rb_str_new(0, 0);
    sep = rb_output_fs;
    str = rb_ary_join(ary, sep);
    return str;
}

static ID inspect_key;

struct inspect_arg {
    VALUE (*func)();
    VALUE arg1, arg2;
};

static VALUE
inspect_call(arg)
    struct inspect_arg *arg;
{
    return (*arg->func)(arg->arg1, arg->arg2);
}

static VALUE
inspect_ensure(obj)
    VALUE obj;
{
    VALUE inspect_tbl;

    inspect_tbl = rb_thread_local_aref(rb_thread_current(), inspect_key);
    rb_ary_pop(inspect_tbl);
    return 0;
}

VALUE
rb_protect_inspect(func, obj, arg)
    VALUE (*func)(ANYARGS);
    VALUE obj, arg;
{
    struct inspect_arg iarg;
    VALUE inspect_tbl;
    VALUE id;

    if (!inspect_key) {
	inspect_key = rb_intern("__inspect_key__");
    }
    inspect_tbl = rb_thread_local_aref(rb_thread_current(), inspect_key);
    if (NIL_P(inspect_tbl)) {
	inspect_tbl = rb_ary_new();
	rb_thread_local_aset(rb_thread_current(), inspect_key, inspect_tbl);
    }
    id = rb_obj_id(obj);
    if (rb_ary_includes(inspect_tbl, id)) {
	return (*func)(obj, arg);
    }
    rb_ary_push(inspect_tbl, id);
    iarg.func = func;
    iarg.arg1 = obj;
    iarg.arg2 = arg;

    return rb_ensure(inspect_call, (VALUE)&iarg, inspect_ensure, obj);
}

VALUE
rb_inspecting_p(obj)
    VALUE obj;
{
    VALUE inspect_tbl;

    if (!inspect_key) {
	inspect_key = rb_intern("__inspect_key__");
    }
    inspect_tbl = rb_thread_local_aref(rb_thread_current(), inspect_key);
    if (NIL_P(inspect_tbl)) return Qfalse;
    return rb_ary_includes(inspect_tbl, rb_obj_id(obj));
}

static VALUE
inspect_ary(ary)
    VALUE ary;
{
    int tainted = OBJ_TAINTED(ary);
    long i = 0;
    VALUE s, str;

    str = rb_str_buf_new2("[");
    for (i=0; i<RARRAY(ary)->len; i++) {
	s = rb_inspect(RARRAY(ary)->ptr[i]);
	if (OBJ_TAINTED(s)) tainted = 1;
	if (i > 0) rb_str_buf_cat2(str, ", ");
	rb_str_buf_append(str, s);
    }
    rb_str_buf_cat2(str, "]");
    if (tainted) OBJ_TAINT(str);
    return str;
}

static VALUE
rb_ary_inspect(ary)
    VALUE ary;
{
    if (RARRAY(ary)->len == 0) return rb_str_new2("[]");
    if (rb_inspecting_p(ary)) return rb_str_new2("[...]");
    return rb_protect_inspect(inspect_ary, ary, 0);
}

static VALUE
rb_ary_to_a(ary)
    VALUE ary;
{
    return ary;
}

VALUE
rb_ary_reverse(ary)
    VALUE ary;
{
    VALUE *p1, *p2;
    VALUE tmp;

    if (RARRAY(ary)->len <= 1) return ary;
    rb_ary_modify(ary);

    p1 = RARRAY(ary)->ptr;
    p2 = p1 + RARRAY(ary)->len - 1;	/* points last item */

    while (p1 < p2) {
	tmp = *p1;
	*p1++ = *p2;
	*p2-- = tmp;
    }

    return ary;
}

static VALUE
rb_ary_reverse_bang(ary)
    VALUE ary;
{
    if (RARRAY(ary)->len <= 1) return Qnil;
    return rb_ary_reverse(ary);
}

static VALUE
rb_ary_reverse_m(ary)
    VALUE ary;
{
    return rb_ary_reverse(ary_dup(ary));
}

int
rb_cmpint(cmp)
    VALUE cmp;
{
    if (FIXNUM_P(cmp)) return NUM2LONG(cmp);
    if (TYPE(cmp) == T_BIGNUM) {
	if (RBIGNUM(cmp)->sign) return 1;
	return -1;
    }
    if (rb_funcall(cmp, '>', 1, INT2FIX(0))) return 1;
    if (rb_funcall(cmp, '<', 1, INT2FIX(0))) return -1;
    return 0;
}

static int
sort_1(a, b)
    VALUE *a, *b;
{
    VALUE retval = rb_yield(rb_assoc_new(*a, *b));
    return rb_cmpint(retval);
}

static int
sort_2(a, b)
    VALUE *a, *b;
{
    VALUE retval;

    if (FIXNUM_P(*a)) {
	if (FIXNUM_P(*b)) return *a - *b;
    }
    else if (TYPE(*a) == T_STRING && TYPE(*b) == T_STRING) {
	return rb_str_cmp(*a, *b);
    }

    retval = rb_funcall(*a, id_cmp, 1, *b);
    return rb_cmpint(retval);
}

static VALUE
sort_internal(ary)
    VALUE ary;
{
    qsort(RARRAY(ary)->ptr, RARRAY(ary)->len, sizeof(VALUE),
	  rb_block_given_p()?sort_1:sort_2);
    return ary;
}

static VALUE
sort_unlock(ary)
    VALUE ary;
{
    FL_UNSET(ary, ARY_TMPLOCK);
    return ary;
}

VALUE
rb_ary_sort_bang(ary)
    VALUE ary;
{
    rb_ary_modify(ary);
    if (RARRAY(ary)->len <= 1) return ary;

    FL_SET(ary, ARY_TMPLOCK);	/* prohibit modification during sort */
    rb_ensure(sort_internal, ary, sort_unlock, ary);
    return ary;
}

VALUE
rb_ary_sort(ary)
    VALUE ary;
{
    ary = ary_dup(ary);
    rb_ary_sort_bang(ary);
    return ary;
}

static VALUE
rb_ary_collect(ary)
    VALUE ary;
{
    long len, i;
    VALUE collect;

    if (!rb_block_given_p()) {
	return rb_ary_new4(RARRAY(ary)->len, RARRAY(ary)->ptr);
    }

    len = RARRAY(ary)->len;
    collect = rb_ary_new2(len);
    for (i=0; i<len; i++) {
	rb_ary_push(collect, rb_yield(RARRAY(ary)->ptr[i]));
    }
    return collect;
}

static VALUE
rb_ary_collect_bang(ary)
    VALUE ary;
{
    long i;

    rb_ary_modify(ary);
    for (i = 0; i < RARRAY(ary)->len; i++) {
	RARRAY(ary)->ptr[i] = rb_yield(RARRAY(ary)->ptr[i]);
    }
    return ary;
}

static VALUE
rb_ary_filter(ary)
    VALUE ary;
{
    rb_warn("Array#filter is deprecated; use Array#collect!");
    return rb_ary_collect_bang(ary);
}

static VALUE
rb_ary_select(argc, argv, ary)
    int argc;
    VALUE *argv;
    VALUE ary;
{
    VALUE result = rb_ary_new();
    long i;

    if (rb_block_given_p()) {
	if (argc > 0) {
	    rb_raise(rb_eArgError, "wrong number arguments(%d for 0)", argc);
	}
	for (i = 0; i < RARRAY(ary)->len; i++) {
	    if (RTEST(rb_yield(RARRAY(ary)->ptr[i]))) {
		rb_ary_push(result, RARRAY(ary)->ptr[i]);
	    }
	}
    }
    else {
	for (i=0; i<argc; i++) {
	    rb_ary_push(result, rb_ary_entry(ary, NUM2LONG(argv[i])));
	}
    }
    return result;
}

VALUE
rb_ary_delete(ary, item)
    VALUE ary;
    VALUE item;
{
    long i1, i2;

    rb_ary_modify(ary);
    for (i1 = i2 = 0; i1 < RARRAY(ary)->len; i1++) {
	if (rb_equal(RARRAY(ary)->ptr[i1], item)) continue;
	if (i1 != i2) {
	    RARRAY(ary)->ptr[i2] = RARRAY(ary)->ptr[i1];
	}
	i2++;
    }
    if (RARRAY(ary)->len == i2) {
	if (rb_block_given_p()) {
	    return rb_yield(item);
	}
	return Qnil;
    }
    else {
	RARRAY(ary)->len = i2;
    }

    return item;
}

VALUE
rb_ary_delete_at(ary, pos)
    VALUE ary;
    long pos;
{
    long i, len = RARRAY(ary)->len;
    VALUE del = Qnil;

    rb_ary_modify(ary);
    if (pos >= len) return Qnil;
    if (pos < 0) pos += len;
    if (pos < 0) return Qnil;

    del = RARRAY(ary)->ptr[pos];
    for (i = pos + 1; i < len; i++, pos++) {
	RARRAY(ary)->ptr[pos] = RARRAY(ary)->ptr[i];
    }
    RARRAY(ary)->len = pos;

    return del;
}

static VALUE
rb_ary_delete_at_m(ary, pos)
    VALUE ary, pos;
{
    return rb_ary_delete_at(ary, NUM2LONG(pos));
}

static VALUE
rb_ary_slice_bang(argc, argv, ary)
    int argc;
    VALUE *argv;
    VALUE ary;
{
    VALUE arg1, arg2;
    long pos, len, i;

    rb_ary_modify(ary);
    if (rb_scan_args(argc, argv, "11", &arg1, &arg2) == 2) {
	pos = NUM2LONG(arg1);
	len = NUM2LONG(arg2);
      delete_pos_len:
	if (pos < 0) {
	    pos = RARRAY(ary)->len + pos;
	}
	arg2 = rb_ary_subseq(ary, pos, len);
	rb_ary_update(ary, pos, len, Qnil);	/* Qnil/rb_ary_new2(0) */
	return arg2;
    }

    if (!FIXNUM_P(arg1) && rb_range_beg_len(arg1, &pos, &len, RARRAY(ary)->len, 1)) {
	goto delete_pos_len;
    }

    pos = NUM2LONG(arg1);
    len = RARRAY(ary)->len;

    if (pos >= len) return Qnil;
    if (pos < 0) pos += len;
    if (pos < 0) return Qnil;

    arg2 = RARRAY(ary)->ptr[pos];
    for (i = pos + 1; i < len; i++, pos++) {
	RARRAY(ary)->ptr[pos] = RARRAY(ary)->ptr[i];
    }
    RARRAY(ary)->len = pos;

    return arg2;
}

static VALUE
rb_ary_reject_bang(ary)
    VALUE ary;
{
    long i1, i2;

    rb_ary_modify(ary);
    for (i1 = i2 = 0; i1 < RARRAY(ary)->len; i1++) {
	if (RTEST(rb_yield(RARRAY(ary)->ptr[i1]))) continue;
	if (i1 != i2) {
	    RARRAY(ary)->ptr[i2] = RARRAY(ary)->ptr[i1];
	}
	i2++;
    }
    if (RARRAY(ary)->len == i2) return Qnil;
    RARRAY(ary)->len = i2;

    return ary;
}

static VALUE
rb_ary_reject(ary)
    VALUE ary;
{
    ary = ary_dup(ary);
    rb_ary_reject_bang(ary);
    return ary;
}

static VALUE
rb_ary_delete_if(ary)
    VALUE ary;
{
    rb_ary_reject_bang(ary);
    return ary;
}

static VALUE
rb_ary_replace(ary, ary2)
    VALUE ary, ary2;
{
    ary2 = to_ary(ary2);
    rb_ary_update(ary, 0, RARRAY(ary)->len, ary2);
    return ary;
}

VALUE
rb_ary_clear(ary)
    VALUE ary;
{
    rb_ary_modify(ary);
    RARRAY(ary)->len = 0;
    if (ARY_DEFAULT_SIZE*3 < RARRAY(ary)->aux.capa) {
	RARRAY(ary)->aux.capa = ARY_DEFAULT_SIZE * 2;
	REALLOC_N(RARRAY(ary)->ptr, VALUE, RARRAY(ary)->aux.capa);
    }
    return ary;
}

static VALUE
rb_ary_fill(argc, argv, ary)
    int argc;
    VALUE *argv;
    VALUE ary;
{
    VALUE item, arg1, arg2;
    long beg, end, len;
    VALUE *p, *pend;

    rb_scan_args(argc, argv, "12", &item, &arg1, &arg2);
    switch (argc) {
      case 1:
	beg = 0;
	len = RARRAY(ary)->len - beg;
	break;
      case 2:
	if (rb_range_beg_len(arg1, &beg, &len, RARRAY(ary)->len, 1)) {
	    break;
	}
	/* fall through */
      case 3:
	beg = NIL_P(arg1)?0:NUM2LONG(arg1);
	if (beg < 0) {
	    beg = RARRAY(ary)->len + beg;
	    if (beg < 0) beg = 0;
	}
	len = NIL_P(arg2)?RARRAY(ary)->len - beg:NUM2LONG(arg2);
	break;
    }
    rb_ary_modify(ary);
    end = beg + len;
    if (end > RARRAY(ary)->len) {
	if (end >= RARRAY(ary)->aux.capa) {
	    RARRAY(ary)->aux.capa = end;
	    REALLOC_N(RARRAY(ary)->ptr, VALUE, RARRAY(ary)->aux.capa);
	}
	if (beg > RARRAY(ary)->len) {
	    rb_mem_clear(RARRAY(ary)->ptr+RARRAY(ary)->len,end-RARRAY(ary)->len);
	}
	RARRAY(ary)->len = end;
    }
    p = RARRAY(ary)->ptr + beg; pend = p + len;

    while (p < pend) {
	*p++ = item;
    }
    return ary;
}

VALUE
rb_ary_plus(x, y)
    VALUE x, y;
{
    VALUE z;

    y = to_ary(y);
    z = rb_ary_new2(RARRAY(x)->len + RARRAY(y)->len);
    MEMCPY(RARRAY(z)->ptr, RARRAY(x)->ptr, VALUE, RARRAY(x)->len);
    MEMCPY(RARRAY(z)->ptr+RARRAY(x)->len, RARRAY(y)->ptr, VALUE, RARRAY(y)->len);
    RARRAY(z)->len = RARRAY(x)->len + RARRAY(y)->len;
    return z;
}

VALUE
rb_ary_concat(x, y)
    VALUE x, y;
{
    y = to_ary(y);
    if (RARRAY(y)->len > 0) {
	rb_ary_update(x, RARRAY(x)->len, 0, y);
    }
    return x;
}

static VALUE
rb_ary_times(ary, times)
    VALUE ary;
    VALUE times;
{
    VALUE ary2;
    long i, len;

    if (TYPE(times) == T_STRING) {
	return rb_ary_join(ary, times);
    }

    len = NUM2LONG(times);
    if (len < 0) {
	rb_raise(rb_eArgError, "negative argument");
    }
    len *= RARRAY(ary)->len;

    ary2 = ary_new(rb_obj_class(ary), len);
    RARRAY(ary2)->len = len;

    for (i=0; i<len; i+=RARRAY(ary)->len) {
	MEMCPY(RARRAY(ary2)->ptr+i, RARRAY(ary)->ptr, VALUE, RARRAY(ary)->len);
    }
    OBJ_INFECT(ary2, ary);

    return ary2;
}

VALUE
rb_ary_assoc(ary, key)
    VALUE ary;
    VALUE key;
{
    VALUE *p, *pend;

    p = RARRAY(ary)->ptr; pend = p + RARRAY(ary)->len;
    while (p < pend) {
	if (TYPE(*p) == T_ARRAY
	    && RARRAY(*p)->len > 0
	    && rb_equal(RARRAY(*p)->ptr[0], key))
	    return *p;
	p++;
    }
    return Qnil;
}

VALUE
rb_ary_rassoc(ary, value)
    VALUE ary;
    VALUE value;
{
    VALUE *p, *pend;

    p = RARRAY(ary)->ptr; pend = p + RARRAY(ary)->len;
    while (p < pend) {
	if (TYPE(*p) == T_ARRAY
	    && RARRAY(*p)->len > 1
	    && rb_equal(RARRAY(*p)->ptr[1], value))
	    return *p;
	p++;
    }
    return Qnil;
}

static VALUE
rb_ary_equal(ary1, ary2)
    VALUE ary1, ary2;
{
    long i;

    if (ary1 == ary2) return Qtrue;
    if (TYPE(ary2) != T_ARRAY) return Qfalse;
    if (RARRAY(ary1)->len != RARRAY(ary2)->len) return Qfalse;
    for (i=0; i<RARRAY(ary1)->len; i++) {
	if (!rb_equal(RARRAY(ary1)->ptr[i], RARRAY(ary2)->ptr[i]))
	    return Qfalse;
    }
    return Qtrue;
}

static VALUE
rb_ary_eql(ary1, ary2)
    VALUE ary1, ary2;
{
    long i;

    if (TYPE(ary2) != T_ARRAY) return Qfalse;
    if (RARRAY(ary1)->len != RARRAY(ary2)->len) return Qfalse;
    for (i=0; i<RARRAY(ary1)->len; i++) {
	if (!rb_eql(RARRAY(ary1)->ptr[i], RARRAY(ary2)->ptr[i]))
	    return Qfalse;
    }
    return Qtrue;
}

static VALUE
rb_ary_hash(ary)
    VALUE ary;
{
    long i;
    VALUE n;
    long h;

    h = RARRAY(ary)->len;
    for (i=0; i<RARRAY(ary)->len; i++) {
	h = (h<<1) | (h<0 ? 1 : 0);
	n = rb_hash(RARRAY(ary)->ptr[i]);
	h ^= NUM2LONG(n);
    }
    return INT2FIX(h);
}

VALUE
rb_ary_includes(ary, item)
    VALUE ary;
    VALUE item;
{
    long i;
    for (i=0; i<RARRAY(ary)->len; i++) {
	if (rb_equal(RARRAY(ary)->ptr[i], item)) {
	    return Qtrue;
	}
    }
    return Qfalse;
}

VALUE
rb_ary_cmp(ary, ary2)
    VALUE ary;
    VALUE ary2;
{
    long i, len;

    if (TYPE(ary2) != T_ARRAY) {
	ary2 = to_ary(ary2);
    }
    len = RARRAY(ary)->len;
    if (len > RARRAY(ary2)->len) {
	len = RARRAY(ary2)->len;
    }
    for (i=0; i<len; i++) {
	VALUE v = rb_funcall(RARRAY(ary)->ptr[i],id_cmp,1,RARRAY(ary2)->ptr[i]);
	if (v != INT2FIX(0)) {
	    return v;
	}
    }
    len = RARRAY(ary)->len - RARRAY(ary2)->len;
    if (len == 0) return INT2FIX(0);
    if (len > 0) return INT2FIX(1);
    return INT2FIX(-1);
}

static VALUE
rb_ary_diff(ary1, ary2)
    VALUE ary1, ary2;
{
    VALUE ary3;
    long i;

    ary2 = to_ary(ary2);
    ary3 = rb_ary_new();
    for (i=0; i<RARRAY(ary1)->len; i++) {
	if (rb_ary_includes(ary2, RARRAY(ary1)->ptr[i])) continue;
	if (rb_ary_includes(ary3, RARRAY(ary1)->ptr[i])) continue;
	rb_ary_push(ary3, RARRAY(ary1)->ptr[i]);
    }
    return ary3;
}

static VALUE
ary_make_hash(ary1, ary2)
    VALUE ary1, ary2;
{
    VALUE hash = rb_hash_new();
    int i;

    for (i=0; i<RARRAY(ary1)->len; i++) {
	rb_hash_aset(hash, RARRAY(ary1)->ptr[i], Qtrue);
    }
    if (ary2) {
	for (i=0; i<RARRAY(ary2)->len; i++) {
	    rb_hash_aset(hash, RARRAY(ary2)->ptr[i], Qtrue);
	}
    }
    return hash;
}

static VALUE
rb_ary_and(ary1, ary2)
    VALUE ary1, ary2;
{
    VALUE hash;
    VALUE ary3 = rb_ary_new();
    long i;

    ary2 = to_ary(ary2);
    hash = ary_make_hash(ary2, 0);

    for (i=0; i<RARRAY(ary1)->len; i++) {
	VALUE v = RARRAY(ary1)->ptr[i];
	if (st_delete(RHASH(hash)->tbl, &v, 0)) {
	    rb_ary_push(ary3, RARRAY(ary1)->ptr[i]);
	}
    }

    return ary3;
}

static VALUE
rb_ary_or(ary1, ary2)
    VALUE ary1, ary2;
{
    VALUE hash;
    VALUE ary3 = rb_ary_new();
    VALUE v;
    long i;

    ary2 = to_ary(ary2);
    hash = ary_make_hash(ary1, ary2);

    for (i=0; i<RARRAY(ary1)->len; i++) {
	v = RARRAY(ary1)->ptr[i];
	if (st_delete(RHASH(hash)->tbl, &v, 0)) {
	    rb_ary_push(ary3, RARRAY(ary1)->ptr[i]);
	}
    }
    for (i=0; i<RARRAY(ary2)->len; i++) {
	v = RARRAY(ary2)->ptr[i];
	if (st_delete(RHASH(hash)->tbl, &v, 0)) {
	    rb_ary_push(ary3, RARRAY(ary2)->ptr[i]);
	}
    }

    return ary3;
}

static VALUE
rb_ary_uniq_bang(ary)
    VALUE ary;
{
    VALUE hash = ary_make_hash(ary, 0);
    VALUE *p, *q, *end;

    if (RARRAY(ary)->len == RHASH(hash)->tbl->num_entries) {
	return Qnil;
    }

    rb_ary_modify(ary);
    p = q = RARRAY(ary)->ptr;
    end = p + RARRAY(ary)->len;
    while (p < end) {
	VALUE v = *p;
	if (st_delete(RHASH(hash)->tbl, &v, 0)) {
	    *q++ = *p;
	}
	p++;
    }
    RARRAY(ary)->len = (q - RARRAY(ary)->ptr);

    return ary;
}

static VALUE
rb_ary_uniq(ary)
    VALUE ary;
{
    ary = ary_dup(ary);
    rb_ary_uniq_bang(ary);
    return ary;
}

static VALUE
rb_ary_compact_bang(ary)
    VALUE ary;
{
    VALUE *p, *t, *end;

    rb_ary_modify(ary);
    p = t = RARRAY(ary)->ptr;
    end = p + RARRAY(ary)->len;
    while (t < end) {
	if (NIL_P(*t)) t++;
	else *p++ = *t++;
    }
    if (RARRAY(ary)->len == (p - RARRAY(ary)->ptr)) {
	return Qnil;
    }
    RARRAY(ary)->len = RARRAY(ary)->aux.capa = (p - RARRAY(ary)->ptr);
    REALLOC_N(RARRAY(ary)->ptr, VALUE, RARRAY(ary)->len);

    return ary;
}

static VALUE
rb_ary_compact(ary)
    VALUE ary;
{
    ary = ary_dup(ary);
    rb_ary_compact_bang(ary);
    return ary;
}

static VALUE
rb_ary_nitems(ary)
    VALUE ary;
{
    long n = 0;
    VALUE *p, *pend;

    p = RARRAY(ary)->ptr;
    pend = p + RARRAY(ary)->len;
    while (p < pend) {
	if (!NIL_P(*p)) n++;
	p++;
    }
    return INT2NUM(n);
}

static int
flatten(ary, idx, ary2, memo)
    VALUE ary;
    long idx;
    VALUE ary2, memo;
{
    VALUE id;
    long i = idx;
    long n, lim = idx + RARRAY(ary2)->len;

    id = rb_obj_id(ary2);
    if (rb_ary_includes(memo, id)) {
	rb_raise(rb_eArgError, "tried to flatten recursive array");
    }
    rb_ary_push(memo, id);
    rb_ary_update(ary, idx, 1, ary2);
    while (i < lim) {
	if (TYPE(RARRAY(ary)->ptr[i]) == T_ARRAY) {
	    n = flatten(ary, i, RARRAY(ary)->ptr[i], memo);
	    i += n; lim += n;
	}
	i++;
    }
    rb_ary_pop(memo);

    return lim - idx - 1;	/* returns number of increased items */
}

static VALUE
rb_ary_flatten_bang(ary)
    VALUE ary;
{
    long i = 0;
    int mod = 0;
    VALUE memo = Qnil;

    rb_ary_modify(ary);
    while (i<RARRAY(ary)->len) {
	VALUE ary2 = RARRAY(ary)->ptr[i];

	if (TYPE(ary2) == T_ARRAY) {
	    if (NIL_P(memo)) {
		memo = rb_ary_new();
	    }
	    i += flatten(ary, i, ary2, memo);
	    mod = 1;
	}
	i++;
    }
    if (mod == 0) return Qnil;
    return ary;
}

static VALUE
rb_ary_flatten(ary)
    VALUE ary;
{
    ary = ary_dup(ary);
    rb_ary_flatten_bang(ary);
    return ary;
}

void
Init_Array()
{
    rb_cArray  = rb_define_class("Array", rb_cObject);
    rb_include_module(rb_cArray, rb_mEnumerable);

    rb_define_singleton_method(rb_cArray, "allocate", rb_ary_s_alloc, 0);
    rb_define_singleton_method(rb_cArray, "[]", rb_ary_s_create, -1);
    rb_define_method(rb_cArray, "initialize", rb_ary_initialize, -1);
    rb_define_method(rb_cArray, "to_s", rb_ary_to_s, 0);
    rb_define_method(rb_cArray, "inspect", rb_ary_inspect, 0);
    rb_define_method(rb_cArray, "to_a", rb_ary_to_a, 0);
    rb_define_method(rb_cArray, "to_ary", rb_ary_to_a, 0);
    rb_define_method(rb_cArray, "frozen?",  rb_ary_frozen_p, 0);

    rb_define_method(rb_cArray, "==", rb_ary_equal, 1);
    rb_define_method(rb_cArray, "eql?", rb_ary_eql, 1);
    rb_define_method(rb_cArray, "hash", rb_ary_hash, 0);
    rb_define_method(rb_cArray, "===", rb_ary_equal, 1);

    rb_define_method(rb_cArray, "[]", rb_ary_aref, -1);
    rb_define_method(rb_cArray, "[]=", rb_ary_aset, -1);
    rb_define_method(rb_cArray, "at", rb_ary_at, 1);
    rb_define_method(rb_cArray, "fetch", rb_ary_fetch, -1);
    rb_define_method(rb_cArray, "first", rb_ary_first, 0);
    rb_define_method(rb_cArray, "last", rb_ary_last, 0);
    rb_define_method(rb_cArray, "concat", rb_ary_concat, 1);
    rb_define_method(rb_cArray, "<<", rb_ary_push, 1);
    rb_define_method(rb_cArray, "push", rb_ary_push_m, -1);
    rb_define_method(rb_cArray, "pop", rb_ary_pop, 0);
    rb_define_method(rb_cArray, "shift", rb_ary_shift, 0);
    rb_define_method(rb_cArray, "unshift", rb_ary_unshift_m, -1);
    rb_define_method(rb_cArray, "insert", rb_ary_insert, -1);
    rb_define_method(rb_cArray, "each", rb_ary_each, 0);
    rb_define_method(rb_cArray, "each_index", rb_ary_each_index, 0);
    rb_define_method(rb_cArray, "reverse_each", rb_ary_reverse_each, 0);
    rb_define_method(rb_cArray, "length", rb_ary_length, 0);
    rb_define_alias(rb_cArray,  "size", "length");
    rb_define_method(rb_cArray, "empty?", rb_ary_empty_p, 0);
    rb_define_method(rb_cArray, "index", rb_ary_index, 1);
    rb_define_method(rb_cArray, "rindex", rb_ary_rindex, 1);
    rb_define_method(rb_cArray, "indexes", rb_ary_indexes, -1);
    rb_define_method(rb_cArray, "indices", rb_ary_indexes, -1);
    rb_define_method(rb_cArray, "clone", rb_ary_clone, 0);
    rb_define_method(rb_cArray, "dup", rb_ary_dup, 0);
    rb_define_method(rb_cArray, "join", rb_ary_join_m, -1);
    rb_define_method(rb_cArray, "reverse", rb_ary_reverse_m, 0);
    rb_define_method(rb_cArray, "reverse!", rb_ary_reverse_bang, 0);
    rb_define_method(rb_cArray, "sort", rb_ary_sort, 0);
    rb_define_method(rb_cArray, "sort!", rb_ary_sort_bang, 0);
    rb_define_method(rb_cArray, "collect", rb_ary_collect, 0);
    rb_define_method(rb_cArray, "collect!", rb_ary_collect_bang, 0);
    rb_define_method(rb_cArray, "select", rb_ary_select, -1);
    rb_define_method(rb_cArray, "map", rb_ary_collect, 0);
    rb_define_method(rb_cArray, "map!", rb_ary_collect_bang, 0);
    rb_define_method(rb_cArray, "filter", rb_ary_filter, 0);
    rb_define_method(rb_cArray, "delete", rb_ary_delete, 1);
    rb_define_method(rb_cArray, "delete_at", rb_ary_delete_at_m, 1);
    rb_define_method(rb_cArray, "delete_if", rb_ary_delete_if, 0);
    rb_define_method(rb_cArray, "reject", rb_ary_reject, 0);
    rb_define_method(rb_cArray, "reject!", rb_ary_reject_bang, 0);
    rb_define_method(rb_cArray, "replace", rb_ary_replace, 1);
    rb_define_method(rb_cArray, "clear", rb_ary_clear, 0);
    rb_define_method(rb_cArray, "fill", rb_ary_fill, -1);
    rb_define_method(rb_cArray, "include?", rb_ary_includes, 1);
    rb_define_method(rb_cArray, "<=>", rb_ary_cmp, 1);

    rb_define_method(rb_cArray, "slice", rb_ary_aref, -1);
    rb_define_method(rb_cArray, "slice!", rb_ary_slice_bang, -1);

    rb_define_method(rb_cArray, "assoc", rb_ary_assoc, 1);
    rb_define_method(rb_cArray, "rassoc", rb_ary_rassoc, 1);

    rb_define_method(rb_cArray, "+", rb_ary_plus, 1);
    rb_define_method(rb_cArray, "*", rb_ary_times, 1);

    rb_define_method(rb_cArray, "-", rb_ary_diff, 1);
    rb_define_method(rb_cArray, "&", rb_ary_and, 1);
    rb_define_method(rb_cArray, "|", rb_ary_or, 1);

    rb_define_method(rb_cArray, "uniq", rb_ary_uniq, 0);
    rb_define_method(rb_cArray, "uniq!", rb_ary_uniq_bang, 0);
    rb_define_method(rb_cArray, "compact", rb_ary_compact, 0);
    rb_define_method(rb_cArray, "compact!", rb_ary_compact_bang, 0);
    rb_define_method(rb_cArray, "flatten", rb_ary_flatten, 0);
    rb_define_method(rb_cArray, "flatten!", rb_ary_flatten_bang, 0);
    rb_define_method(rb_cArray, "nitems", rb_ary_nitems, 0);

    id_cmp = rb_intern("<=>");
}
