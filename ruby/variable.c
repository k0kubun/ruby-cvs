/**********************************************************************

  variable.c -

  $Author$
  $Date$
  created at: Tue Apr 19 23:55:15 JST 1994

  Copyright (C) 1993-2002 Yukihiro Matsumoto
  Copyright (C) 2000  Network Applied Communication Laboratory, Inc.
  Copyright (C) 2000  Information-technology Promotion Agency, Japan

**********************************************************************/

#include "ruby.h"
#include "env.h"
#include "node.h"
#include "st.h"
#include "util.h"

static st_table *rb_global_tbl;
st_table *rb_class_tbl;

void
Init_var_tables()
{
    rb_global_tbl = st_init_numtable();
    rb_class_tbl = st_init_numtable();
}

struct fc_result {
    ID name;
    VALUE klass;
    VALUE path;
    VALUE track;
    struct fc_result *prev;
};

static VALUE
fc_path(fc, name)
    struct fc_result *fc;
    ID name;
{
    VALUE path, tmp;

    path = rb_str_new2(rb_id2name(name));
    while (fc) {
	if (fc->track == rb_cObject) break;
	if (ROBJECT(fc->track)->iv_tbl &&
	    st_lookup(ROBJECT(fc->track)->iv_tbl, rb_intern("__classpath__"), &tmp)) {
	    tmp = rb_str_dup(tmp);
	    rb_str_cat2(tmp, "::");
	    rb_str_append(tmp, path);

	    return tmp;
	}
	tmp = rb_str_new2(rb_id2name(fc->name));
	rb_str_cat2(tmp, "::");
	rb_str_append(tmp, path);
	path = tmp;
	fc = fc->prev;
    }
    return path;
}

static int
fc_i(key, value, res)
    ID key;
    VALUE value;
    struct fc_result *res;
{
    if (!rb_is_const_id(key)) return ST_CONTINUE;

    if (value == res->klass) {
	res->path = fc_path(res, key);
	return ST_STOP;
    }
    switch (TYPE(value)) {
      case T_MODULE:
      case T_CLASS:
	if (!RCLASS(value)->iv_tbl) return ST_CONTINUE;
	else {
	    struct fc_result arg;
	    struct fc_result *list;

	    list = res;
	    while (list) {
		if (list->track == value) return ST_CONTINUE;
		list = list->prev;
	    }

	    arg.name = key;
	    arg.path = 0;
	    arg.klass = res->klass;
	    arg.track = value;
	    arg.prev = res;
	    st_foreach(RCLASS(value)->iv_tbl, fc_i, &arg);
	    if (arg.path) {
		res->path = arg.path;
		return ST_STOP;
	    }
	}
	break;

      default:
	break;
	    
    }
    return ST_CONTINUE;
}

static VALUE
find_class_path(klass)
    VALUE klass;
{
    struct fc_result arg;

    arg.name = 0;
    arg.path = 0;
    arg.klass = klass;
    arg.track = rb_cObject;
    arg.prev = 0;
    if (RCLASS(rb_cObject)->iv_tbl) {
	st_foreach(RCLASS(rb_cObject)->iv_tbl, fc_i, &arg);
    }
    if (arg.path == 0) {
	st_foreach(rb_class_tbl, fc_i, &arg);
    }
    if (arg.path) {
	if (!ROBJECT(klass)->iv_tbl) {
	    ROBJECT(klass)->iv_tbl = st_init_numtable();
	} 
	st_insert(ROBJECT(klass)->iv_tbl,rb_intern("__classpath__"),arg.path);
	return arg.path;
    }
    return Qnil;
}

static VALUE
classname(klass)
    VALUE klass;
{
    VALUE path = Qnil;
    ID classpath = rb_intern("__classpath__");

    klass = rb_class_real(klass);
    if (!klass) klass = rb_cObject;
    if (ROBJECT(klass)->iv_tbl &&
	!st_lookup(ROBJECT(klass)->iv_tbl, classpath, &path)) {
	ID classid = rb_intern("__classid__");

	if (st_lookup(ROBJECT(klass)->iv_tbl, classid, &path)) {
	    path = rb_str_new2(rb_id2name(SYM2ID(path)));
	    st_insert(ROBJECT(klass)->iv_tbl, classpath, path);
	    st_delete(RCLASS(klass)->iv_tbl, &classid, 0);
	}
    }
    if (NIL_P(path)) {
	path = find_class_path(klass);
	if (NIL_P(path)) {
	    return 0;
	}
	return path;
    }
    if (TYPE(path) != T_STRING)
	rb_bug("class path is not set properly");
    return path;
}

VALUE
rb_mod_name(mod)
    VALUE mod;
{
    VALUE path = classname(mod);

    if (path) return rb_str_dup(path);
    return rb_str_new(0,0);
}

VALUE
rb_class_path(klass)
    VALUE klass;
{
    VALUE path = classname(klass);

    if (path) return path;
    else {
	VALUE str;
	char *s = "Class";

	if (TYPE(klass) == T_MODULE) {
	    if (rb_obj_class(klass) == rb_cModule) {
		s = "Module";
	    }
	    else {
		s = rb_class2name(RBASIC(klass)->klass);
	    }
	}
	str = rb_str_new(0, 2 + strlen(s) + 3 + 2 * SIZEOF_LONG + 1);
	sprintf(RSTRING(str)->ptr, "#<%s:0x%lx>", s, klass);
	RSTRING(str)->len = strlen(RSTRING(str)->ptr);

	return str;
    }
}

