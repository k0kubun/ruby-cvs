/**********************************************************************

  regex.h - OniGuruma (regular expression library)

  Copyright (C) 2002  K.Kosako (kosako@sofnec.co.jp)

**********************************************************************/
#ifndef REGEX_H
#define REGEX_H

#define ONIGURUMA
#define ONIGURUMA_VERSION           130    /* 1.3 */

/* config parameters */
#ifndef RE_NREGS
#define RE_NREGS                     10
#endif
#define REG_NREGION            RE_NREGS
#define REG_MAX_BACKREF_NUM        1000
#define REG_MAX_REPEAT_NUM       100000
#define REG_CHAR_TABLE_SIZE         256

#define REGCODE_UNDEF         ((RegCharEncoding )0)

#if defined(RUBY_PLATFORM) && defined(M17N_H)
#define REG_RUBY_M17N
typedef m17n_encoding*        RegCharEncoding;
#define REGCODE_DEFAULT       REGCODE_UNDEF
#else
typedef const char*           RegCharEncoding;
#define MBCTYPE_ASCII         0
#define MBCTYPE_EUC           1
#define MBCTYPE_SJIS          2
#define MBCTYPE_UTF8          3

#define REGCODE_ASCII         REG_MBLEN_TABLE[MBCTYPE_ASCII]
#define REGCODE_UTF8          REG_MBLEN_TABLE[MBCTYPE_UTF8]
#define REGCODE_EUCJP         REG_MBLEN_TABLE[MBCTYPE_EUC]
#define REGCODE_SJIS          REG_MBLEN_TABLE[MBCTYPE_SJIS]
#define REGCODE_DEFAULT       REGCODE_ASCII

extern const char REG_MBLEN_TABLE[][REG_CHAR_TABLE_SIZE];
#endif /* else RUBY && M17N */

extern RegCharEncoding RegDefaultCharEncoding;

#if defined(RUBY_PLATFORM) && !defined(M17N_H)
#undef ismbchar
#define ismbchar(c)    (mbclen((c)) != 1)
#define mbclen(c)      RegDefaultCharEncoding[(unsigned char )(c)]
#endif

/* argument values for regex_new(), regex_recompile() */
#define REG_TRANSTABLE_USE_DEFAULT   ((RegTransTableType )0)
#define REG_OPTION_DEFAULT            REG_OPTION_NONE

/* GNU regex options */
#define RE_OPTION_IGNORECASE    (1L)
#define RE_OPTION_EXTENDED      (RE_OPTION_IGNORECASE << 1)
#define RE_OPTION_MULTILINE     (RE_OPTION_EXTENDED   << 1)
#define RE_OPTION_SINGLELINE    (RE_OPTION_MULTILINE  << 1)
#define RE_OPTION_POSIXLINE     (RE_OPTION_MULTILINE|RE_OPTION_SINGLELINE)
#define RE_OPTION_LONGEST       (RE_OPTION_SINGLELINE << 1)

/* options */
#define REG_OPTION_NONE             0
#define REG_OPTION_SINGLELINE       RE_OPTION_SINGLELINE
#define REG_OPTION_MULTILINE        RE_OPTION_MULTILINE
#define REG_OPTION_IGNORECASE       RE_OPTION_IGNORECASE
#define REG_OPTION_EXTEND           RE_OPTION_EXTENDED
#define REG_OPTION_FIND_LONGEST     RE_OPTION_LONGEST
#define REG_OPTION_FIND_NOT_EMPTY   (REG_OPTION_FIND_LONGEST   << 1)
/* options (search time) */
#define REG_OPTION_NOTBOL           (REG_OPTION_FIND_NOT_EMPTY << 1)
#define REG_OPTION_NOTEOL           (REG_OPTION_NOTBOL         << 1)

#define REG_OPTION_ON(options,regopt)      ((options) |= (regopt))
#define REG_OPTION_OFF(options,regopt)     ((options) &= ~(regopt))
#define IS_REG_OPTION_ON(options,option)   ((options) & (option))

