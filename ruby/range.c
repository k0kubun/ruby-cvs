/**********************************************************************

  range.c -

  $Author$
  $Date$
  created at: Thu Aug 19 17:46:47 JST 1993

  Copyright (C) 1993-2002 Yukihiro Matsumoto

**********************************************************************/

#include "ruby.h"

VALUE rb_cRange;
static ID id_cmp, id_succ, id_beg, id_end, id_excl;

#define EXCL(r) RTEST(rb_ivar_get((r), id_excl))
#define SET_EXCL(r,v) rb_ivar_set((r), id_excl, (v)?Qtrue:Qfalse)

static VALUE
range_check(args)
    VALUE *args;
{
    rb_funcall(args[0], id_cmp, 1, args[1]);
    if (!FIXNUM_P(args[0]) && !rb_obj_is_kind_of(args[0], rb_cNumeric)) {
	rb_funcall(args[0], id_succ, 0, 0);
    }
    return Qnil;
}

static VALUE
range_failed()
{
    rb_raise(rb_eArgError, "bad value for range");
    return Qnil;		/* dummy */
}

static void
range_init(obj, beg, end, exclude_end)
    VALUE obj, beg, end;
    int exclude_end;
{
    VALUE args[2];

    args[0] = beg; args[1] = end;
    if (!FIXNUM_P(beg) || !FIXNUM_P(end)) {
	rb_rescue(range_check, (VALUE)args, range_failed, 0);
    }

    SET_EXCL(obj, exclude_end);
    rb_ivar_set(obj, id_beg, beg);
    rb_ivar_set(obj, id_end, end);
}

VALUE
rb_range_new(beg, end, exclude_end)
    VALUE beg, end;
    int exclude_end;
{
    VALUE obj = rb_obj_alloc(rb_cRange);

    range_init(obj, beg, end, exclude_end);
    return obj;
}

static VALUE
range_initialize(argc, argv, obj)
    int argc;
    VALUE *argv;
    VALUE obj;
{
    VALUE beg, end, flags;
    
    rb_scan_args(argc, argv, "21", &beg, &end, &flags);
    /* Ranges are immutable, so that they should be initialized only once. */
    if (rb_ivar_defined(obj, id_beg)) {
	rb_name_error(rb_intern("initialized"), "`initialize' called twice");
    }
    range_init(obj, beg, end, RTEST(flags));
    return Qnil;
}

static VALUE
range_exclude_end_p(range)
    VALUE range;
{
    return EXCL(range)?Qtrue:Qfalse;
}

static VALUE
range_eq(range, obj)
    VALUE range, obj;
{
    if (range == obj) return Qtrue;
    if (!rb_obj_is_kind_of(obj, rb_cRange)) return Qfalse;

    if (!rb_equal(rb_ivar_get(range, id_beg), rb_ivar_get(obj, id_beg)))
	return Qfalse;
    if (!rb_equal(rb_ivar_get(range, id_end), rb_ivar_get(obj, id_end)))
	return Qfalse;

    if (EXCL(range) != EXCL(obj)) return Qfalse;

    return Qtrue;
}

static int
r_eq(a, b)
    VALUE a, b;
{
    if (a == b) return Qtrue;

    if (rb_funcall(a, id_cmp, 1, b) == INT2FIX(0))
	return Qtrue;
    return Qfalse;
}

static int
r_lt(a, b)
    VALUE a, b;
{
    VALUE r = rb_funcall(a, id_cmp, 1, b);

    if (rb_cmpint(r) < 0) return Qtrue;
    return Qfalse;
}

static int
r_le(a, b)
    VALUE a, b;
{
    VALUE r = rb_funcall(a, id_cmp, 1, b);

    if (rb_cmpint(r) <= 0) return Qtrue;
    return Qfalse;
}

static int
r_gt(a,b)
    VALUE a, b;
{
    VALUE r = rb_funcall(a, id_cmp, 1, b);

    if (rb_cmpint(r) > 0) return Qtrue;
    return Qfalse;
}