void
rb_set_class_path(klass, under, name)
    VALUE klass, under;
    const char *name;
{
    VALUE str;

    if (under == rb_cObject) {
	str = rb_str_new2(name);
    }
    else {
	str = rb_str_dup(rb_class_path(under));
	rb_str_cat2(str, "::");
	rb_str_cat2(str, name);
    }
    rb_iv_set(klass, "__classpath__", str);
}

VALUE
rb_path2class(path)
    const char *path;
{
    const char *pbeg, *p;
    ID id;
    VALUE c = rb_cObject;

    if (path[0] == '#') {
	rb_raise(rb_eArgError, "can't retrieve anonymous class %s", path);
    }
    pbeg = p = path;
    while (*p) {
	VALUE str;

	while (*p && *p != ':') p++;
	str = rb_str_new(pbeg, p-pbeg);
	id = rb_intern(RSTRING(str)->ptr);
	if (p[0] == ':') {
	    if (p[1] != ':') goto undefined_class;
	    p += 2;
	    pbeg = p;
	}
	if (!rb_const_defined(c, id)) {
	  undefined_class:
	    rb_raise(rb_eArgError, "undefined class/module %s", rb_id2name(id));
	    rb_raise(rb_eArgError, "undefined class/module %s", path);
	}
	c = rb_const_get_at(c, id);
	switch (TYPE(c)) {
	  case T_MODULE:
	  case T_CLASS:
	    break;
	  default:
	    rb_raise(rb_eTypeError, "%s does not refer class/module", path);
	}
    }

    return c;
}

void
rb_name_class(klass, id)
    VALUE klass;
    ID id;
{
    rb_iv_set(klass, "__classid__", ID2SYM(id));
}

static st_table *autoload_tbl = 0;

static void
rb_autoload_id(id, filename)
    ID id;
    const char *filename;
{
    rb_secure(4);
    if (!rb_is_const_id(id)) {
	rb_name_error(id, "autoload must be constant name");
    }

    if (!autoload_tbl) {
	autoload_tbl = st_init_numtable();
    }
    st_insert(autoload_tbl, id, strdup(filename));
}

void
rb_autoload(klass, filename)
    const char *klass, *filename;
{
    rb_autoload_id(rb_intern(klass), filename);
}

VALUE
rb_f_autoload(obj, klass, file)
    VALUE obj, klass, file;
{
    rb_autoload_id(rb_to_id(klass), StringValuePtr(file));
    return Qnil;
}

char *
rb_class2name(klass)
    VALUE klass;
{
    return RSTRING(rb_class_path(klass))->ptr;
}

struct trace_var {
    int removed;
    void (*func)();
    VALUE data;
    struct trace_var *next;
};

struct global_variable {
    int   counter;
    void *data;
    VALUE (*getter)();
    void  (*setter)();
    void  (*marker)();
    int block_trace;
    struct trace_var *trace;
};

struct global_entry {
    struct global_variable *var;
    ID id;
};

static VALUE undef_getter();
static void  undef_setter();
static void  undef_marker();

static VALUE val_getter();
static void  val_setter();
static void  val_marker();

static VALUE var_getter();
static void  var_setter();
static void  var_marker();

struct global_entry*
rb_global_entry(id)
    ID id;
{
    struct global_entry *entry;

    if (!st_lookup(rb_global_tbl, id, &entry)) {
	struct global_variable *var;
	entry = ALLOC(struct global_entry);
	var = ALLOC(struct global_variable);
	entry->id = id;
	entry->var = var;
	var->counter = 1;
	var->data = 0;
	var->getter = undef_getter;
	var->setter = undef_setter;
	var->marker = undef_marker;

	var->block_trace = 0;
	var->trace = 0;
	st_add_direct(rb_global_tbl, id, entry);
    }
    return entry;
}

static VALUE
undef_getter(id)
    ID id;
{
    rb_warning("global variable `%s' not initialized", rb_id2name(id));

    return Qnil;
}

static void
undef_setter(val, id, data, var)
    VALUE val;
    ID id;
    void *data;
    struct global_variable *var;
{
    var->getter = val_getter;
    var->setter = val_setter;
    var->marker = val_marker;

    var->data = (void*)val;
}

static void
undef_marker()
{
}

static VALUE
val_getter(id, val)
    ID id;
    VALUE val;
{
    return val;
}

static void
val_setter(val, id, data, var)
    VALUE val;
    ID id;
    void *data;
    struct global_variable *var;
{
    var->data = (void*)val;
}

static void
val_marker(data)
    VALUE data;
{
    if (data) rb_gc_mark_maybe(data);
}

static VALUE
var_getter(id, var)
    ID id;
    VALUE *var;
{
    if (!var) return Qnil;
    return *var;
}

static void
var_setter(val, id, var)
    VALUE val;
    ID id;
    VALUE *var;
{
    *var = val;
}

static void
var_marker(var)
    VALUE *var;
{
    if (var) rb_gc_mark_maybe(*var);
}

