/************************************************

  object.c -

  $Author$
  $Date$
  created at: Thu Jul 15 12:01:24 JST 1993

  Copyright (C) 1993-1996 Yukihiro Matsumoto

************************************************/

#include "ruby.h"
#include "st.h"
#include <stdio.h>

VALUE mKernel;
VALUE cObject;
VALUE cModule;
VALUE cClass;
extern VALUE cFixnum;
VALUE cData;

static VALUE cNilClass;
static VALUE cTrueClass;
static VALUE cFalseClass;

struct st_table *new_idhash();

VALUE f_sprintf();

VALUE obj_alloc();

static ID eq, eql;
static ID inspect;

VALUE
rb_equal(obj1, obj2)
    VALUE obj1, obj2;
{
    VALUE result;

    result = rb_funcall(obj1, eq, 1, obj2);
    if (result == FALSE || NIL_P(result))
	return FALSE;
    return TRUE;
}

int
rb_eql(obj1, obj2)
    VALUE obj1, obj2;
{
    return rb_funcall(obj1, eql, 1, obj2);
}

VALUE
obj_equal(obj1, obj2)
    VALUE obj1, obj2;
{
    if (obj1 == obj2) return TRUE;
    return FALSE;
}

static VALUE
any_to_a(obj)
    VALUE obj;
{
    return ary_new3(1, obj);
}

static VALUE
obj_id(obj)
    VALUE obj;
{
    return INT2NUM((int)obj);
}

static VALUE
obj_type(obj)
    VALUE obj;
{
    VALUE cl = CLASS_OF(obj);

    if (FL_TEST(cl, FL_SINGLETON)) {
	cl = RCLASS(cl)->super;
    }
    return cl;
}

static VALUE
obj_clone(obj)
    VALUE obj;
{
    VALUE clone;

    if (TYPE(obj) != T_OBJECT) {
	TypeError("can't clone %s", rb_class2name(CLASS_OF(obj)));
    }
    clone = obj_alloc(RBASIC(obj)->class);
    CLONESETUP(clone,obj);
    if (ROBJECT(obj)->iv_tbl) {
	ROBJECT(clone)->iv_tbl = st_copy(ROBJECT(obj)->iv_tbl);
    RBASIC(clone)->class = singleton_class_clone(RBASIC(obj)->class);
    RBASIC(clone)->flags = RBASIC(obj)->flags;
    }

    return clone;
}

static VALUE
obj_dup(obj)
    VALUE obj;
{
    return rb_funcall(obj, rb_intern("clone"), 0, 0);
}

VALUE
any_to_s(obj)
    VALUE obj;
{
    char buf[256];

    sprintf(buf, "#<%s:0x%x>", rb_class2name(CLASS_OF(obj)), obj);
    return str_new2(buf);
}

VALUE
rb_inspect(obj)
    VALUE obj;
{
    return obj_as_string(rb_funcall(obj, inspect, 0, 0));
}

static int
inspect_i(id, value, str)
    ID id;
    VALUE value;
    VALUE str;
{
    VALUE str2;
    char *ivname;

    /* need not to show internal data */
    if (CLASS_OF(value) == 0) return ST_CONTINUE;
    if (RSTRING(str)->ptr[0] == '-') {
	RSTRING(str)->ptr[0] = '#';
	str_cat(str, ": ", 2);
    }
    else {
	str_cat(str, ", ", 2);
    }
    ivname = rb_id2name(id);
    str_cat(str, ivname, strlen(ivname));
    str_cat(str, "=", 1);
    if (TYPE(value) == T_OBJECT) {
	str2 = any_to_s(value);
    }
    else {
	str2 = rb_inspect(value);
    }
    str_cat(str, RSTRING(str2)->ptr, RSTRING(str2)->len);

    return ST_CONTINUE;
}