static VALUE
r_eqq_str_i(i, data)
    VALUE i;
    VALUE *data;
{
    if (rb_str_cmp(i, data[0]) == 0) {
	data[1] = Qtrue;
	rb_iter_break();
    }
    return Qnil;
}

static VALUE
range_eqq(range, obj)
    VALUE range, obj;
{
    VALUE beg, end;

    beg = rb_ivar_get(range, id_beg);
    end = rb_ivar_get(range, id_end);

    if (FIXNUM_P(beg) && FIXNUM_P(obj) && FIXNUM_P(end)) {
	if (FIX2LONG(beg) <= FIX2LONG(obj)) {
	    if (EXCL(range)) {
		if (FIX2LONG(obj) < FIX2LONG(end)) return Qtrue;
	    }
	    else {
		if (FIX2LONG(obj) <= FIX2LONG(end)) return Qtrue;
	    }
	}
	return Qfalse;
    }
    else if (TYPE(beg) == T_STRING &&
	     TYPE(obj) == T_STRING &&
	     TYPE(end) == T_STRING) {
	VALUE data[2];

	data[0] = obj; data[1] = Qfalse;
	rb_iterate(rb_each, range, r_eqq_str_i, (VALUE)data);
	return data[1];
    }
    else if (r_le(beg, obj)) {
	if (EXCL(range)) {
	    if (r_lt(obj, end)) return Qtrue;
	}
	else {
	    if (r_le(obj, end)) return Qtrue;
	}
    }
    return Qfalse;
}

static VALUE
range_eql(range, obj)
    VALUE range, obj;
{
    if (range == obj) return Qtrue;
    if (!rb_obj_is_kind_of(obj, rb_cRange)) return Qfalse;

    if (!rb_eql(rb_ivar_get(range, id_beg), rb_ivar_get(obj, id_beg)))
	return Qfalse;
    if (!rb_eql(rb_ivar_get(range, id_end), rb_ivar_get(obj, id_end)))
	return Qfalse;

    if (EXCL(range) != EXCL(obj)) return Qfalse;

    return Qtrue;
}

static VALUE
range_hash(range, obj)
    VALUE range, obj;
{
    long hash = EXCL(range);
    VALUE v;

    v = rb_hash(rb_ivar_get(range, id_beg));
    hash ^= v << 1;
    v = rb_hash(rb_ivar_get(range, id_end));
    hash ^= v << 9;

    return INT2FIX(hash);
}

static VALUE
range_each(range)
    VALUE range;
{
    VALUE b, e;

    b = rb_ivar_get(range, id_beg);
    e = rb_ivar_get(range, id_end);

    if (FIXNUM_P(b) && FIXNUM_P(e)) { /* fixnums are special */
	long end = FIX2LONG(e);
	long i;

	if (!EXCL(range)) end += 1;
	for (i=FIX2LONG(b); i<end; i++) {
	    rb_yield(INT2NUM(i));
	}
    }
    else if (TYPE(b) == T_STRING) {
	rb_str_upto(b, e, EXCL(range));
    }
    else if (rb_obj_is_kind_of(b, rb_cNumeric)) {
	b = rb_Integer(b);
	e = rb_Integer(e);

	if (!EXCL(range)) e = rb_funcall(e, '+', 1, INT2FIX(1));
	while (RTEST(rb_funcall(b, '<', 1, e))) {
	    rb_yield(b);
	    b = rb_funcall(b, '+', 1, INT2FIX(1));
	}
    }
    else {			/* generic each */
	VALUE v = b;

	if (EXCL(range)) {
	    while (r_lt(v, e)) {
		if (r_eq(v, e)) break;
		rb_yield(v);
		v = rb_funcall(v, id_succ, 0, 0);
	    }
	}
	else {
	    while (r_le(v, e)) {
		rb_yield(v);
		if (r_eq(v, e)) break;
		v = rb_funcall(v, id_succ, 0, 0);
	    }
	}
    }

    return range;
}

static VALUE
r_step_str(args)
    VALUE *args;
{
    return rb_str_upto(args[0], args[1], EXCL(args[2]));
}

static VALUE
r_step_str_i(i, iter)
    VALUE i;
    long *iter;
{
    iter[0]--;
    if (iter[0] == 0) {
	rb_yield(i);
	iter[0] = iter[1];
    }
    return Qnil;
}