static void
readonly_setter(val, id, var)
    VALUE val;
    ID id;
    void *var;
{
    rb_name_error(id, "can't set variable %s", rb_id2name(id));
}

static int
mark_global_entry(key, entry)
    ID key;
    struct global_entry *entry;
{
    struct trace_var *trace;
    struct global_variable *var = entry->var;

    (*var->marker)(var->data);
    trace = var->trace;
    while (trace) {
	if (trace->data) rb_gc_mark_maybe(trace->data);
	trace = trace->next;
    }
    return ST_CONTINUE;
}

void
rb_gc_mark_global_tbl()
{
    st_foreach(rb_global_tbl, mark_global_entry, 0);
}

static ID
global_id(name)
    const char *name;
{
    ID id;

    if (name[0] == '$') id = rb_intern(name);
    else {
	char *buf = ALLOCA_N(char, strlen(name)+2);
	buf[0] = '$';
	strcpy(buf+1, name);
	id = rb_intern(buf);
    }
    return id;
}

void
rb_define_hooked_variable(name, var, getter, setter)
    const char  *name;
    VALUE *var;
    VALUE (*getter)();
    void  (*setter)();
{
    struct global_variable *gvar;
    ID id = global_id(name);

    gvar = rb_global_entry(id)->var;
    gvar->data = (void*)var;
    gvar->getter = getter?getter:var_getter;
    gvar->setter = setter?setter:var_setter;
    gvar->marker = var_marker;
}

void
rb_define_variable(name, var)
    const char  *name;
    VALUE *var;
{
    rb_define_hooked_variable(name, var, 0, 0);
}

void
rb_define_readonly_variable(name, var)
    const char  *name;
    VALUE *var;
{
    rb_define_hooked_variable(name, var, 0, readonly_setter);
}

void
rb_define_virtual_variable(name, getter, setter)
    const char  *name;
    VALUE (*getter)();
    void  (*setter)();
{
    if (!getter) getter = val_getter;
    if (!setter) setter = readonly_setter;
    rb_define_hooked_variable(name, 0, getter, setter);
}

static void
rb_trace_eval(cmd, val)
    VALUE cmd, val;
{
    rb_eval_cmd(cmd, rb_ary_new3(1, val), 0);
}

VALUE
rb_f_trace_var(argc, argv)
    int argc;
    VALUE *argv;
{
    VALUE var, cmd;
    struct global_entry *entry;
    struct trace_var *trace;

    rb_secure(4);
    if (rb_scan_args(argc, argv, "11", &var, &cmd) == 1) {
	cmd = rb_f_lambda();
    }
    if (NIL_P(cmd)) {
	return rb_f_untrace_var(argc, argv);
    }
    entry = rb_global_entry(rb_to_id(var));
    if (OBJ_TAINTED(cmd)) {
	rb_raise(rb_eSecurityError, "Insecure: tainted variable trace");
    }
    trace = ALLOC(struct trace_var);
    trace->next = entry->var->trace;
    trace->func = rb_trace_eval;
    trace->data = cmd;
    trace->removed = 0;
    entry->var->trace = trace;

    return Qnil;
}

static void
remove_trace(var)
    struct global_variable *var;
{
    struct trace_var *trace = var->trace;
    struct trace_var t;
    struct trace_var *next;

    t.next = trace;
    trace = &t;
    while (trace->next) {
	next = trace->next;
	if (next->removed) {
	    trace->next = next->next;
	    free(next);
	}
	else {
	    trace = next;
	}
    }
    var->trace = t.next;
}

VALUE
rb_f_untrace_var(argc, argv)
    int argc;
    VALUE *argv;
{
    VALUE var, cmd;
    ID id;
    struct global_entry *entry;
    struct trace_var *trace;

    rb_scan_args(argc, argv, "11", &var, &cmd);
    id = rb_to_id(var);
    if (!st_lookup(rb_global_tbl, id, &entry)) {
	rb_name_error(id, "undefined global variable %s", rb_id2name(id));
    }

    trace = entry->var->trace;
    if (NIL_P(cmd)) {
	VALUE ary = rb_ary_new();

	while (trace) {
	    struct trace_var *next = trace->next;
	    rb_ary_push(ary, (VALUE)trace->data);
	    trace->removed = 1;
	    trace = next;
	}

	if (!entry->var->block_trace) remove_trace(entry->var);
	return ary;
    }
    else {
	while (trace) {
	    if (trace->data == cmd) {
		trace->removed = 1;
		if (!entry->var->block_trace) remove_trace(entry->var);
		return rb_ary_new3(1, cmd);
	    }
	    trace = trace->next;
	}
    }
    return Qnil;
}

VALUE
rb_gvar_get(entry)
    struct global_entry *entry;
{
    struct global_variable *var = entry->var;
    return (*var->getter)(entry->id, var->data, var);
}

struct trace_data {
    struct trace_var *trace;
    VALUE val;
};
    
static VALUE
trace_ev(data)
    struct trace_data *data;
{
    struct trace_var *trace = data->trace;

    while (trace) {
	(*trace->func)(trace->data, data->val);
	trace = trace->next;
    }
    return Qnil;		/* not reached */
}

static VALUE
trace_en(var)
    struct global_variable *var;
{
    var->block_trace = 0;
    remove_trace(var);
    return Qnil;		/* not reached */
}