static VALUE
obj_inspect(obj)
    VALUE obj;
{
    if (TYPE(obj) == T_OBJECT
	&& ROBJECT(obj)->iv_tbl
	&& ROBJECT(obj)->iv_tbl->num_entries > 0) {
	VALUE str;
	char *b;

	str = str_new2("-<");
	b = rb_class2name(CLASS_OF(obj));
	str_cat(str, b, strlen(b));
	st_foreach(ROBJECT(obj)->iv_tbl, inspect_i, str);
	str_cat(str, ">", 1);

	return str;
    }
    return rb_funcall(obj, rb_intern("to_s"), 0, 0);
}

VALUE
obj_is_instance_of(obj, c)
    VALUE obj, c;
{
    VALUE cl;

    switch (TYPE(c)) {
      case T_MODULE:
      case T_CLASS:
	break;

      case T_NIL:
	if (NIL_P(obj)) return TRUE;
	return FALSE;

      case T_FALSE:
	if (obj) return FALSE;
	return TRUE;

      case T_TRUE:
	if (obj) return TRUE;
	return FALSE;

      default:
	TypeError("class or module required");
    }

    cl = CLASS_OF(obj);
    while (FL_TEST(cl, FL_SINGLETON)) {
	cl = RCLASS(cl)->super;
    }
    if (c == cl) return TRUE;
    return FALSE;
}

VALUE
obj_is_kind_of(obj, c)
    VALUE obj, c;
{
    VALUE cl = CLASS_OF(obj);

    switch (TYPE(c)) {
      case T_MODULE:
      case T_CLASS:
	break;

      case T_NIL:
	if (NIL_P(obj)) return TRUE;
	return FALSE;

      case T_FALSE:
	if (obj) return FALSE;
	return TRUE;

      case T_TRUE:
	if (obj) return TRUE;
	return FALSE;

      default:
	TypeError("class or module required");
    }

    while (cl) {
	if (cl == c || RCLASS(cl)->m_tbl == RCLASS(c)->m_tbl)
	    return TRUE;
	cl = RCLASS(cl)->super;
    }
    return FALSE;
}

static VALUE
obj_dummy(obj)
    VALUE obj;
{
    return Qnil;
}

static VALUE
nil_to_s(obj)
    VALUE obj;
{
    return str_new2("");
}

static VALUE
nil_to_a(obj)
    VALUE obj;
{
    return ary_new2(0);
}

static VALUE
nil_inspect(obj)
    VALUE obj;
{
    return str_new2("nil");
}

static VALUE
nil_type(obj)
    VALUE obj;
{
    return cNilClass;
}

static VALUE
nil_plus(x, y)
    VALUE x, y;
{
    switch (TYPE(y)) {
      case T_FIXNUM:
      case T_FLOAT:
      case T_BIGNUM:
      case T_STRING:
      case T_ARRAY:
	return y;
      default:
	TypeError("tried to add %s(%s) to nil",
		  RSTRING(obj_as_string(y))->ptr, rb_class2name(CLASS_OF(y)));
    }
    /* not reached */
}

static VALUE
main_to_s(obj)
    VALUE obj;
{
    return str_new2("main");
}

static VALUE
true_to_s(obj)
    VALUE obj;
{
    return str_new2("true");
}

static VALUE
true_type(obj)
    VALUE obj;
{
    return cTrueClass;
}

static VALUE
false_to_s(obj)
    VALUE obj;
{
    return str_new2("false");
}

static VALUE
false_type(obj)
    VALUE obj;
{
    return cFalseClass;
}

static VALUE
rb_true(obj)
    VALUE obj;
{
    return TRUE;
}

static VALUE
rb_false(obj)
    VALUE obj;
{
    return FALSE;
}

VALUE
obj_alloc(class)
    VALUE class;
{
    NEWOBJ(obj, struct RObject);
    OBJSETUP(obj, class, T_OBJECT);
    obj->iv_tbl = 0;

    return (VALUE)obj;
}

static VALUE
mod_clone(module)
    VALUE module;
{
    NEWOBJ(clone, struct RClass);
    OBJSETUP(clone, CLASS_OF(module), TYPE(module));

    clone->super = RCLASS(module)->super;
    clone->iv_tbl = 0;
    clone->m_tbl = 0;		/* avoid GC crashing  */
    clone->m_tbl = st_copy(RCLASS(module)->m_tbl);

    return (VALUE)clone;
}