static VALUE
range_step(argc, argv, range)
    int argc;
    VALUE *argv;
    VALUE range;
{
    VALUE b, e, step;

    b = rb_ivar_get(range, id_beg);
    e = rb_ivar_get(range, id_end);
    rb_scan_args(argc, argv, "01", &step);

    if (FIXNUM_P(b) && FIXNUM_P(e)) { /* fixnums are special */
	long end = FIX2LONG(e);
	long i, s = (argc == 0) ? 1 : NUM2LONG(step);

	if (!EXCL(range)) end += 1;
	for (i=FIX2LONG(b); i<end; i+=s) {
	    rb_yield(INT2NUM(i));
	}
    }
    else if (rb_obj_is_kind_of(b, rb_cNumeric)) {
	b = rb_Integer(b);
	e = rb_Integer(e);
	step = rb_Integer(step);

	if (!EXCL(range)) e = rb_funcall(e, '+', 1, INT2FIX(1));
	while (RTEST(rb_funcall(b, '<', 1, e))) {
	    rb_yield(b);
	    b = rb_funcall(b, '+', 1, step);
	}
    }
    else if (TYPE(b) == T_STRING) {
	VALUE args[5];
	long iter[2];

	args[0] = b; args[1] = e; args[2] = range;
	iter[0] = 1; iter[1] = NUM2LONG(step);
	rb_iterate((VALUE(*)_((VALUE)))r_step_str, (VALUE)args, r_step_str_i,
		   (VALUE)iter);
    }
    else {			/* generic each */
	VALUE v = b;
	long lim = NUM2INT(step);
	long i;

	if (EXCL(range)) {
	    while (r_lt(v, e)) {
		if (r_eq(v, e)) break;
		rb_yield(v);
		for (i=0; i<lim; i++) 
		    v = rb_funcall(v, id_succ, 0, 0);
	    }
	}
	else {
	    while (r_le(v, e)) {
		rb_yield(v);
		if (r_eq(v, e)) break;
		for (i=0; i<lim; i++) 
		    v = rb_funcall(v, id_succ, 0, 0);
	    }
	}
    }

    return range;
}

static VALUE
range_first(obj)
    VALUE obj;
{
    VALUE b;

    b = rb_ivar_get(obj, id_beg);
    return b;
}

static VALUE
range_last(obj)
    VALUE obj;
{
    VALUE e;

    e = rb_ivar_get(obj, id_end);
    return e;
}

VALUE
rb_range_beg_len(range, begp, lenp, len, err)
    VALUE range;
    long *begp, *lenp;
    long len;
    int err;
{
    long beg, end, b, e;

    if (!rb_obj_is_kind_of(range, rb_cRange)) return Qfalse;

    beg = b = NUM2LONG(rb_ivar_get(range, id_beg));
    end = e = NUM2LONG(rb_ivar_get(range, id_end));

    if (beg < 0) {
	beg += len;
	if (beg < 0) goto out_of_range;
    }
    if (err == 0 || err == 2) {
	if (beg > len) goto out_of_range;
	if (end > len || (!EXCL(range) && end == len))
	    end = len;
    }
    if (end < 0) {
	end += len;
	if (end < 0) {
	    if (beg == 0 && end == -1 && !EXCL(range)) {
		len = 0;
		goto length_set;
	    }
	    goto out_of_range;
	}
    }
    len = end - beg;
    if (!EXCL(range)) len++;	/* include end point */
    if (len < 0) goto out_of_range;

  length_set:
    *begp = beg;
    *lenp = len;

    return Qtrue;

  out_of_range:
    if (err) {
	rb_raise(rb_eRangeError, "%d..%s%d out of range",
		 b, EXCL(range)? "." : "", e);
    }
    return Qnil;
}

static VALUE
range_to_s(range)
    VALUE range;
{
    VALUE str, str2;

    str = rb_obj_as_string(rb_ivar_get(range, id_beg));
    str2 = rb_obj_as_string(rb_ivar_get(range, id_end));
    str = rb_str_dup(str);
    rb_str_cat(str, "...", EXCL(range)?3:2);
    rb_str_append(str, str2);
    OBJ_INFECT(str, str2);

    return str;
}