VALUE
rb_gvar_set(entry, val)
    struct global_entry *entry;
    VALUE val;
{
    struct trace_data trace;
    struct global_variable *var = entry->var;

    if (rb_safe_level() >= 4)
	rb_raise(rb_eSecurityError, "Insecure: can't change global variable value");
    (*var->setter)(val, entry->id, var->data, var);

    if (var->trace && !var->block_trace) {
	var->block_trace = 1;
	trace.trace = var->trace;
	trace.val = val;
	rb_ensure(trace_ev, (VALUE)&trace, trace_en, (VALUE)var);
    }
    return val;
}

VALUE
rb_gv_set(name, val)
    const char *name;
    VALUE val;
{
    struct global_entry *entry;

    entry = rb_global_entry(global_id(name));
    return rb_gvar_set(entry, val);
}

VALUE
rb_gv_get(name)
    const char *name;
{
    struct global_entry *entry;

    entry = rb_global_entry(global_id(name));
    return rb_gvar_get(entry);
}

VALUE
rb_gvar_defined(entry)
    struct global_entry *entry;
{
    if (entry->var->getter == undef_getter) return Qfalse;
    return Qtrue;
}

static int
gvar_i(key, entry, ary)
    ID key;
    struct global_entry *entry;
    VALUE ary;
{
    rb_ary_push(ary, rb_str_new2(rb_id2name(key)));
    return ST_CONTINUE;
}

VALUE
rb_f_global_variables()
{
    VALUE ary = rb_ary_new();
    char buf[4];
    char *s = "&`'+123456789";

    st_foreach(rb_global_tbl, gvar_i, ary);
    if (!NIL_P(rb_backref_get())) {
	while (*s) {
	    sprintf(buf, "$%c", *s++);
	    rb_ary_push(ary, rb_str_new2(buf));
	}
    }
    return ary;
}

void
rb_alias_variable(name1, name2)
    ID name1;
    ID name2;
{
    struct global_entry *entry1, *entry2;

    if (rb_safe_level() >= 4)
	rb_raise(rb_eSecurityError, "Insecure: can't alias global variable");

    entry2 = rb_global_entry(name2);
    if (!st_lookup(rb_global_tbl, name1, &entry1)) {
	entry1 = ALLOC(struct global_entry);
	entry1->id = name1;
	st_add_direct(rb_global_tbl, name1, entry1);
    }
    else if (entry1->var != entry2->var) {
	struct global_variable *var = entry1->var;
	if (var->block_trace) {
	    rb_raise(rb_eRuntimeError, "can't alias in tracer");
	}
	var->counter--;
	if (var->counter == 0) {
	    struct trace_var *trace = var->trace;
	    while (trace) {
		struct trace_var *next = trace->next;
		free(trace);
		trace = next;
	    }
	    free(var);
	}
    }
    else {
	return;
    }
    entry2->var->counter++;
    entry1->var = entry2->var;
}

static int special_generic_ivar = 0;
static st_table *generic_iv_tbl;

st_table*
rb_generic_ivar_table(obj)
    VALUE obj;
{
    st_table *tbl;

    if (!generic_iv_tbl) return 0;
    if (!st_lookup(generic_iv_tbl, obj, &tbl)) return 0;
    return tbl;
}

static VALUE
generic_ivar_get(obj, id)
    VALUE obj;
    ID id;
{
    st_table *tbl;
    VALUE val;

    if (!generic_iv_tbl) return Qnil;
    if (!st_lookup(generic_iv_tbl, obj, &tbl)) return Qnil;
    if (st_lookup(tbl, id, &val)) {
	return val;
    }
    return Qnil;
}

static void
generic_ivar_set(obj, id, val)
    VALUE obj;
    ID id;
    VALUE val;
{
    st_table *tbl;

    if (rb_special_const_p(obj)) {
	special_generic_ivar = 1;
    }
    if (!generic_iv_tbl) {
	generic_iv_tbl = st_init_numtable();
    }

    if (!st_lookup(generic_iv_tbl, obj, &tbl)) {
	FL_SET(obj, FL_EXIVAR);
	tbl = st_init_numtable();
	st_add_direct(generic_iv_tbl, obj, tbl);
	st_add_direct(tbl, id, val);
	return;
    }
    st_insert(tbl, id, val);
}

static VALUE
generic_ivar_defined(obj, id)
    VALUE obj;
    ID id;
{
    st_table *tbl;
    VALUE val;

    if (!generic_iv_tbl) return Qfalse;
    if (!st_lookup(generic_iv_tbl, obj, &tbl)) return Qfalse;
    if (st_lookup(tbl, id, &val)) {
	return Qtrue;
    }
    return Qfalse;
}

static int
generic_ivar_remove(obj, id, valp)
    VALUE obj;
    ID id;
    VALUE *valp;
{
    st_table *tbl;
    int status;

    if (!generic_iv_tbl) return 0;
    if (!st_lookup(generic_iv_tbl, obj, &tbl)) return 0;
    status = st_delete(tbl, &id, valp);
    if (tbl->num_entries == 0) {
	st_delete(generic_iv_tbl, &obj, &tbl);
	st_free_table(tbl);
    }
    return status;
}