static VALUE
mod_to_s(class)
    VALUE class;
{
    return rb_class_path(class);
}

static VALUE
mod_eqq(mod, arg)
    VALUE mod, arg;
{
    return obj_is_kind_of(arg, mod);
}

static VALUE
mod_le(mod, arg)
    VALUE mod, arg;
{
    switch (TYPE(arg)) {
      case T_MODULE:
      case T_CLASS:
	break;
      default:
	TypeError("compared with non class/module");
    }

    while (mod) {
	if (RCLASS(mod)->m_tbl == RCLASS(arg)->m_tbl)
	    return TRUE;
	mod = RCLASS(mod)->super;
    }

    return FALSE;
}

static VALUE
mod_lt(mod, arg)
    VALUE mod, arg;
{
    if (mod == arg) return FALSE;
    return mod_le(mod, arg);
}

static VALUE
mod_ge(mod, arg)
    VALUE mod, arg;
{
    switch (TYPE(arg)) {
      case T_MODULE:
      case T_CLASS:
	break;
      default:
	TypeError("compared with non class/module");
    }

    return mod_lt(arg, mod);
}

static VALUE
mod_gt(mod, arg)
    VALUE mod, arg;
{
    if (mod == arg) return FALSE;
    return mod_ge(mod, arg);
}

static VALUE
mod_cmp(mod, arg)
    VALUE mod, arg;
{
    if (mod == arg) return INT2FIX(0);

    switch (TYPE(arg)) {
      case T_MODULE:
      case T_CLASS:
	break;
      default:
	TypeError("<=> requires Class or Module (%s given)",
		  rb_class2name(CLASS_OF(arg)));
	break;
    }

    if (mod_le(mod, arg)) {
	return INT2FIX(-1);
    }
    return INT2FIX(1);
}

VALUE module_new();
VALUE class_new_instance();

static VALUE
class_s_new(argc, argv)
    int argc;
    VALUE *argv;
{
    VALUE super, klass;

    rb_scan_args(argc, argv, "01", &super);
    if (NIL_P(super)) super = cObject;
    Check_Type(super, T_CLASS);
    if (FL_TEST(super, FL_SINGLETON)) {
	TypeError("can't make subclass of virtual class");
    }
    klass = class_new(super);
    /* make metaclass */
    RBASIC(klass)->class = singleton_class_new(RBASIC(super)->class);
    singleton_class_attached(RBASIC(klass)->class, klass);

    return klass;
}

VALUE mod_name();
VALUE mod_included_modules();
VALUE mod_ancestors();
VALUE class_instance_methods();
VALUE class_private_instance_methods();

static VALUE
class_superclass(cl)
    VALUE cl;
{
    VALUE super = RCLASS(cl)->super;

    while (TYPE(super) == T_ICLASS) {
	super = RCLASS(super)->super;
    }
    if (!super) {
	return Qnil;
    }
    return super;
}

ID
rb_to_id(name)
    VALUE name;
{
    if (TYPE(name) == T_STRING) {
	return rb_intern(RSTRING(name)->ptr);
    }
    Check_Type(name, T_FIXNUM);
    return FIX2UINT(name);
}

static VALUE
mod_attr(argc, argv, class)
    int argc;
    VALUE *argv;
    VALUE class;
{
    VALUE name, pub;

    rb_scan_args(argc, argv, "11", &name, &pub);
    rb_define_attr(class, rb_to_id(name), 1, RTEST(pub));
    return Qnil;
}

static VALUE
mod_attr_reader(argc, argv, class)
    int argc;
    VALUE *argv;
    VALUE class;
{
    int i;

    for (i=0; i<argc; i++) {
	rb_define_attr(class, rb_to_id(argv[i]), 1, 0);
    }
    return Qnil;
}