/* error codes */
#define REG_IS_PATTERN_ERROR(ecode)   ((ecode) <= -100 && (ecode) > -300)
/* normal return */
#define REG_NORMAL                                             0
#define REG_MISMATCH                                          -1
/* internal error */
#define REGERR_MEMORY                                         -2
#define REGERR_TYPE_BUG                                      -10
#define REGERR_STACK_BUG                                     -11
#define REGERR_UNDEFINED_BYTECODE                            -12
#define REGERR_UNEXPECTED_BYTECODE                           -13
#define REGERR_TABLE_FOR_IGNORE_CASE_IS_NOT_SETTED           -20
#define REGERR_DEFAULT_ENCODING_IS_NOT_SETTED                -21
#define REGERR_SPECIFIED_ENCODING_CANT_CONVERT_TO_WIDE_CHAR  -22
/* syntax error */
#define REGERR_END_PATTERN_AT_LEFT_BRACE                    -100
#define REGERR_END_PATTERN_AT_LEFT_BRACKET                  -101
#define REGERR_EMPTY_CHAR_CLASS                             -102
#define REGERR_PREMATURE_END_OF_CHAR_CLASS                  -103
#define REGERR_END_PATTERN_AT_BACKSLASH                     -104
#define REGERR_END_PATTERN_AT_META                          -105
#define REGERR_END_PATTERN_AT_CONTROL                       -106
#define REGERR_END_PATTERN_AFTER_BACKSLASH                  -107
#define REGERR_META_CODE_SYNTAX                             -108
#define REGERR_CONTROL_CODE_SYNTAX                          -109
#define REGERR_CHAR_CLASS_VALUE_AT_END_OF_RANGE             -110
#define REGERR_CHAR_CLASS_VALUE_AT_START_OF_RANGE           -111
#define REGERR_UNMATCHED_RANGE_SPECIFIER_IN_CHAR_CLASS      -112
#define REGERR_TARGET_OF_REPEAT_OPERATOR_NOT_SPECIFIED      -113
#define REGERR_TARGET_OF_REPEAT_OPERATOR_INVALID            -114
#define REGERR_NESTED_REPEAT_OPERATOR                       -115
#define REGERR_UNMATCHED_RIGHT_PARENTHESIS                  -116
#define REGERR_END_PATTERN_WITH_UNMATCHED_PARENTHESIS       -117
#define REGERR_END_PATTERN_AT_GROUP_OPTION                  -118
#define REGERR_UNDEFINED_GROUP_OPTION                       -119
#define REGERR_END_PATTERN_AT_GROUP_COMMENT                 -120
#define REGERR_INVALID_POSIX_BRACKET_TYPE                   -121
#define REGERR_INVALID_LOOK_BEHIND_PATTERN                  -122
/* values error */
#define REGERR_TOO_BIG_NUMBER                               -200
#define REGERR_TOO_BIG_NUMBER_FOR_REPEAT_RANGE              -201
#define REGERR_UPPER_SMALLER_THAN_LOWER_IN_REPEAT_RANGE     -202
#define REGERR_RIGHT_SMALLER_THAN_LEFT_IN_CLASS_RANGE       -203
#define REGERR_MISMATCH_CODE_LENGTH_IN_CLASS_RANGE          -204
#define REGERR_TOO_MANY_MULTI_BYTE_RANGES                   -205
#define REGERR_TOO_SHORT_MULTI_BYTE_STRING                  -206
#define REGERR_TOO_BIG_BACKREF_NUMBER                       -207
#define REGERR_TOO_BIG_WIDE_CHAR_VALUE                      -208
#define REGERR_TOO_LONG_WIDE_CHAR_VALUE                     -209
#define REGERR_INVALID_WIDE_CHAR_VALUE                      -210

/* match result region type */
struct re_registers {
  int  allocated;
  int  num_regs;
  int* beg;
  int* end;
};

typedef struct re_registers   RegRegion;
typedef unsigned int          RegOptionType;
typedef unsigned char*        RegTransTableType;
typedef unsigned int          RegDistance;
typedef unsigned char         UChar;

/* regex_t state */
#define REG_STATE_NORMAL              0
#define REG_STATE_SEARCHING           1
#define REG_STATE_COMPILING          -1
#define REG_STATE_MODIFY             -2

#define REG_STATE(regex) \
  ((regex)->state > 0 ? REG_STATE_SEARCHING : (regex)->state)

typedef struct re_pattern_buffer {
  /* common members in BBuf(bytes-buffer) type */
  unsigned char* p;         /* compiled pattern */
  unsigned int used;        /* used space for p */
  unsigned int alloc;       /* allocated space for p */

  int state;                /* normal, searching, compiling */
  int max_mem;              /* used memory(...) num counted from 1 */
  int num_repeat;           /* OP_REPEAT/OP_REPEAT_NG id-counter */
  int num_null_check;       /* OP_NULL_CHECK_START/END id counter */
  unsigned int mem_stats;   /* mem:n -> n-bit flag (n:1-31)
                              (backref-ed or must be cleared in backtrack) */
  RegCharEncoding   code;
  RegOptionType     options;
  RegTransTableType transtable;  /* char-case trans table */

  /* optimize info (string search and char-map and anchor) */
  int            optimize;          /* optimize flag */
  int            threshold_len;     /* search str-length for apply optimize */
  int            anchor;            /* BEGIN_BUF, BEGIN_POS, (SEMI_)END_BUF */
  RegDistance    anchor_dmin;       /* (SEMI_)END_BUF anchor distance */
  RegDistance    anchor_dmax;       /* (SEMI_)END_BUF anchor distance */
  int            sub_anchor;        /* start-anchor for exact or map */
  unsigned char *exact;
  unsigned char *exact_end;
  unsigned char  map[REG_CHAR_TABLE_SIZE];  /* used as BM skip or char-map */
  int           *int_map;                   /* BM skip for exact_len > 255 */
  int           *int_map_backward;          /* BM skip for backward search */
  RegDistance    dmin;                      /* min-distance of exact or map */
  RegDistance    dmax;                      /* max-distance of exact or map */

  /* regex_t link chain */
  struct re_pattern_buffer* chain; /* escape compile-conflict on multi-thread */
} regex_t;