void
rb_mark_generic_ivar(obj)
    VALUE obj;
{
    st_table *tbl;

    if (!generic_iv_tbl) return;
    if (st_lookup(generic_iv_tbl, obj, &tbl)) {
	rb_mark_tbl(tbl);
    }
}

static int
givar_mark_i(key, value)
    ID key;
    VALUE value;
{
    rb_gc_mark(value);
    return ST_CONTINUE;
}

static int
givar_i(obj, tbl)
    VALUE obj;
    st_table *tbl;
{
    if (rb_special_const_p(obj)) {
	st_foreach(tbl, givar_mark_i, 0);
    }
    return ST_CONTINUE;
}

void
rb_mark_generic_ivar_tbl()
{
    if (!generic_iv_tbl) return;
    if (special_generic_ivar == 0) return;
    st_foreach(generic_iv_tbl, givar_i, 0);
}

void
rb_free_generic_ivar(obj)
    VALUE obj;
{
    st_table *tbl;

    if (st_delete(generic_iv_tbl, &obj, &tbl))
	st_free_table(tbl);
}

void
rb_copy_generic_ivar(clone, obj)
    VALUE clone, obj;
{
    st_table *tbl;

    if (!generic_iv_tbl) return;
    if (st_lookup(generic_iv_tbl, obj, &tbl)) {
	st_table *old;

	if (st_lookup(generic_iv_tbl, clone, &old)) {
	    st_free_table(old);
	    st_insert(generic_iv_tbl, clone, st_copy(tbl));
	}
	else {
	    st_add_direct(generic_iv_tbl, clone, st_copy(tbl));
	}
    }
}

VALUE
rb_ivar_get(obj, id)
    VALUE obj;
    ID id;
{
    VALUE val;

    switch (TYPE(obj)) {
      case T_OBJECT:
      case T_CLASS:
      case T_MODULE:
	if (ROBJECT(obj)->iv_tbl && st_lookup(ROBJECT(obj)->iv_tbl, id, &val))
	    return val;
	break;
      default:
	if (FL_TEST(obj, FL_EXIVAR) || rb_special_const_p(obj))
	    return generic_ivar_get(obj, id);
	break;
    }
    rb_warning("instance variable %s not initialized", rb_id2name(id));

    return Qnil;
}

VALUE
rb_ivar_set(obj, id, val)
    VALUE obj;
    ID id;
    VALUE val;
{
    if (!OBJ_TAINTED(obj) && rb_safe_level() >= 4)
	rb_raise(rb_eSecurityError, "Insecure: can't modify instance variable");
    if (OBJ_FROZEN(obj)) rb_error_frozen("object");
    switch (TYPE(obj)) {
      case T_OBJECT:
      case T_CLASS:
      case T_MODULE:
	if (!ROBJECT(obj)->iv_tbl) ROBJECT(obj)->iv_tbl = st_init_numtable();
	st_insert(ROBJECT(obj)->iv_tbl, id, val);
	break;
      default:
	generic_ivar_set(obj, id, val);
	break;
    }
    return val;
}

VALUE
rb_ivar_defined(obj, id)
    VALUE obj;
    ID id;
{
    switch (TYPE(obj)) {
      case T_OBJECT:
      case T_CLASS:
      case T_MODULE:
	if (ROBJECT(obj)->iv_tbl && st_lookup(ROBJECT(obj)->iv_tbl, id, 0))
	    return Qtrue;
	break;
      default:
	if (FL_TEST(obj, FL_EXIVAR) || rb_special_const_p(obj))
	    return generic_ivar_defined(obj, id);
	break;
    }
    return Qfalse;
}

static int
ivar_i(key, entry, ary)
    ID key;
    struct global_entry *entry;
    VALUE ary;
{
    if (rb_is_instance_id(key)) {
	rb_ary_push(ary, rb_str_new2(rb_id2name(key)));
    }
    return ST_CONTINUE;
}

VALUE
rb_obj_instance_variables(obj)
    VALUE obj;
{
    VALUE ary;

    ary = rb_ary_new();
    switch (TYPE(obj)) {
      case T_OBJECT:
      case T_CLASS:
      case T_MODULE:
	if (ROBJECT(obj)->iv_tbl) {
	    st_foreach(ROBJECT(obj)->iv_tbl, ivar_i, ary);
	}
	break;
      default:
	if (!generic_iv_tbl) break;
	if (FL_TEST(obj, FL_EXIVAR) || rb_special_const_p(obj)) {
	    st_table *tbl;

	    if (st_lookup(generic_iv_tbl, obj, &tbl)) {
		st_foreach(tbl, ivar_i, ary);
	    }
	}
	break;
    }
    return ary;
}

VALUE
rb_obj_remove_instance_variable(obj, name)
    VALUE obj, name;
{
    VALUE val = Qnil;
    ID id = rb_to_id(name);

    if (!OBJ_TAINTED(obj) && rb_safe_level() >= 4)
	rb_raise(rb_eSecurityError, "Insecure: can't modify instance variable");
    if (OBJ_FROZEN(obj)) rb_error_frozen("object");
    if (!rb_is_instance_id(id)) {
	rb_name_error(id, "`%s' is not an instance variable", rb_id2name(id));
    }

    switch (TYPE(obj)) {
      case T_OBJECT:
      case T_CLASS:
      case T_MODULE:
	if (ROBJECT(obj)->iv_tbl && st_delete(ROBJECT(obj)->iv_tbl, &id, &val)) {
	    return val;
	}
	break;
      default:
	if (FL_TEST(obj, FL_EXIVAR) || rb_special_const_p(obj)) {
	    if (generic_ivar_remove(obj, id, &val)) {
		return val;
	    }
	}
	break;
    }
    rb_name_error(id, "instance variable %s not defined", rb_id2name(id));
    return Qnil;		/* not reached */
}