static VALUE
range_inspect(range)
    VALUE range;
{
    VALUE str, str2;

    str = rb_inspect(rb_ivar_get(range, id_beg));
    str2 = rb_inspect(rb_ivar_get(range, id_end));
    str = rb_str_dup(str);
    rb_str_cat(str, "...", EXCL(range)?3:2);
    rb_str_append(str, str2);
    OBJ_INFECT(str, str2);

    return str;
}

static VALUE
length_i(i, length)
    VALUE i;
    int *length;
{
    (*length)++;
    return Qnil;
}

VALUE
rb_length_by_each(obj)
    VALUE obj;
{
    int length = 0;

    rb_iterate(rb_each, obj, length_i, (VALUE)&length);
    return INT2FIX(length);
}

static VALUE
range_length(range)
    VALUE range;
{
    VALUE beg, end;
    long size;

    beg = rb_ivar_get(range, id_beg);
    end = rb_ivar_get(range, id_end);

    if (r_gt(beg, end)) {
	return INT2FIX(0);
    }
    if (FIXNUM_P(beg) && FIXNUM_P(end)) {
	if (EXCL(range)) {
	    return INT2NUM(NUM2LONG(end) - NUM2LONG(beg));
	}
	else {
	    return INT2NUM(NUM2LONG(end) - NUM2LONG(beg) + 1);
	}
    }
    if (!rb_obj_is_kind_of(beg, rb_cInteger)) {
	return rb_length_by_each(range);
    }
    size = rb_funcall(end, '-', 1, beg);
    if (!EXCL(range)) {
	size = rb_funcall(size, '+', 1, INT2FIX(1));
    }
    if (TYPE(size) == T_FLOAT) {
	size = rb_funcall(size, rb_intern("floor"), 0);
    }

    return size;
}

static VALUE
range_member(range, val)
    VALUE range, val;
{
    VALUE beg, end;

    beg = rb_ivar_get(range, id_beg);
    end = rb_ivar_get(range, id_end);

    if (r_gt(beg, val)) return Qfalse;
    if (EXCL(range)) {
	if (r_lt(val, end)) return Qtrue;
    }
    else {
	if (r_le(val, end)) return Qtrue;
    }
    return Qfalse;
}

void
Init_Range()
{
    rb_cRange = rb_define_class("Range", rb_cObject);
    rb_include_module(rb_cRange, rb_mEnumerable);
    rb_define_method(rb_cRange, "initialize", range_initialize, -1);
    rb_define_method(rb_cRange, "==", range_eq, 1);
    rb_define_method(rb_cRange, "===", range_eqq, 1);
    rb_define_method(rb_cRange, "eql?", range_eql, 1);
    rb_define_method(rb_cRange, "hash", range_hash, 0);
    rb_define_method(rb_cRange, "each", range_each, 0);
    rb_define_method(rb_cRange, "step", range_step, -1);
    rb_define_method(rb_cRange, "first", range_first, 0);
    rb_define_method(rb_cRange, "last", range_last, 0);
    rb_define_method(rb_cRange, "begin", range_first, 0);
    rb_define_method(rb_cRange, "end", range_last, 0);
    rb_define_method(rb_cRange, "to_s", range_to_s, 0);
    rb_define_method(rb_cRange, "inspect", range_inspect, 0);
    rb_define_alias(rb_cRange,  "to_ary", "to_a");

    rb_define_method(rb_cRange, "exclude_end?", range_exclude_end_p, 0);

    rb_define_method(rb_cRange, "length", range_length, 0);
    rb_define_method(rb_cRange, "size", range_length, 0);
    rb_define_method(rb_cRange, "member?", range_member, 1);
    rb_define_method(rb_cRange, "include?", range_member, 1);

    id_cmp = rb_intern("<=>");
    id_succ = rb_intern("succ");
    id_beg = rb_intern("begin");
    id_end = rb_intern("end");
    id_excl = rb_intern("excl");
}