static VALUE
mod_attr_writer(argc, argv, class)
    int argc;
    VALUE *argv;
    VALUE class;
{
    int i;

    for (i=0; i<argc; i++) {
	rb_define_attr(class, rb_to_id(argv[i]), 0, 1);
    }
    return Qnil;
}

static VALUE
mod_attr_accessor(argc, argv, class)
    int argc;
    VALUE *argv;
    VALUE class;
{
    int i;

    for (i=0; i<argc; i++) {
	rb_define_attr(class, rb_to_id(argv[i]), 1, 1);
    }
    return Qnil;
}

VALUE mod_constants();

static VALUE
mod_const_get(mod, name)
    VALUE mod, name;
{
    return rb_const_get_at(mod, rb_to_id(name));
}

static VALUE
mod_const_set(mod, name, value)
    VALUE mod, name, value;
{
    rb_const_set(mod, rb_to_id(name), value);
    return value;
}

static VALUE
mod_const_defined(mod, name)
    VALUE mod, name;
{
    return rb_const_defined_at(mod, rb_to_id(name));
}

static VALUE
obj_methods(obj)
    VALUE obj;
{
    VALUE argv[1];

    argv[0] = TRUE;
    return class_instance_methods(1, argv, CLASS_OF(obj));
}

VALUE obj_singleton_methods();

static VALUE
obj_private_methods(obj)
    VALUE obj;
{
    VALUE argv[1];

    argv[0] = TRUE;
    return class_private_instance_methods(1, argv, CLASS_OF(obj));
}

VALUE obj_instance_variables();
VALUE obj_remove_instance_variable();

static VALUE
f_integer(obj, arg)
    VALUE obj, arg;
{
    int i;

    switch (TYPE(arg)) {
      case T_FLOAT:
	if (RFLOAT(arg)->value <= (double)FIXNUM_MAX
	    && RFLOAT(arg)->value >= (double)FIXNUM_MIN) {
	    i = (int)RFLOAT(arg)->value;
	    break;
	}
	return dbl2big(RFLOAT(arg)->value);

      case T_BIGNUM:
	return arg;

      case T_STRING:
	return str2inum(RSTRING(arg)->ptr, 0);

      default:
	i = NUM2INT(arg);
    }
    return INT2NUM(i);
}

VALUE
rb_Integer(val)
    VALUE val;
{
    return f_integer(Qnil, val);
}

static VALUE
to_flo(val)
    VALUE val;
{
    return rb_funcall(val, rb_intern("to_f"), 0);
}

static VALUE
fail_to_flo(val)
    VALUE val;
{
    TypeError("failed to convert %s into Float", rb_class2name(CLASS_OF(val)));
}

double big2dbl();

static VALUE
f_float(obj, arg)
    VALUE obj, arg;
{
    switch (TYPE(arg)) {
      case T_FIXNUM:
	return float_new((double)FIX2INT(arg));

      case T_FLOAT:
	return arg;

      case T_BIGNUM:
	return float_new(big2dbl(arg));

      default:
	return rb_rescue(to_flo, arg, fail_to_flo, arg);
    }
}

VALUE
rb_Float(val)
    VALUE val;
{
    return f_float(Qnil, val);
}

double
num2dbl(val)
    VALUE val;
{
    VALUE v = rb_Float(val);
    return RFLOAT(v)->value;
}

static VALUE
f_string(obj, arg)
    VALUE obj, arg;
{
    return rb_funcall(arg, rb_intern("to_s"), 0);
}

VALUE
rb_String(val)
    VALUE val;
{
    return f_string(Qnil, val);
}

static VALUE
f_array(obj, arg)
    VALUE obj, arg;
{
    if (TYPE(arg) == T_ARRAY) return arg;
    arg = rb_funcall(arg, rb_intern("to_a"), 0);
    if (TYPE(arg) != T_ARRAY) {
	TypeError("`to_a' did not return Array");
    }
    return arg;
}

VALUE
rb_Array(val)
    VALUE val;
{
    return f_array(Qnil, val);
}