static int
top_const_get(id, klassp)
    ID id;
    VALUE *klassp;
{
    /* pre-defined class */
    if (st_lookup(rb_class_tbl, id, klassp)) return Qtrue;

    /* autoload */
    if (autoload_tbl && st_lookup(autoload_tbl, id, 0)) {
	rb_autoload_load(id);
	*klassp = rb_const_get(rb_cObject, id);
	return Qtrue;
    }
    return Qfalse;
}

VALUE
rb_const_get_at(klass, id)
    VALUE klass;
    ID id;
{
    VALUE value;

    if (RCLASS(klass)->iv_tbl && st_lookup(RCLASS(klass)->iv_tbl, id, &value)) {
	return value;
    }
    if (klass == rb_cObject && top_const_get(id, &value)) {
	return value;
    }
    rb_name_error(id, "uninitialized constant %s::%s",
		  RSTRING(rb_class_path(klass))->ptr,
		  rb_id2name(id));
    return Qnil;		/* not reached */
}

void
rb_autoload_load(id)
    ID id;
{
    char *modname;
    VALUE module;

    st_delete(autoload_tbl, &id, &modname);
    if (rb_provided(modname)) {
	free(modname);
	return;
    }
    module = rb_str_new2(modname);
    free(modname);
    FL_UNSET(module, FL_TAINT);
    rb_f_require(Qnil, module);
}

VALUE
rb_const_get(klass, id)
    VALUE klass;
    ID id;
{
    VALUE value, tmp;
    int mod_retry = 0;

    tmp = klass;
  retry:
    while (tmp) {
	if (RCLASS(tmp)->iv_tbl && st_lookup(RCLASS(tmp)->iv_tbl,id,&value)) {
	    return value;
	}
	if (tmp == rb_cObject && top_const_get(id, &value)) return value;
	tmp = RCLASS(tmp)->super;
    }
    if (!mod_retry && BUILTIN_TYPE(klass) == T_MODULE) {
	mod_retry = 1;
	tmp = rb_cObject;
	goto retry;
    }

    /* Uninitialized constant */
    if (klass && klass != rb_cObject) {
	rb_name_error(id, "uninitialized constant %s at %s",
		      rb_id2name(id),
		      RSTRING(rb_class_path(klass))->ptr);
    }
    else { /* global_uninitialized */
	rb_name_error(id, "uninitialized constant %s",rb_id2name(id));
    }
    return Qnil;		/* not reached */
}

VALUE
rb_mod_remove_const(mod, name)
    VALUE mod, name;
{
    ID id = rb_to_id(name);
    VALUE val;

    if (!rb_is_const_id(id)) {
	rb_name_error(id, "`%s' is not constant", rb_id2name(id));
    }
    if (!OBJ_TAINTED(mod) && rb_safe_level() >= 4)
	rb_raise(rb_eSecurityError, "Insecure: can't remove constant");
    if (OBJ_FROZEN(mod)) rb_error_frozen("class/module");

    if (RCLASS(mod)->iv_tbl && st_delete(ROBJECT(mod)->iv_tbl, &id, &val)) {
	return val;
    }
    if (rb_const_defined_at(mod, id)) {
	rb_name_error(id, "cannot remove %s::%s", 
		 rb_class2name(mod), rb_id2name(id));
    }
    rb_name_error(id, "constant %s::%s not defined", 
		  rb_class2name(mod), rb_id2name(id));
    return Qnil;		/* not reached */
}

static int
sv_i(key, value, tbl)
    ID key;
    VALUE value;
    st_table *tbl;
{
    if (rb_is_const_id(key)) {
	if (!st_lookup(tbl, key, 0)) {
	    st_insert(tbl, key, key);
	}
    }
    return ST_CONTINUE;
}

static int
autoload_i(key, name, tbl)
    ID key;
    const char *name;
    st_table *tbl;
{
    if (!st_lookup(tbl, key, 0)) {
	st_insert(tbl, key, key);
    }
    return ST_CONTINUE;
}

void*
rb_mod_const_at(mod, data)
    VALUE mod;
    void *data;
{
    st_table *tbl = data;
    if (!tbl) {
	tbl = st_init_numtable();
    }
    if (RCLASS(mod)->iv_tbl) {
	st_foreach(RCLASS(mod)->iv_tbl, sv_i, tbl);
    }
    if ((VALUE)mod == rb_cObject) {
	st_foreach(rb_class_tbl, sv_i, tbl);
	if (autoload_tbl) {
	    st_foreach(autoload_tbl, autoload_i, tbl);
	}
    }
    return tbl;
}

void*
rb_mod_const_of(mod, data)
    VALUE mod;
    void *data;
{
    for (;;) {
	data = rb_mod_const_at(mod, data);
	mod = RCLASS(mod)->super;
	if (!mod) break;
    }
    return data;
}