#ifdef RUBY_PLATFORM
#define re_mbcinit              ruby_re_mbcinit
#define re_compile_pattern      ruby_re_compile_pattern
#define re_recompile_pattern    ruby_re_recompile_pattern
#define re_free_pattern         ruby_re_free_pattern
#define re_adjust_startpos      ruby_re_adjust_startpos
#define re_search               ruby_re_search
#define re_match                ruby_re_match
#define re_set_casetable        ruby_re_set_casetable
#define re_copy_registers       ruby_re_copy_registers
#define re_free_registers       ruby_re_free_registers
#define register_info_type      ruby_register_info_type
#define re_error_code_to_str    ruby_error_code_to_str

#define ruby_error_code_to_str  regex_error_code_to_str
#define ruby_re_copy_registers  regex_region_copy
#else
#define re_error_code_to_str    regex_error_code_to_str
#define re_copy_registers       regex_region_copy
#endif


/* Native API */
#ifdef __STDC__
extern int   regex_init(void);
extern char* regex_error_code_to_str(int err_code);
extern int   regex_new(regex_t** reg, UChar* pattern, UChar* pattern_end,
		RegOptionType option, RegCharEncoding code, UChar* transtable);
extern int   regex_clone(regex_t* to, regex_t* from);
extern void  regex_free(regex_t* reg);
extern int   regex_recompile(regex_t* reg, UChar* pattern, UChar* pattern_end,
		RegOptionType option, RegCharEncoding code, UChar* transtable);
extern int   regex_search(regex_t* reg, UChar* str, UChar* end,
			  UChar* start, UChar* range, RegRegion* region,
			  RegOptionType option);
extern int   regex_match(regex_t* reg, UChar* str, UChar* end, UChar* at,
			 RegRegion* region, RegOptionType option);
extern RegRegion* regex_region_new(void);
extern void  regex_region_free(RegRegion* region, int free_self);
extern void  regex_region_copy(RegRegion* r1, RegRegion* r2);
extern void  regex_region_clear(RegRegion* region);
extern int   regex_region_resize(RegRegion* region, int n);
extern UChar* regex_get_prev_char_head(RegCharEncoding code,
				       UChar* start, UChar* s);
extern UChar* regex_get_left_adjust_char_head(RegCharEncoding code,
					      UChar* start, UChar* s);
extern UChar* regex_get_right_adjust_char_head(RegCharEncoding code,
					       UChar* start, UChar* s);
extern void  regex_set_default_trans_table(UChar* table);
extern int   regex_end(void);

#else

extern int    regex_init();
extern char*  regex_error_code_to_str();
extern int    regex_new();
extern int    regex_clone();
extern void   regex_free();
extern int    regex_recompile();
extern int    regex_search();
extern int    regex_match();
extern RegRegion* regex_region_new();
extern void   regex_region_free();
extern void   regex_region_copy();
extern void   regex_region_clear();
extern int    regex_region_resize();
extern UChar* regex_get_prev_char_head();
extern UChar* regex_get_left_adjust_char_head();
extern UChar* regex_get_right_adjust_char_head();
extern void   regex_set_default_trans_table();
extern int    regex_end();
#endif

/* GNU regex compatible API */
#ifdef __STDC__

#ifdef REG_RUBY_M17N
extern void  re_mbcinit(RegCharEncoding);
#else
extern void  re_mbcinit(int);
#endif

extern char* re_compile_pattern(const char*, int, struct re_pattern_buffer*);
extern char* re_recompile_pattern(const char*, int, struct re_pattern_buffer*);
extern void  re_free_pattern(struct re_pattern_buffer*);
extern int   re_adjust_startpos(struct re_pattern_buffer*, const char*,
				int, int, int);
extern int   re_search(struct re_pattern_buffer*, const char*, int, int, int,
		       struct re_registers*);
extern int   re_match(struct re_pattern_buffer*, const char *, int, int,
		      struct re_registers*);
extern void  re_set_casetable(const char*);
extern void  re_free_registers(struct re_registers*);
extern int   re_alloc_pattern(struct re_pattern_buffer**);  /* added */

#else

extern void  re_mbcinit();
extern char* re_compile_pattern();
extern char* re_recompile_pattern();  /* added */
extern void  re_free_pattern();
extern int   re_adjust_startpos();
extern int   re_search();
extern int   re_match();
extern void  re_set_casetable();
extern void  re_free_registers();
extern int   re_alloc_pattern();  /* added */

#endif /* __STDC__ */

#endif /* REGEX_H */