VALUE
rb_to_a(val)			/* backward compatibility */
    VALUE val;
{
    return f_array(Qnil, val);
}

static VALUE
boot_defclass(name, super)
    char *name;
    VALUE super;
{
    extern st_table *rb_class_tbl;
    VALUE obj = class_new(super);
    ID id = rb_intern(name);

    rb_name_class(obj, id);
    st_add_direct(rb_class_tbl, id, obj);
    return (VALUE)obj;
}

VALUE
rb_class_of(obj)
    VALUE obj;
{
    if (FIXNUM_P(obj)) return cFixnum;
    if (obj == Qnil) return cNilClass;
    if (obj == FALSE) return cFalseClass;
    if (obj == TRUE) return cTrueClass;

    return RBASIC(obj)->class;
}

VALUE TopSelf;

void
Init_Object()
{
    VALUE metaclass;

    cObject = boot_defclass("Object", 0);
    cModule = boot_defclass("Module", cObject);
    cClass =  boot_defclass("Class",  cModule);

    metaclass = RBASIC(cObject)->class = singleton_class_new(cClass);
    singleton_class_attached(metaclass, cObject);
    metaclass = RBASIC(cModule)->class = singleton_class_new(metaclass);
    singleton_class_attached(metaclass, cModule);
    metaclass = RBASIC(cClass)->class = singleton_class_new(metaclass);
    singleton_class_attached(metaclass, cClass);

    mKernel = rb_define_module("Kernel");
    rb_include_module(cObject, mKernel);
    rb_define_private_method(cClass, "inherited", obj_dummy, 1);

    /*
     * Ruby's Class Hierarchy Chart
     *
     *                           +------------------+
     *                           |                  |
     *             Object---->(Object)              |
     *              ^  ^        ^  ^                |
     *              |  |        |  |                |
     *              |  |  +-----+  +---------+      |
     *              |  |  |                  |      |
     *              |  +-----------+         |      |
     *              |     |        |         |      |
     *       +------+     |     Module--->(Module)  |
     *       |            |        ^         ^      |
     *  OtherClass-->(OtherClass)  |         |      |
     *                             |         |      |
     *                           Class---->(Class)  |
     *                             ^                |
     *                             |                |
     *                             +----------------+
     *
     *   + All metaclasses are instances of the class `Class'.
     */

    rb_define_method(mKernel, "nil?", rb_false, 0);
    rb_define_method(mKernel, "==", obj_equal, 1);
    rb_define_alias(mKernel, "equal?", "==");
    rb_define_alias(mKernel, "===", "==");
    rb_define_method(mKernel, "=~", rb_false, 1);

    rb_define_method(mKernel, "eql?", obj_equal, 1);

    rb_define_method(mKernel, "hash", obj_id, 0);
    rb_define_method(mKernel, "id", obj_id, 0);
    rb_define_method(mKernel, "type", obj_type, 0);

    rb_define_method(mKernel, "clone", obj_clone, 0);
    rb_define_method(mKernel, "dup", obj_dup, 0);

    rb_define_method(mKernel, "to_a", any_to_a, 0);
    rb_define_method(mKernel, "to_s", any_to_s, 0);
    rb_define_method(mKernel, "inspect", obj_inspect, 0);
    rb_define_method(mKernel, "methods", obj_methods, 0);
    rb_define_method(mKernel, "singleton_methods", obj_singleton_methods, 0);
    rb_define_method(mKernel, "private_methods", obj_private_methods, 0);
    rb_define_method(mKernel, "instance_variables", obj_instance_variables, 0);
    rb_define_method(mKernel, "remove_instance_variable", obj_remove_instance_variable, 0);

    rb_define_method(mKernel, "instance_of?", obj_is_instance_of, 1);
    rb_define_method(mKernel, "kind_of?", obj_is_kind_of, 1);
    rb_define_method(mKernel, "is_a?", obj_is_kind_of, 1);

    rb_define_global_function("sprintf", f_sprintf, -1);
    rb_define_alias(mKernel, "format", "sprintf");

    rb_define_global_function("Integer", f_integer, 1);
    rb_define_global_function("Float", f_float, 1);

    rb_define_global_function("String", f_string, 1);
    rb_define_global_function("Array", f_array, 1);

    cNilClass = rb_define_class("NilClass", cObject);
    rb_define_method(cNilClass, "type", nil_type, 0);
    rb_define_method(cNilClass, "to_s", nil_to_s, 0);
    rb_define_method(cNilClass, "to_a", nil_to_a, 0);
    rb_define_method(cNilClass, "inspect", nil_inspect, 0);

    rb_define_method(cNilClass, "nil?", rb_true, 0);
    rb_undef_method(CLASS_OF(cNilClass), "new");
    rb_define_global_const("NIL", Qnil);

    /* default addition */
    rb_define_method(cNilClass, "+", nil_plus, 1);

    rb_define_global_function("initialize", obj_dummy, -1);
    rb_define_global_function("singleton_method_added", obj_dummy, 1);

    rb_define_method(cModule, "===", mod_eqq, 1);
    rb_define_method(cModule, "<=>",  mod_cmp, 1);
    rb_define_method(cModule, "<",  mod_lt, 1);
    rb_define_method(cModule, "<=", mod_le, 1);
    rb_define_method(cModule, ">",  mod_gt, 1);
    rb_define_method(cModule, ">=", mod_ge, 1);
    rb_define_method(cModule, "clone", mod_clone, 0);
    rb_define_method(cModule, "to_s", mod_to_s, 0);
    rb_define_method(cModule, "included_modules", mod_included_modules, 0);
    rb_define_method(cModule, "name", mod_name, 0);
    rb_define_method(cModule, "ancestors", mod_ancestors, 0);

    rb_define_private_method(cModule, "attr", mod_attr, -1);
    rb_define_private_method(cModule, "attr_reader", mod_attr_reader, -1);
    rb_define_private_method(cModule, "attr_writer", mod_attr_writer, -1);
    rb_define_private_method(cModule, "attr_accessor", mod_attr_accessor, -1);

    rb_define_singleton_method(cModule, "new", module_new, 0);
    rb_define_method(cModule, "instance_methods", class_instance_methods, -1);
    rb_define_method(cModule, "private_instance_methods", class_private_instance_methods, -1);

    rb_define_method(cModule, "constants", mod_constants, 0);
    rb_define_method(cModule, "const_get", mod_const_get, 1);
    rb_define_method(cModule, "const_set", mod_const_set, 2);
    rb_define_method(cModule, "const_defined?", mod_const_defined, 1);
    rb_define_private_method(cModule, "method_added", obj_dummy, 1);

    rb_define_method(cClass, "new", class_new_instance, -1);
    rb_define_method(cClass, "superclass", class_superclass, 0);
    rb_define_singleton_method(cClass, "new", class_s_new, -1);
    rb_undef_method(cClass, "extend_object");
    rb_undef_method(cClass, "append_features");

    rb_define_singleton_method(cClass, "new", class_s_new, -1);

    cData = rb_define_class("Data", cObject);

    TopSelf = obj_alloc(cObject);
    rb_global_variable(&TopSelf);
    rb_define_singleton_method(TopSelf, "to_s", main_to_s, 0);

    cTrueClass = rb_define_class("TrueClass", cObject);
    rb_define_method(cTrueClass, "to_s", true_to_s, 0);
    rb_define_method(cTrueClass, "type", true_type, 0);
    rb_undef_method(CLASS_OF(cTrueClass), "new");
    rb_define_global_const("TRUE", TRUE);

    cFalseClass = rb_define_class("FalseClass", cObject);
    rb_define_method(cFalseClass, "to_s", false_to_s, 0);
    rb_define_method(cFalseClass, "type", false_type, 0);
    rb_undef_method(CLASS_OF(cFalseClass), "new");
    rb_define_global_const("FALSE", FALSE);

    eq = rb_intern("==");
    eql = rb_intern("eql?");
    inspect = rb_intern("inspect");
}