static int
list_i(key, value, ary)
    ID key, value;
    VALUE ary;
{
    rb_ary_push(ary, rb_str_new2(rb_id2name(key)));
    return ST_CONTINUE;
}

VALUE
rb_const_list(data)
    void *data;
{
    st_table *tbl = data;
    VALUE ary;

    if (!tbl) return rb_ary_new2(0);
    ary = rb_ary_new2(tbl->num_entries);
    st_foreach(tbl, list_i, ary);
    st_free_table(tbl);

    return ary;
}

VALUE
rb_mod_constants(mod)
    VALUE mod;
{
    return rb_const_list(rb_mod_const_of(mod, 0));
}

int
rb_const_defined_at(klass, id)
    VALUE klass;
    ID id;
{
    if (RCLASS(klass)->iv_tbl && st_lookup(RCLASS(klass)->iv_tbl, id, 0)) {
	return Qtrue;
    }
    if (klass == rb_cObject) {
	return rb_const_defined(klass, id);
    }
    return Qfalse;
}

int
rb_autoload_defined(id)
    ID id;
{
    if (autoload_tbl && st_lookup(autoload_tbl, id, 0))
	return Qtrue;
    return Qfalse;
}

int
rb_const_defined(klass, id)
    VALUE klass;
    ID id;
{
    VALUE tmp = klass;

    while (tmp) {
	if (RCLASS(tmp)->iv_tbl && st_lookup(RCLASS(tmp)->iv_tbl,id,0)) {
	    return Qtrue;
	}
	tmp = RCLASS(tmp)->super;
    }
    if (BUILTIN_TYPE(klass) == T_MODULE) {
	return rb_const_defined(rb_cObject, id);
    }
    if (st_lookup(rb_class_tbl, id, 0))
	return Qtrue;
    return rb_autoload_defined(id);
}

static void
mod_av_set(klass, id, val, isconst)
    VALUE klass;
    ID id;
    VALUE val;
    int isconst;
{
    char *dest = isconst ? "constant" : "class variable";

    if (!OBJ_TAINTED(klass) && rb_safe_level() >= 4)
	rb_raise(rb_eSecurityError, "Insecure: can't set %s", dest);
    if (OBJ_FROZEN(klass)) rb_error_frozen("class/module");
    if (!RCLASS(klass)->iv_tbl) {
	RCLASS(klass)->iv_tbl = st_init_numtable();
    }
    else if (isconst) {
	if (st_lookup(RCLASS(klass)->iv_tbl, id, 0) ||
	    (klass == rb_cObject && st_lookup(rb_class_tbl, id, 0))) {
	    rb_warn("already initialized %s %s", dest, rb_id2name(id));
	}
    }

    st_insert(RCLASS(klass)->iv_tbl, id, val);
}

void
rb_const_set(klass, id, val)
    VALUE klass;
    ID id;
    VALUE val;
{
    mod_av_set(klass, id, val, Qtrue);
}

void
rb_const_assign(klass, id, val)
    VALUE klass;
    ID id;
    VALUE val;
{
    VALUE tmp = klass;
    
    while (tmp) {
	if (RCLASS(tmp)->iv_tbl && st_lookup(RCLASS(tmp)->iv_tbl,id,0)) {
	    st_insert(RCLASS(tmp)->iv_tbl, id, val);
	    return;
	}
	tmp = RCLASS(tmp)->super;
    }
    /* pre-defined class */
    if (st_lookup(rb_class_tbl, id, 0)) {
	st_delete(rb_class_tbl, &id, 0);
	st_insert(RCLASS(rb_cObject)->iv_tbl, id, val);
	return;
    }

    /* autoload */
    if (autoload_tbl && st_lookup(autoload_tbl, id, 0)) {
	char *modname;

	st_delete(autoload_tbl, &id, &modname);
	free(modname);
	st_insert(RCLASS(rb_cObject)->iv_tbl, id, val);
	return;
    }

    /* Uninitialized constant */
    if (klass && klass != rb_cObject)
	rb_name_error(id, "uninitialized constant %s::%s",
		      RSTRING(rb_class_path(klass))->ptr,
		      rb_id2name(id));
    else {
	rb_name_error(id, "uninitialized constant %s",rb_id2name(id));
    }
}

void
rb_define_const(klass, name, val)
    VALUE klass;
    const char *name;
    VALUE val;
{
    ID id = rb_intern(name);

    if (klass == rb_cObject) {
	rb_secure(4);
    }
    rb_const_set(klass, id, val);
}

void
rb_define_global_const(name, val)
    const char *name;
    VALUE val;
{
    rb_define_const(rb_cObject, name, val);
}

static VALUE
original_module(c)
    VALUE c;
{
    if (TYPE(c) == T_ICLASS)
	return RBASIC(c)->klass;
    return c;
}

static void
cvar_override_check(id, a)
    ID id;
    VALUE a;
{
    VALUE base = original_module(a);

    a = RCLASS(a)->super;
    while (a) {
	if (RCLASS(a)->iv_tbl) {
	    if (st_lookup(RCLASS(a)->iv_tbl,id,0)) {
		rb_warning("class variable %s of %s is overridden by %s",
			   rb_id2name(id), rb_class2name(original_module(a)),
			   rb_class2name(base));
	    }
	}
	a = RCLASS(a)->super;
    }
}

void
rb_cvar_set(klass, id, val, warn)
    VALUE klass;
    ID id;
    VALUE val;
    int warn;
{
    VALUE tmp;

    tmp = klass;
    while (tmp) {
	if (RCLASS(tmp)->iv_tbl && st_lookup(RCLASS(tmp)->iv_tbl,id,0)) {
	    if (OBJ_FROZEN(tmp)) rb_error_frozen("class/module");
	    if (!OBJ_TAINTED(tmp) && rb_safe_level() >= 4)
		rb_raise(rb_eSecurityError, "Insecure: can't modify class variable");
	    if (warn && RTEST(ruby_verbose) && klass != tmp) {
		rb_warning("already initialized class variable %s", rb_id2name(id));
	    }
	    st_insert(RCLASS(tmp)->iv_tbl,id,val);
	    if (RTEST(ruby_verbose)) {
		cvar_override_check(id, tmp);
	    }
	    return;
	}
	tmp = RCLASS(tmp)->super;
    }

    mod_av_set(klass, id, val, Qfalse);
}

VALUE
rb_cvar_get(klass, id)
    VALUE klass;
    ID id;
{
    VALUE value;
    VALUE tmp;

    tmp = klass;
    while (tmp) {
	if (RCLASS(tmp)->iv_tbl) {
	    if (st_lookup(RCLASS(tmp)->iv_tbl,id,&value)) {
		if (RTEST(ruby_verbose)) {
		    cvar_override_check(id, tmp);
		}
		return value;
	    }
	}
	tmp = RCLASS(tmp)->super;
    }

    rb_name_error(id,"uninitialized class variable %s in %s",
		  rb_id2name(id), rb_class2name(klass));
    return Qnil;		/* not reached */
}

VALUE
rb_cvar_defined(klass, id)
    VALUE klass;
    ID id;
{
    VALUE tmp;

    tmp = klass;
    while (tmp) {
	if (RCLASS(tmp)->iv_tbl && st_lookup(RCLASS(tmp)->iv_tbl,id,0)) {
	    return Qtrue;
	}
	tmp = RCLASS(tmp)->super;
    }

    return Qfalse;
}

void
rb_cv_set(klass, name, val)
    VALUE klass;
    const char *name;
    VALUE val;
{
    ID id = rb_intern(name);
    if (!rb_is_class_id(id)) {
	rb_name_error(id, "wrong class variable name %s", name);
    }
    rb_cvar_set(klass, id, val, Qfalse);
}

VALUE
rb_cv_get(klass, name)
    VALUE klass;
    const char *name;
{
    ID id = rb_intern(name);
    if (!rb_is_class_id(id)) {
	rb_name_error(id, "wrong class variable name %s", name);
    }
    return rb_cvar_get(klass, id);
}

void
rb_define_class_variable(klass, name, val)
    VALUE klass;
    const char *name;
    VALUE val;
{
    ID id = rb_intern(name);

    if (!rb_is_class_id(id)) {
	rb_name_error(id, "wrong class variable name %s", name);
    }
    rb_cvar_set(klass, id, val, Qtrue);
}

static int
cv_i(key, value, ary)
    ID key;
    VALUE value;
    VALUE ary;
{
    if (rb_is_class_id(key)) {
	VALUE kval = rb_str_new2(rb_id2name(key));
	if (!rb_ary_includes(ary, kval)) {
	    rb_ary_push(ary, kval);
	}
    }
    return ST_CONTINUE;
}

VALUE
rb_mod_class_variables(obj)
    VALUE obj;
{
    VALUE ary = rb_ary_new();

    for (;;) {
	if (RCLASS(obj)->iv_tbl) {
	    st_foreach(RCLASS(obj)->iv_tbl, cv_i, ary);
	}
	obj = RCLASS(obj)->super;
	if (!obj) break;
    }
    return ary;
}

VALUE
rb_mod_remove_cvar(mod, name)
    VALUE mod, name;
{
    ID id = rb_to_id(name);
    VALUE val;

    if (!rb_is_class_id(id)) {
	rb_name_error(id, "wrong class variable name %s", rb_id2name(id));
    }
    if (!OBJ_TAINTED(mod) && rb_safe_level() >= 4)
	rb_raise(rb_eSecurityError, "Insecure: can't remove class variable");
    if (OBJ_FROZEN(mod)) rb_error_frozen("class/module");

    if (RCLASS(mod)->iv_tbl && st_delete(ROBJECT(mod)->iv_tbl, &id, &val)) {
	return val;
    }
    if (rb_cvar_defined(mod, id)) {
	rb_name_error(id, "cannot remove %s for %s", 
		 rb_id2name(id), rb_class2name(mod));
    }
    rb_name_error(id, "class variable %s not defined for %s",
		  rb_id2name(id), rb_class2name(mod));
    return Qnil;		/* not reached */
}

VALUE
rb_iv_get(obj, name)
    VALUE obj;
    const char *name;
{
    ID id = rb_intern(name);

    return rb_ivar_get(obj, id);
}

VALUE
rb_iv_set(obj, name, val)
    VALUE obj;
    const char *name;
    VALUE val;
{
    ID id = rb_intern(name);

    return rb_ivar_set(obj, id, val);
}
