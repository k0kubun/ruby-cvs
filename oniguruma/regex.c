/**********************************************************************

  regex.c -  OniGuruma (regular expression library)

  Copyright (C) 2002  K.Kosako (kosako@sofnec.co.jp)

**********************************************************************/
/* for debug */
/* #define REG_DEBUG_PARSE_TREE */
/* #define REG_DEBUG_COMPILE */
/* #define REG_DEBUG_SEARCH */
/* #define REG_DEBUG_MATCH */
/* #define REG_DONT_OPTIMIZE */

/* for byte-code statistical data. */
/* #define REG_DEBUG_STATISTICS */

#if defined(REG_DEBUG_PARSE_TREE) || defined(REG_DEBUG_MATCH) || \
    defined(REG_DEBUG_COMPILE) || defined(REG_DEBUG_STATISTICS)
#ifndef REG_DEBUG
#define REG_DEBUG
#endif
#endif

#if defined(__i386) || defined(__i386__) || defined(_M_IX86) || \
    (defined(__ppc__) && defined(__APPLE__)) || \
    defined(__mc68020__)
#define UNALIGNED_WORD_ACCESS
#endif

/* config */
#define USE_RECYCLE_NODE
#define USE_ERROR_MESSAGE_STRING
/* convert [push, exact...] --> [push_or_jump_exact, exact...] */
#define USE_OP_PUSH_OR_JUMP_EXACT
#define USE_QUALIFIER_PEEK_NEXT
#define INIT_MATCH_STACK_SIZE                  160
#define POSITION_SEARCH_MIN_THRESHOLD_LENGTH     7


/* include headers */
#include "config.h"

/* interface to external system */
#if defined(RUBY_PLATFORM) || defined(RUBY)
#include "ruby.h"
#include "rubysig.h"      /* for DEFER_INTS, ENABLE_INTS */
#define THREAD_ATOMIC_START     DEFER_INTS
#define THREAD_ATOMIC_END       ENABLE_INTS
#else
#define THREAD_ATOMIC_START
#define THREAD_ATOMIC_END
#define xmalloc     malloc
#define xrealloc    realloc
#define xfree       free

/* non Ruby platform default, you can change. */
#define USE_ONIGURUMA_EXTENSION
#define USE_WORD_BEGIN_END          /* "\<": word-begin, "\>": word-end */
#define DEFAULT_TRANSTABLE_EXIST    1
#endif

#define xmemset     memset
#define xmemcpy     memcpy
#define xmemmove    memmove
#define xalloca     alloca

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#if defined(HAVE_ALLOCA_H) && !defined(__GNUC__)
#include <alloca.h>
#endif

#ifdef HAVE_STRING_H
# include <string.h>
#else
# include <strings.h>
#endif

#include <ctype.h>
#include <sys/types.h>

#ifdef REG_DEBUG
# include <stdio.h>
#endif

#include "regex.h"

#if defined(RUBY_PLATFORM) || defined(RUBY)
#ifndef REG_RUBY_M17N
#define USE_COMPATIBILITY_FOR_RUBY_EXTENSION_LIBRARY
#endif
#endif

extern char*
regex_error_code_to_str(int code)
{
  char *p;

  if (code >= 0) return NULL;

#ifdef USE_ERROR_MESSAGE_STRING
  switch (code) {
  case REG_MISMATCH:
    p = "mismatch"; break;
  case REGERR_MEMORY:
    p = "fail to memory allocation"; break;
  case REGERR_TYPE_BUG:
    p = "undefined type (bug)"; break;
  case REGERR_STACK_BUG:
    p = "stack error (bug)"; break;
  case REGERR_UNDEFINED_BYTECODE:
    p = "undefined bytecode (bug)"; break;
  case REGERR_UNEXPECTED_BYTECODE:
    p = "unexpected bytecode (bug)"; break;
  case REGERR_TABLE_FOR_IGNORE_CASE_IS_NOT_SETTED:
    p = "table for ignore-case is not setted"; break;
  case REGERR_DEFAULT_ENCODING_IS_NOT_SETTED:
    p = "default multibyte-encoding is not setted"; break;
  case REGERR_SPECIFIED_ENCODING_CANT_CONVERT_TO_WIDE_CHAR:
    p = "can't convert to wide-char on specified multibyte-encoding"; break;
  case REGERR_END_PATTERN_AT_LEFT_BRACE:
    p = "end pattern at left brace"; break;
  case REGERR_END_PATTERN_AT_LEFT_BRACKET:
    p = "end pattern at left bracket"; break;
  case REGERR_EMPTY_CHAR_CLASS:
    p = "empty char-class"; break;
  case REGERR_PREMATURE_END_OF_CHAR_CLASS:
    p = "premature end of char-class"; break;
  case REGERR_END_PATTERN_AT_BACKSLASH:
    p = "end pattern at backslash"; break;
  case REGERR_END_PATTERN_AT_META:
    p = "end pattern at meta"; break;
  case REGERR_END_PATTERN_AT_CONTROL:
    p = "end pattern at control"; break;
  case REGERR_END_PATTERN_AFTER_BACKSLASH:
    p = "end pattern after backslash"; break;
  case REGERR_META_CODE_SYNTAX:
    p = "illegal meta-code syntax"; break;
  case REGERR_CONTROL_CODE_SYNTAX:
    p = "illegal control-code syntax"; break;
  case REGERR_CHAR_CLASS_VALUE_AT_END_OF_RANGE:
    p = "char-class value at end of range"; break;
  case REGERR_CHAR_CLASS_VALUE_AT_START_OF_RANGE:
    p = "char-class value at start of range"; break;
  case REGERR_UNMATCHED_RANGE_SPECIFIER_IN_CHAR_CLASS:
    p = "unmatched range specifier in char-class"; break;
  case REGERR_TARGET_OF_REPEAT_OPERATOR_NOT_SPECIFIED:
    p = "target of repeat operator is not specified"; break;
  case REGERR_TARGET_OF_REPEAT_OPERATOR_INVALID:
    p = "target of repeat operator is invalid"; break;
  case REGERR_NESTED_REPEAT_OPERATOR:
    p = "nested repeat operator"; break;
  case REGERR_UNMATCHED_RIGHT_PARENTHESIS:
    p = "unmatched right parenthesis"; break;
  case REGERR_END_PATTERN_WITH_UNMATCHED_PARENTHESIS:
    p = "end pattern with unmatched parenthesis"; break;
  case REGERR_END_PATTERN_AT_GROUP_OPTION:
    p = "end pattern at group option"; break;
  case REGERR_UNDEFINED_GROUP_OPTION:
    p = "undefined group option"; break;
  case REGERR_END_PATTERN_AT_GROUP_COMMENT:
    p = "end pattern at group comment"; break;
  case REGERR_INVALID_POSIX_BRACKET_TYPE:
    p = "invalid POSIX bracket type"; break;
  case REGERR_INVALID_LOOK_BEHIND_PATTERN:
    p = "invalid pattern in look-behind(?<)"; break;
  case REGERR_TOO_BIG_NUMBER:
    p = "too big number"; break;
  case REGERR_TOO_BIG_NUMBER_FOR_REPEAT_RANGE:
    p = "too big number for repeat range"; break;
  case REGERR_UPPER_SMALLER_THAN_LOWER_IN_REPEAT_RANGE:
    p = "upper is smaller than lower in repeat range"; break;
  case REGERR_RIGHT_SMALLER_THAN_LEFT_IN_CLASS_RANGE:
    p = "right is smaller than left in char-class range"; break;
  case REGERR_MISMATCH_CODE_LENGTH_IN_CLASS_RANGE:
    p = "mismatch multibyte code length in char-class range"; break;
  case REGERR_TOO_MANY_MULTI_BYTE_RANGES:
    p = "too many multibyte code ranges are specified"; break;
  case REGERR_TOO_SHORT_MULTI_BYTE_STRING:
    p = "too short multibyte code string"; break;
  case REGERR_TOO_BIG_BACKREF_NUMBER:
    p = "too big backref-number"; break;

  default:
    p = "undefined error code"; break;
  }
#else
  p = "error message is not supported";
#endif

  return p;
}


#ifndef UNALIGNED_WORD_ACCESS
#define WORD_ALIGNMENT_SIZE       SIZEOF_INT

#define GET_ALIGNMENT_PAD_SIZE(addr,pad_size) do {\
  (pad_size) = WORD_ALIGNMENT_SIZE - ((int )(addr) % WORD_ALIGNMENT_SIZE);\
  if ((pad_size) == WORD_ALIGNMENT_SIZE) (pad_size) = 0;\
} while (0)

#define ALIGNMENT_RIGHT(addr) do {\
  (addr) += (WORD_ALIGNMENT_SIZE - 1);\
  (addr) -= ((int )(addr) % WORD_ALIGNMENT_SIZE);\
} while (0)

static unsigned char PadBuf[WORD_ALIGNMENT_SIZE];

#define B_SHIFT  8
#define B_MASK   0xff

#define SERIALIZE_2BYTE_INT(i,p) do {\
  *(p)     = ((i) >> B_SHIFT) & B_MASK;\
  *((p)+1) = (i) & B_MASK;\
} while (0)

#define SERIALIZE_4BYTE_INT(i,p) do {\
  *(p)     = ((i) >> B_SHIFT*3) & B_MASK;\
  *((p)+1) = ((i) >> B_SHIFT*2) & B_MASK;\
  *((p)+2) = ((i) >> B_SHIFT  ) & B_MASK;\
  *((p)+3) = (i) & B_MASK;\
} while (0)

#define SERIALIZE_8BYTE_INT(i,p) do {\
  *(p)     = ((i) >> B_SHIFT*7) & B_MASK;\
  *((p)+1) = ((i) >> B_SHIFT*6) & B_MASK;\
  *((p)+2) = ((i) >> B_SHIFT*5) & B_MASK;\
  *((p)+3) = ((i) >> B_SHIFT*4) & B_MASK;\
  *((p)+4) = ((i) >> B_SHIFT*3) & B_MASK;\
  *((p)+5) = ((i) >> B_SHIFT*2) & B_MASK;\
  *((p)+6) = ((i) >> B_SHIFT  ) & B_MASK;\
  *((p)+7) = (i) & B_MASK;\
} while (0)

#define GET_2BYTE_INT_INC(type,i,p) do {\
  (i) = (type )(((unsigned int )(*(p)) << B_SHIFT) | (unsigned int )((p)[1]));\
  (p) += 2;\
} while (0)

#define GET_4BYTE_INT_INC(type,i,p) do {\
  (i) = (type )(((unsigned int )((p)[0]) << B_SHIFT*3) | \
		((unsigned int )((p)[1]) << B_SHIFT*2) | \
		((unsigned int )((p)[2]) << B_SHIFT  ) | \
		((unsigned int )((p)[3])             )); \
  (p) += 4;\
} while (0)

#define GET_8BYTE_INT_INC(type,i,p) do {\
  (i) = (type )(((unsigned long )((p)[0]) << B_SHIFT*7) | \
		((unsigned long )((p)[1]) << B_SHIFT*6) | \
		((unsigned long )((p)[2]) << B_SHIFT*5) | \
		((unsigned long )((p)[3]) << B_SHIFT*4) | \
		((unsigned long )((p)[4]) << B_SHIFT*3) | \
		((unsigned long )((p)[5]) << B_SHIFT*2) | \
		((unsigned long )((p)[6]) << B_SHIFT  ) | \
		((unsigned long )((p)[7])             )); \
  (p) += 8;\
} while (0)

#if SIZEOF_SHORT == 2
#define GET_SHORT_INC(i,p)     GET_2BYTE_INT_INC(short,i,p)
#define SERIALIZE_SHORT(i,p)   SERIALIZE_2BYTE_INT(i,p)
#elif SIZEOF_SHORT == 4
#define GET_SHORT_INC(i,p)     GET_4BYTE_INT_INC(short,i,p)
#define SERIALIZE_SHORT(i,p)   SERIALIZE_4BYTE_INT(i,p)
#elif SIZEOF_SHORT == 8
#define GET_SHORT_INC(i,p)     GET_8BYTE_INT_INC(short,i,p)
#define SERIALIZE_SHORT(i,p)   SERIALIZE_8BYTE_INT(i,p)
#endif

#if SIZEOF_INT == 2
#define GET_INT_INC(i,p)       GET_2BYTE_INT_INC(int,i,p)
#define GET_UINT_INC(i,p)      GET_2BYTE_INT_INC(unsigned,i,p)
#define SERIALIZE_INT(i,p)     SERIALIZE_2BYTE_INT(i,p)
#define SERIALIZE_UINT(i,p)    SERIALIZE_2BYTE_INT(i,p)
#elif SIZEOF_INT == 4
#define GET_INT_INC(i,p)       GET_4BYTE_INT_INC(int,i,p)
#define GET_UINT_INC(i,p)      GET_4BYTE_INT_INC(unsigned,i,p)
#define SERIALIZE_INT(i,p)     SERIALIZE_4BYTE_INT(i,p)
#define SERIALIZE_UINT(i,p)    SERIALIZE_4BYTE_INT(i,p)
#elif SIZEOF_INT == 8
#define GET_INT_INC(i,p)       GET_8BYTE_INT_INC(int,i,p)
#define GET_UINT_INC(i,p)      GET_8BYTE_INT_INC(unsigned,i,p)
#define SERIALIZE_INT(i,p)     SERIALIZE_8BYTE_INT(i,p)
#define SERIALIZE_UINT(i,p)    SERIALIZE_8BYTE_INT(i,p)
#endif

#endif /* UNALIGNED_WORD_ACCESS */


/* optimize flags */
#define REG_OPTIMIZE_NONE         0
#define REG_OPTIMIZE_EXACT        1     /* Slow Search */
#define REG_OPTIMIZE_EXACT_BM     2     /* Boyer Moore Search */
#define REG_OPTIMIZE_EXACT_IC     3     /* Slow Search (ignore case) */
#define REG_OPTIMIZE_MAP          5     /* char map */

/* mem stats */
#define MEM_STAT_BITS_NUM      (SIZEOF_INT * 8)
#define MEM_STATS_ON_AT(stats,n) do {\
  if ((n) < MEM_STAT_BITS_NUM)\
    (stats) |= (1 << (n));\
  else\
    (stats) |= 1;\
} while (0)

#define MEM_STATS_ON_ALL(stats)  (stats) = ~((unsigned int)0)

#define GET_MEM_STATS_AT(stats,n) \
  ((n) < MEM_STAT_BITS_NUM ?  ((stats) & (1 << n)) : ((stats) & 1))


#define INT_MAX_LIMIT           ((1UL << (SIZEOF_INT * 8 - 1)) - 1)

typedef unsigned int   WCInt;
#define SIZE_WCINT     sizeof(WCInt)

#define INFINITE_DISTANCE  ~((RegDistance )0)

static RegDistance
distance_add(RegDistance d1, RegDistance d2)
{
  if (d1 == INFINITE_DISTANCE || d2 == INFINITE_DISTANCE)
    return INFINITE_DISTANCE;
  else {
    if (d1 <= INFINITE_DISTANCE - d2) return d1 + d2;
    else return INFINITE_DISTANCE;
  }
}

static RegDistance
distance_multiply(RegDistance d, int m)
{
  if (d < INFINITE_DISTANCE / m)
    return d * m;
  else
    return INFINITE_DISTANCE;
}

static RegDistance
distance_distance(RegDistance d1, RegDistance d2)
{
  if (d1 == INFINITE_DISTANCE || d2 == INFINITE_DISTANCE)
    return INFINITE_DISTANCE;

  if (d1 > d2) return d1 - d2;
  else         return d2 - d1;
}


RegCharEncoding RegDefaultCharEncoding = REGCODE_DEFAULT;
static UChar*   DefaultTransTable = (UChar* )0;
static UChar    AmbiguityTable[REG_CHAR_TABLE_SIZE];

#define IS_AMBIGUITY_CHAR(trans, c) \
  (((trans) == DefaultTransTable) ? (AmbiguityTable[(c)] >= 2) : \
                                    (trans)[(c)+256] >= 2)

#ifdef DEFAULT_TRANSTABLE_EXIST

static UChar DTT[] = {
  '\000', '\001', '\002', '\003', '\004', '\005', '\006', '\007',
  '\010', '\011', '\012', '\013', '\014', '\015', '\016', '\017',
  '\020', '\021', '\022', '\023', '\024', '\025', '\026', '\027',
  '\030', '\031', '\032', '\033', '\034', '\035', '\036', '\037',
  '\040', '\041', '\042', '\043', '\044', '\045', '\046', '\047',
  '\050', '\051', '\052', '\053', '\054', '\055', '\056', '\057',
  '\060', '\061', '\062', '\063', '\064', '\065', '\066', '\067',
  '\070', '\071', '\072', '\073', '\074', '\075', '\076', '\077',
  '\100', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
  '\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
  '\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
  '\170', '\171', '\172', '\133', '\134', '\135', '\136', '\137',
  '\140', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
  '\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
  '\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
  '\170', '\171', '\172', '\173', '\174', '\175', '\176', '\177',
  '\200', '\201', '\202', '\203', '\204', '\205', '\206', '\207',
  '\210', '\211', '\212', '\213', '\214', '\215', '\216', '\217',
  '\220', '\221', '\222', '\223', '\224', '\225', '\226', '\227',
  '\230', '\231', '\232', '\233', '\234', '\235', '\236', '\237',
  '\240', '\241', '\242', '\243', '\244', '\245', '\246', '\247',
  '\250', '\251', '\252', '\253', '\254', '\255', '\256', '\257',
  '\260', '\261', '\262', '\263', '\264', '\265', '\266', '\267',
  '\270', '\271', '\272', '\273', '\274', '\275', '\276', '\277',
  '\300', '\301', '\302', '\303', '\304', '\305', '\306', '\307',
  '\310', '\311', '\312', '\313', '\314', '\315', '\316', '\317',
  '\320', '\321', '\322', '\323', '\324', '\325', '\326', '\327',
  '\330', '\331', '\332', '\333', '\334', '\335', '\336', '\337',
  '\340', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
  '\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357',
  '\360', '\361', '\362', '\363', '\364', '\365', '\366', '\367',
  '\370', '\371', '\372', '\373', '\374', '\375', '\376', '\377',
};
#endif


#if defined STDC_HEADERS || (!defined isascii && !defined HAVE_ISASCII)
# define IS_ASCII(c) 1
#else
# define IS_ASCII(c) isascii(c)
#endif

#ifdef isblank
# define IS_BLANK(c) (IS_ASCII(c) && isblank(c))
#else
# define IS_BLANK(c) ((c) == ' ' || (c) == '\t')
#endif
#ifdef isgraph
# define IS_GRAPH(c) (IS_ASCII(c) && isgraph(c))
#else
# define IS_GRAPH(c) (IS_ASCII(c) && isprint(c) && !isspace(c))
#endif

#define IS_PRINT(c)  (isprint(c)  && IS_ASCII(c))
#define IS_ALNUM(c)  (isalnum(c)  && IS_ASCII(c))
#define IS_ALPHA(c)  (isalpha(c)  && IS_ASCII(c))
#define IS_LOWER(c)  (islower(c)  && IS_ASCII(c))
#define IS_UPPER(c)  (isupper(c)  && IS_ASCII(c))
#define IS_CNTRL(c)  (iscntrl(c)  && IS_ASCII(c))
#define IS_PUNCT(c)  (ispunct(c)  && IS_ASCII(c))
#define IS_SPACE(c)  (isspace(c)  && IS_ASCII(c))
#define IS_DIGIT(c)  (isdigit(c)  && IS_ASCII(c))
#define IS_XDIGIT(c) (isxdigit(c) && IS_ASCII(c))
#define IS_ODIGIT(c) (IS_DIGIT(c) && (c) < '8')

#define DIGITVAL(c)    ((c) - '0')
#define ODIGITVAL(c)   DIGITVAL(c)
#define XDIGITVAL(c) \
  (IS_DIGIT(c) ? DIGITVAL(c) : (IS_UPPER(c) ? (c) - 'A' + 10 : (c) - 'a' + 10))

#define IS_SINGLELINE(option)     ((option) & REG_OPTION_SINGLELINE)
#define IS_MULTILINE(option)      ((option) & REG_OPTION_MULTILINE)
#define IS_IGNORECASE(option)     ((option) & REG_OPTION_IGNORECASE)
#define IS_EXTEND(option)         ((option) & REG_OPTION_EXTEND)
#define IS_FIND_LONGEST(option)   ((option) & REG_OPTION_FIND_LONGEST)
#define IS_FIND_NOT_EMPTY(option) ((option) & REG_OPTION_FIND_NOT_EMPTY)
#define IS_POSIXLINE(option)      (IS_SINGLELINE(option) && IS_MULTILINE(option))
#define IS_FIND_CONDITION(option) ((option) & \
          (REG_OPTION_FIND_LONGEST | REG_OPTION_FIND_NOT_EMPTY))

#ifdef NEWLINE
#undef NEWLINE
#endif
#define NEWLINE     '\n'
#define IS_NULL(p)                    (((void*)(p)) == (void*)0)
#define IS_NOT_NULL(p)                (((void*)(p)) != (void*)0)
#define IS_NEWLINE(c)                 ((c) == NEWLINE)
#define CHECK_NULL_RETURN(p)          if (IS_NULL(p)) return NULL
#define CHECK_NULL_RETURN_VAL(p,val)  if (IS_NULL(p)) return (val)

/* bitset */
#define BITS_PER_BYTE      8
#define SINGLE_BYTE_SIZE   (1 << BITS_PER_BYTE)
#define BITS_IN_ROOM       (sizeof(Bits) * BITS_PER_BYTE)
#define BITSET_SIZE        (SINGLE_BYTE_SIZE / BITS_IN_ROOM)

#ifdef UNALIGNED_WORD_ACCESS
typedef unsigned int   Bits;
#else
typedef unsigned char  Bits;
#endif
typedef Bits           BitSet[BITSET_SIZE];
typedef Bits*          BitSetRef;

#define NULL_UCHARP       ((UChar* )0)

#define BITSET_CLEAR(bs) do {\
  int i;\
  for (i = 0; i < BITSET_SIZE; i++) { (bs)[i] = 0; }\
} while (0)

#define BS_ROOM(bs,pos)            (bs)[pos / BITS_IN_ROOM]
#define BS_BIT(pos)                (1 << (pos % BITS_IN_ROOM))

#define BITSET_AT(bs, pos)         (BS_ROOM(bs,pos) & BS_BIT(pos))
#define BITSET_SET_BIT(bs, pos)     BS_ROOM(bs,pos) |= BS_BIT(pos)
#define BITSET_CLEAR_BIT(bs, pos)   BS_ROOM(bs,pos) &= ~(BS_BIT(pos))

#define SIZE_BITSET        sizeof(BitSet)

static int
bitset_is_empty(BitSetRef bs)
{
  int i;
  for (i = 0; i < BITSET_SIZE; i++) {
    if (bs[i] != 0) return 0;
  }
  return 1;
}

static int
bitset_on_num(BitSetRef bs)
{
  int i, n;

  n = 0;
  for (i = 0; i < SINGLE_BYTE_SIZE; i++) {
    if (BITSET_AT(bs, i)) n++;
  }
  return n;
}


/* bytes buffer */
typedef struct _BBuf {
  UChar* p;
  unsigned int used;
  unsigned int alloc;
} BBuf;

static int
bbuf_init(BBuf* buf, int size)
{
  buf->p = (UChar* )xmalloc(size);
  if (IS_NULL(buf->p)) return(REGERR_MEMORY);

  buf->alloc = size;
  buf->used  = 0;
  return 0;
}

#define BBUF_INIT(buf,size)    bbuf_init((BBuf* )(buf), (size))

#define BBUF_SIZE_INC(buf,inc) do{\
  (buf)->alloc += (inc);\
  (buf)->p = (UChar* )xrealloc((buf)->p, (buf)->alloc);\
  if (IS_NULL((buf)->p)) return(REGERR_MEMORY);\
} while (0)

#define BBUF_EXPAND(buf,low) do{\
  do { (buf)->alloc *= 2; } while ((buf)->alloc < low);\
  (buf)->p = (UChar* )xrealloc((buf)->p, (buf)->alloc);\
  if (IS_NULL((buf)->p)) return(REGERR_MEMORY);\
} while (0)

#define BBUF_ENSURE_SIZE(buf,size) do{\
  int new_alloc = (buf)->alloc;\
  while (new_alloc < (size)) { new_alloc *= 2; }\
  if ((buf)->alloc != new_alloc) {\
    (buf)->p = (UChar* )xrealloc((buf)->p, new_alloc);\
    if (IS_NULL((buf)->p)) return(REGERR_MEMORY);\
    (buf)->alloc = new_alloc;\
  }\
} while (0)

#define BBUF_WRITE(buf,pos,bytes,n) do{\
  int used = (pos) + (n);\
  if ((buf)->alloc < used) BBUF_EXPAND((buf),used);\
  xmemcpy((buf)->p + (pos), (bytes), (n));\
  if ((buf)->used < used) (buf)->used = used;\
} while (0)

#define BBUF_WRITE1(buf,pos,byte) do{\
  int used = (pos) + 1;\
  if ((buf)->alloc < used) BBUF_EXPAND((buf),used);\
  (buf)->p[(pos)] = (byte);\
  if ((buf)->used < used) (buf)->used = used;\
} while (0)

#define BBUF_ADD(buf,bytes,n)       BBUF_WRITE((buf),(buf)->used,(bytes),(n))
#define BBUF_ADD1(buf,byte)         BBUF_WRITE1((buf),(buf)->used,(byte))
#define BBUF_GET_ADD_POSITION(buf)  ((buf)->p + (buf)->used)

/* from < to */
#define BBUF_MOVE_RIGHT(buf,from,to,n) do {\
  if ((to) + (n) > (buf)->alloc) BBUF_EXPAND((buf),(to) + (n));\
  xmemmove((buf)->p + (to), (buf)->p + (from), (n));\
  if ((to) + (n) > (buf)->used) (buf)->used = (to) + (n);\
} while (0)

/* from > to */
#define BBUF_MOVE_LEFT(buf,from,to,n) do {\
  xmemmove((buf)->p + (to), (buf)->p + (from), (n));\
} while (0)

/* from > to */
#define BBUF_MOVE_LEFT_REDUCE(buf,from,to) do {\
  xmemmove((buf)->p + (to), (buf)->p + (from), (buf)->used - (from));\
  (buf)->used -= (from - to);\
} while (0)

#define BBUF_INSERT(buf,pos,bytes,n) do {\
  if (pos >= (buf)->used) {\
    BBUF_WRITE(buf,pos,bytes,n);\
  }\
  else {\
    BBUF_MOVE_RIGHT((buf),(pos),(pos) + (n),((buf)->used - (pos)));\
    xmemcpy((buf)->p + (pos), (bytes), (n));\
  }\
} while (0)

#define BBUF_GET_BYTE(buf, pos) (buf)->p[(pos)]


/* node type */
#define N_EMPTY        (1<<  0)
#define N_LIST         (1<<  1)
#define N_ALT          (1<<  2)
#define N_STRING       (1<<  3)
#define N_STRING_RAW   (1<<  4)  /* immediate bytes (backslashed-number) */
#define N_CCLASS       (1<<  5)
#define N_CTYPE        (1<<  6)
#define N_ANYCHAR      (1<<  7)
#define N_ANCHOR       (1<<  8)
#define N_BACKREF      (1<<  9)
#define N_QUALIFIER    (1<< 10)
#define N_OPTION       (1<< 11)
#define N_EFFECT       (1<< 12)

#define NTYPE(node)        ((node)->type)
#define NCONS(node)        ((node)->u.cons)
#define NSTRING(node)      ((node)->u.str)
#define NCCLASS(node)      ((node)->u.cclass)
#define NCTYPE(node)       ((node)->u.ctype)
#define NQUALIFIER(node)   ((node)->u.qualifier)
#define NANCHOR(node)      ((node)->u.anchor)
#define NBACKREF(node)     ((node)->u.effect)
#define NOPTION(node)      ((node)->u.opt)
#define NEFFECT(node)      ((node)->u.effect)

#define CTYPE_WORD              (1<<0)
#define CTYPE_NOT_WORD          (1<<1)
#define CTYPE_WORD_BOUND        (1<<2)
#define CTYPE_NOT_WORD_BOUND    (1<<3)
#define CTYPE_WORD_BEGIN        (1<<4)
#define CTYPE_WORD_END          (1<<5)
#ifdef USE_ONIGURUMA_EXTENSION
#define CTYPE_WORD_SB           (1<<6)
#define CTYPE_WORD_MB           (1<<7)
#endif

#define ANCHOR_BEGIN_BUF        (1<<0)
#define ANCHOR_BEGIN_LINE       (1<<1)
#define ANCHOR_BEGIN_POSITION   (1<<2)
#define ANCHOR_END_BUF          (1<<3)
#define ANCHOR_SEMI_END_BUF     (1<<4)
#define ANCHOR_END_LINE         (1<<5)
#define ANCHOR_ANYCHAR_STAR     (1<<6)   /* ".*" optimize info */
#define ANCHOR_ANYCHAR_STAR_PL  (1<<7)   /* ".*" optimize info (posix-line) */

#define ANCHOR_ANYCHAR_STAR_MASK (ANCHOR_ANYCHAR_STAR | ANCHOR_ANYCHAR_STAR_PL)
#define ANCHOR_END_BUF_MASK      (ANCHOR_END_BUF | ANCHOR_SEMI_END_BUF)

#define EFFECT_MEMORY           (1<<0)
#define EFFECT_PREC_READ        (1<<1)
#define EFFECT_PREC_READ_NOT    (1<<2)
#define EFFECT_STOP_BACKTRACK   (1<<3)
#define EFFECT_LOOK_BEHIND      (1<<4)
#define EFFECT_LOOK_BEHIND_NOT  (1<<5)

#define REPEAT_INFINITE         -1
#define IS_REPEAT_INFINITE(n)   ((n) == REPEAT_INFINITE)

#define NODE_STR_MARGIN         16
#define NODE_STR_BUF_SIZE       28

#define N_STRING_MASK       (N_STRING | N_STRING_RAW)
#define NSTRING_LEN(node)   ((node)->u.str.end - (node)->u.str.s)

typedef struct {
  UChar* s;
  UChar* end;
  int    capa;    /* (allocated size - 1) or 0: use buf[] */
  UChar  buf[NODE_STR_BUF_SIZE];
} StrNode;

typedef struct {
  int    not;
  BitSet bs;
  BBuf*  mbuf;     /* multi-byte info or NULL */
} CClassNode;

typedef struct {
  struct _Node* target;
  int lower;
  int upper;
  int greedy;
  int by_number;      /* {n,m} */
  int target_empty;   /* target can match with empty data */
  struct _Node* head_exact;
  struct _Node* next_head_exact;
} QualifierNode;

typedef struct {
  RegOptionType option;
  struct _Node* target;
} OptionNode;

typedef struct {
  int type;
  int regnum;
  struct _Node* target;
} EffectNode;

typedef struct _Node {
  int type;
  union {
    StrNode       str;
    CClassNode    cclass;
    QualifierNode qualifier;
    OptionNode    opt;
    EffectNode    effect;
    struct {
      struct _Node* left;
      struct _Node* right;
    } cons;
    struct {
      int type;
    } anchor;
    struct {
      int type;
    } ctype;
  } u;
} Node;


/* scan pattern methods */
#define PEND_VALUE  -1

#define PFETCH(c)   do { (c) = *p++; } while (0)
#define PUNFETCH    p--
#define PINC        p++
#define PPEEK       (p < end ? *p : PEND_VALUE)
#define PEND        (p < end ?  0 : 1)

#define TTRANS(table,c)        ((table)[c])

/* methods for support multi-byte code, */
#define ismb(code,c)             (mblen((code),(c)) != 1)
#define MB2WC(p,end,code)        mb2wc((p),(end),(code))
#define MBBACK(code,start,s,n)   step_backward_char((code),(start),(s),(n))

#ifdef REG_RUBY_M17N

#define MB2WC_AVAILABLE(enc)          1
#define mbmaxlen(enc)                 m17n_mbmaxlen(enc)
#define mblen(enc,c)                  m17n_mbclen(enc,c)
#define mbmaxlen_dist(enc) \
      (mbmaxlen(enc) > 0 ? mbmaxlen(enc) : INFINITE_DISTANCE)

#define IS_SINGLEBYTE_CODE(enc)    (m17n_mbmaxlen(enc) == 1)
/* #define IS_INDEPENDENT_TRAIL(enc)  m17n_independent_trail(enc) */
#define IS_INDEPENDENT_TRAIL(enc)  IS_SINGLEBYTE_CODE(enc)

#define IS_CODE_ASCII(enc,c)     IS_ASCII(c)
#define IS_CODE_GRAPH(enc,c)     IS_GRAPH(c)
#define IS_CODE_PRINT(enc,c)     m17n_isprint(enc,c)
#define IS_CODE_ALNUM(enc,c)     m17n_isalnum(enc,c)
#define IS_CODE_ALPHA(enc,c)     m17n_isalpha(enc,c)
#define IS_CODE_LOWER(enc,c)     m17n_islower(enc,c)
#define IS_CODE_UPPER(enc,c)     m17n_isupper(enc,c)
#define IS_CODE_CNTRL(enc,c)     m17n_iscntrl(enc,c)
#define IS_CODE_PUNCT(enc,c)     m17n_ispunct(enc,c)
#define IS_CODE_SPACE(enc,c)     m17n_isspace(enc,c)
#define IS_CODE_BLANK(enc,c)     IS_BLANK(c)
#define IS_CODE_DIGIT(enc,c)     m17n_isdigit(enc,c)
#define IS_CODE_XDIGIT(enc,c)    m17n_isxdigit(enc,c)

#define IS_CODE_WORD(enc,c)      m17n_iswchar(enc,c)
#define ISNOT_CODE_WORD(enc,c)  (!m17n_iswchar(enc,c))

#define IS_WORD_STR(code,s,end) \
  (ismb((code),*(s)) ? (s + mblen((code),*(s)) <= (end)) : \
                       m17n_iswchar(code,*(s)))
#define IS_WORD_STR_INC(code,s,end) \
  (ismb((code),*(s)) ? ((s) += mblen((code),*(s)), (s) <= (end)) : \
                       (s++, m17n_iswchar(code,s[-1])))

#define IS_WORD_HEAD(enc,c) (ismb(enc,c) ? 1 : IS_CODE_WORD(enc,c))

#define IS_SB_WORD(code,c)  (mblen(code,c) == 1 && IS_CODE_WORD(code,c))
#define IS_MB_WORD(code,c)  ismb(code,c)

#define mb2wc(p,e,enc)        m17n_codepoint((enc),(p),(e))

#else  /* REG_RUBY_M17N */

const char REG_MBLEN_TABLE[][REG_CHAR_TABLE_SIZE] = {
  { /* ascii */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
  },
  { /* utf8 */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 1, 1
  },
  { /* euc-jp */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 3,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1
  },
  { /* sjis */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2
  }
};

static const char SJIS_FOLLOW_TABLE[SINGLE_BYTE_SIZE] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0
};

static int
mb_max_length(RegCharEncoding code)
{
  /* can't use switch statement, code isn't int type. */
       if (code == REGCODE_ASCII)  return 1;
  else if (code == REGCODE_EUCJP)  return 3;
  else if (code == REGCODE_SJIS)   return 2;
  else return 6; /* REGCODE_UTF8 */
}

#define MB2WC_AVAILABLE(code)       1

#define mbmaxlen_dist(code)         mbmaxlen(code)
#define mbmaxlen(code)              mb_max_length(code)
#define mblen(code,c)               (code)[(c)]

#define IS_SINGLEBYTE_CODE(code)    ((code) == REGCODE_ASCII)
#define IS_INDEPENDENT_TRAIL(code)  ((code) <= REGCODE_UTF8)

#define IS_CODE_ASCII(code,c)        IS_ASCII(c)
#define IS_CODE_GRAPH(code,c)        IS_GRAPH(c)
#define IS_CODE_PRINT(code,c)        IS_PRINT(c)
#define IS_CODE_ALNUM(code,c)        IS_ALNUM(c)
#define IS_CODE_ALPHA(code,c)        IS_ALPHA(c)
#define IS_CODE_LOWER(code,c)        IS_LOWER(c)
#define IS_CODE_UPPER(code,c)        IS_UPPER(c)
#define IS_CODE_CNTRL(code,c)        IS_CNTRL(c)
#define IS_CODE_PUNCT(code,c)        IS_PUNCT(c)
#define IS_CODE_SPACE(code,c)        IS_SPACE(c)
#define IS_CODE_BLANK(code,c)        IS_BLANK(c)
#define IS_CODE_DIGIT(code,c)        IS_DIGIT(c)
#define IS_CODE_XDIGIT(code,c)       IS_XDIGIT(c)

#define IS_SB_WORD(code,c)  (IS_CODE_ALNUM(code,c) || (c) == '_')
#define IS_MB_WORD(code,c)  ismb(code,c)

#define IS_CODE_WORD(code,c) \
  (IS_SB_WORD(code,c) && ((c) < 0x80 || (code) == REGCODE_ASCII))
#define ISNOT_CODE_WORD(code,c) \
  ((!IS_SB_WORD(code,c)) && !ismb(code,c))

#define IS_WORD_STR(code,s,end) \
  (ismb((code),*(s)) ? (s + mblen((code),*(s)) <= (end)) : \
                       IS_SB_WORD(code,*(s)))
#define IS_WORD_STR_INC(code,s,end) \
  (ismb((code),*(s)) ? ((s) += mblen((code),*(s)), (s) <= (end)) : \
                       (s++, IS_SB_WORD(code,s[-1])))

#define IS_WORD_HEAD(code,c) (ismb(code,c) ? 1 : IS_SB_WORD(code,c))

static WCInt
mb2wc(UChar* p, UChar* end, RegCharEncoding code)
{
  int c, i, len;
  WCInt n;

  if (code == REGCODE_UTF8) {
    PFETCH(c);
    len = mblen(code,c) - 1;
    n = c & ((1 << (6 - len)) - 1);
    while (len--) {
      PFETCH(c);
      n = (n << 6) | (c & ((1 << 6) - 1));
    }
  }
  else {
    PFETCH(c);
    len = mblen(code,c);
    n = c;
    if (len == 1) return n;

    for (i = 1; i < len; i++) {
      if (PEND) break;
      PFETCH(c);
      n <<= 8;  n += c;
    }

    len = mbmaxlen(code) - i;
    while (len-- > 0) {
      n <<= 8; 
    }
  }
  return n;
}

#define eucjp_islead(c)    ((UChar )((c) - 0xa1) > 0xfe - 0xa1)
#define utf8_islead(c)     ((UChar )((c) & 0xc0) != 0x80)
#define sjis_ismbfirst(c)  ismb(REGCODE_SJIS, (c))
#define sjis_ismbtrail(c)  SJIS_FOLLOW_TABLE[(c)]

#endif /* not REG_RUBY_M17N */


static UChar*
get_left_adjust_char_head(RegCharEncoding code, UChar* start, UChar* s)
{
  UChar *p;
  int len;

  if (s <= start) return s;
  p = s;

#ifdef REG_RUBY_M17N
  while (!m17n_islead(code, *p) && p > start) p--;
  while (p + (len = mblen(code, *p)) < s) {
    p += len;
  }
  if (p + len == s) return s;
  return p;
#else

  if (code == REGCODE_ASCII) {
    return p;
  }
  else if (code ==  REGCODE_EUCJP) {
    while (!eucjp_islead(*p) && p > start) p--;
    len = mblen(code, *p);
    if (p + len > s) return p;
    p += len;
    return p + ((s - p) & ~1);
  }
  else if (code == REGCODE_SJIS) {
    if (sjis_ismbtrail(*p)) {
      while (p > start) {
	if (! sjis_ismbfirst(*--p)) {
	  p++;
	  break;
	}
      } 
    }
    len = mblen(code, *p);
    if (p + len > s) return p;
    p += len;
    return p + ((s - p) & ~1);
  }
  else { /* REGCODE_UTF8 */
    while (!utf8_islead(*p) && p > start) p--;
    return p;
  }
#endif  /* REG_RUBY_M17N */
}

static UChar*
get_right_adjust_char_head(RegCharEncoding code, UChar* start, UChar* s)
{
  UChar* p = get_left_adjust_char_head(code, start, s);

  if (p < s) {
    p += mblen(code, *p);
  }
  return p;
}

static UChar*
get_right_adjust_char_head_with_prev(RegCharEncoding code,
				     UChar* start, UChar* s, UChar** prev)
{
  UChar* p = get_left_adjust_char_head(code, start, s);

  if (p < s) {
    if (prev) *prev = p;
    p += mblen(code, *p);
  }
  else {
    if (prev) *prev = (UChar* )NULL; /* Sorry */
  }
  return p;
}

static UChar*
get_prev_char_head(RegCharEncoding code, UChar* start, UChar* s)
{
  if (s <= start)
    return (UChar* )NULL;

  return get_left_adjust_char_head(code, start, s - 1);
}

static UChar*
step_backward_char(RegCharEncoding code, UChar* start, UChar* s, int n)
{
  while (IS_NOT_NULL(s) && n-- > 0) {
    if (s <= start)
      return (UChar* )NULL;

    s = get_left_adjust_char_head(code, start, s - 1);
  }
  return s;
}

/* used as function pointer value */
static int
is_code_ascii(RegCharEncoding code, UChar c) { return IS_CODE_ASCII(code, c); }
static int
is_code_graph(RegCharEncoding code, UChar c) { return IS_CODE_GRAPH(code, c); }
static int
is_code_print(RegCharEncoding code, UChar c) { return IS_CODE_PRINT(code, c); }
static int
is_code_alnum(RegCharEncoding code, UChar c) { return IS_CODE_ALNUM(code, c); }
static int
is_code_alpha(RegCharEncoding code, UChar c) { return IS_CODE_ALPHA(code, c); }
static int
is_code_lower(RegCharEncoding code, UChar c) { return IS_CODE_LOWER(code, c); }
static int
is_code_upper(RegCharEncoding code, UChar c) { return IS_CODE_UPPER(code, c); }
static int
is_code_cntrl(RegCharEncoding code, UChar c) { return IS_CODE_CNTRL(code, c); }
static int
is_code_punct(RegCharEncoding code, UChar c) { return IS_CODE_PUNCT(code, c); }
static int
is_code_space(RegCharEncoding code, UChar c) { return IS_CODE_SPACE(code, c); }
static int
is_code_blank(RegCharEncoding code, UChar c) { return IS_CODE_BLANK(code, c); }
static int
is_code_digit(RegCharEncoding code, UChar c) { return IS_CODE_DIGIT(code, c); }
static int
is_code_xdigit(RegCharEncoding code, UChar c) { return IS_CODE_XDIGIT(code, c); }

static int
k_strncmp(UChar* s1, UChar* s2, int n)
{
  int x;

  while (n-- > 0) {
    x = *s2++ - *s1++;
    if (x) return x;
  }
  return 0;
}

static void
k_strcpy(UChar* dest, UChar* src, UChar* end)
{
  int len = end - src;
  if (len > 0) {
    xmemcpy(dest, src, len);
    dest[len] = (UChar )0;
  }
}

static UChar*
k_strdup(UChar* s, UChar* end)
{
  int len = end - s;

  if (len > 0) {
    UChar* r = (UChar* )xmalloc(len + 1);
    CHECK_NULL_RETURN(r);
    xmemcpy(r, s, len);
    r[len] = (UChar )0;
    return r;
  }
  else return NULL;
}

static UChar*
k_strcat_capa(UChar* dest, UChar* dest_end, UChar* src, UChar* src_end, int capa)
{
  UChar* r;

  if (dest)
    r = (UChar* )xrealloc(dest, capa + 1);
  else
    r = (UChar* )xmalloc(capa + 1);

  CHECK_NULL_RETURN(r);
  k_strcpy(r + (dest_end - dest), src, src_end);
  return r;
}

/* dest on static area */
static UChar*
strcat_capa_from_static(UChar* dest, UChar* dest_end,
			UChar* src, UChar* src_end, int capa)
{
  UChar* r;

  r = (UChar* )xmalloc(capa + 1);
  CHECK_NULL_RETURN(r);
  k_strcpy(r, dest, dest_end);
  k_strcpy(r + (dest_end - dest), src, src_end);
  return r;
}

#ifdef USE_RECYCLE_NODE
typedef struct _FreeNode {
  struct _FreeNode* next;
} FreeNode;

static FreeNode* FreeNodeList = (FreeNode* )NULL;
#endif

static void
node_free(Node* node)
{
  if (IS_NULL(node)) return ;

  switch (NTYPE(node)) {
  case N_STRING:
  case N_STRING_RAW:
    if (IS_NOT_NULL(NSTRING(node).s) && NSTRING(node).s != NSTRING(node).buf) {
      xfree(NSTRING(node).s);
    }
    break;

  case N_LIST:
  case N_ALT:
    node_free(NCONS(node).left);
    node_free(NCONS(node).right);
    break;

  case N_CCLASS:
    if (NCCLASS(node).mbuf)
      xfree(NCCLASS(node).mbuf);
    break;

  case N_QUALIFIER:
    if (NQUALIFIER(node).target)
      node_free(NQUALIFIER(node).target);
    break;

  case N_OPTION:
    if (NOPTION(node).target)
      node_free(NOPTION(node).target);
    break;

  case N_EFFECT:
    if (NEFFECT(node).target)
      node_free(NEFFECT(node).target);
    break;
  }

#ifdef USE_RECYCLE_NODE
  {
    FreeNode* n;

    n = (FreeNode* )node;
    n->next = FreeNodeList;
    FreeNodeList = n;
  }
#else
  xfree(node);
#endif
}

static Node*
node_new()
{
  Node* node;

#ifdef USE_RECYCLE_NODE
  if (IS_NOT_NULL(FreeNodeList)) {
    node = (Node* )FreeNodeList;
    FreeNodeList = FreeNodeList->next;
    return node;
  }
#endif

  node = (Node* )xmalloc(sizeof(Node));
  return node;
}

static Node*
node_new_empty()
{
  Node* node = node_new();
  CHECK_NULL_RETURN(node);
  node->type = N_EMPTY;
  return node;
}

static Node*
node_new_cclass()
{
  Node* node = node_new();
  CHECK_NULL_RETURN(node);
  node->type = N_CCLASS;
  BITSET_CLEAR(NCCLASS(node).bs);
  NCCLASS(node).not  = 0;
  NCCLASS(node).mbuf = NULL;
  return node;
}

static Node*
node_new_ctype(int type)
{
  Node* node = node_new();
  CHECK_NULL_RETURN(node);
  node->type = N_CTYPE;
  NCTYPE(node).type = type;
  return node;
}

static Node*
node_new_anychar()
{
  Node* node = node_new();
  CHECK_NULL_RETURN(node);
  node->type = N_ANYCHAR;
  return node;
}

static Node*
node_new_list(Node* left, Node* right)
{
  Node* node = node_new();
  CHECK_NULL_RETURN(node);
  node->type = N_LIST;
  NCONS(node).left  = left;
  NCONS(node).right = right;
  return node;
}

static Node*
node_new_alt(Node* left, Node* right)
{
  Node* node = node_new();
  CHECK_NULL_RETURN(node);
  node->type = N_ALT;
  NCONS(node).left  = left;
  NCONS(node).right = right;
  return node;
}

static Node*
node_new_anchor(int type)
{
  Node* node = node_new();
  CHECK_NULL_RETURN(node);
  node->type = N_ANCHOR;
  NANCHOR(node).type = type;
  return node;
}

static Node*
node_new_backref(int regnum)
{
  Node* node = node_new();
  CHECK_NULL_RETURN(node);
  node->type = N_BACKREF;
  NBACKREF(node).regnum = regnum;
  return node;
}

static Node*
node_new_qualifier(int lower, int upper, int by_number)
{
  Node* node = node_new();
  CHECK_NULL_RETURN(node);
  node->type = N_QUALIFIER;
  NQUALIFIER(node).target = NULL;
  NQUALIFIER(node).lower  = lower;
  NQUALIFIER(node).upper  = upper;
  NQUALIFIER(node).greedy = 1;
  NQUALIFIER(node).by_number       = by_number;
  NQUALIFIER(node).target_empty    = 0;
  NQUALIFIER(node).head_exact      = (Node* )NULL;
  NQUALIFIER(node).next_head_exact = (Node* )NULL;
  return node;
}

static Node*
node_new_option(RegOptionType option)
{
  Node* node = node_new();
  CHECK_NULL_RETURN(node);
  node->type = N_OPTION;
  NOPTION(node).option = option;
  NOPTION(node).target = NULL;
  return node;
}

static Node*
node_new_effect(int type)
{
  Node* node = node_new();
  CHECK_NULL_RETURN(node);
  node->type = N_EFFECT;
  NEFFECT(node).type   = type;
  NEFFECT(node).regnum = 0;
  NOPTION(node).target = NULL;
  return node;
}

static int
node_str_cat(Node* node, UChar* s, UChar* end)
{
  int addlen = end - s;

  if (addlen > 0) {
    int len  = NSTRING(node).end - NSTRING(node).s;

    if (NSTRING(node).capa > 0 || (len + addlen > NODE_STR_BUF_SIZE - 1)) {
      UChar* p;
      int capa = len + addlen + NODE_STR_MARGIN;

      if (capa <= NSTRING(node).capa) {
	k_strcpy(NSTRING(node).s + len, s, end);
      }
      else {
	if (NSTRING(node).s == NSTRING(node).buf)
	  p = strcat_capa_from_static(NSTRING(node).s, NSTRING(node).end,
				      s, end, capa);
	else
	  p = k_strcat_capa(NSTRING(node).s, NSTRING(node).end, s, end, capa);

	CHECK_NULL_RETURN_VAL(p, 1);
	NSTRING(node).s    = p;
	NSTRING(node).capa = capa;
      }
    }
    else {
      k_strcpy(NSTRING(node).s + len, s, end);
    }
    NSTRING(node).end = NSTRING(node).s + len + addlen;
  }

  return 0;
}

static int
node_str_cat_char(Node* node, UChar c)
{
  UChar s[1];

  s[0] = c;
  return node_str_cat(node, s, s + 1);
}

static void
node_conv_to_str_node(Node* node, int type)
{
  node->type = type;  /* type: N_STRING / N_STRING_RAW */
  NSTRING(node).capa = 0;
  NSTRING(node).s    = NSTRING(node).buf;
  NSTRING(node).end  = NSTRING(node).buf;
}

static Node*
node_new_str(UChar* s, UChar* end)
{
  Node* node = node_new();
  CHECK_NULL_RETURN(node);

  node->type = N_STRING;
  NSTRING(node).capa = 0;
  NSTRING(node).s    = NSTRING(node).buf;
  NSTRING(node).end  = NSTRING(node).buf;
  if (node_str_cat(node, s, end)) {
    node_free(node);
    return NULL;
  }
  return node;
}

static Node*
node_new_str_raw(UChar* s, UChar* end)
{
  Node* node = node_new_str(s, end);
  node->type = N_STRING_RAW;
  return node;
}

static Node*
node_new_str_char(UChar c)
{
  UChar p[1];

  p[0] = c;
  return node_new_str(p, p + 1);
}

static Node*
node_new_str_raw_char(UChar c)
{
  UChar p[1];

  p[0] = c;
  return node_new_str_raw(p, p + 1);
}

static Node*
str_node_split_last_char(StrNode* sn, int type, RegCharEncoding code)
{
  UChar *p;
  Node* n = (Node* )NULL;

  if (sn->end > sn->s) {
    p = get_prev_char_head(code, sn->s, sn->end);
    if (p && p > sn->s) { /* can be splitted. */
      if (type == N_STRING_RAW)
	n = node_new_str_raw(p, sn->end);
      else
	n = node_new_str(p, sn->end);

      sn->end = p;
    }
  }
  return n;
}

typedef struct {
  RegOptionType   option;
  RegCharEncoding code;
  int   regnum;
  Node* last_node;
  int   paren_nest;
} ScanEnv;

#define NOT_TERM         ((UChar )0)

static int
scan_unsigned_number(UChar** src, UChar* end)
{
  unsigned int num, val;
  int c;
  UChar* p = *src;

  num = 0;
  while (!PEND) {
    PFETCH(c);
    if (IS_DIGIT(c)) {
      val = (unsigned int )DIGITVAL(c);
      if ((INT_MAX_LIMIT - val) / 10UL < num)
	return -1;  /* overflow */

      num = num * 10 + val;
    }
    else {
      PUNFETCH;
      break;
    }
  }
  *src = p;
  return num;
}

static int
scan_unsigned_hexadecimal_number(UChar** src, UChar* end, int maxlen)
{
  int c;
  unsigned int num, val;
  UChar* p = *src;

  num = 0;
  while (!PEND && maxlen-- != 0) {
    PFETCH(c);
    if (IS_XDIGIT(c)) {
      val = (unsigned int )XDIGITVAL(c);
      if ((INT_MAX_LIMIT - val) / 16UL < num)
	return -1;  /* overflow */

      num = (num << 4) + XDIGITVAL(c);
    }
    else {
      PUNFETCH;
      break;
    }
  }
  *src = p;
  return num;
}

static int
scan_unsigned_octal_number(UChar** src, UChar* end, int maxlen)
{
  int c;
  unsigned int num, val;
  UChar* p = *src;

  num = 0;
  while (!PEND && maxlen-- != 0) {
    PFETCH(c);
    if (IS_ODIGIT(c)) {
      val = ODIGITVAL(c);
      if ((INT_MAX_LIMIT - val) / 8UL < num)
	return -1;  /* overflow */

      num = (num << 3) + val;
    }
    else {
      PUNFETCH;
      break;
    }
  }
  *src = p;
  return num;
}

/* data format:
   [n][from-1][to-1][from-2][to-2] ... [from-n][to-n]
   (all data size is WCInt)
 */
#define GET_WCINT(wc,p)   (wc) = *((WCInt* )(p))
#define BBUF_WRITE_WCINT(bbuf,pos,wc) \
    BBUF_WRITE(bbuf, pos, &(wc), SIZE_WCINT)

static int
is_in_wc_range(UChar* p, WCInt wc)
{
  WCInt n, *data;
  int low, high, x;

  GET_WCINT(n, p);
  data = (WCInt* )p;
  data++;

  for (low = 0, high = n; low < high; ) {
    x = (low + high) >> 1;
    if (wc > data[x * 2 + 1])
      low = x + 1;
    else
      high = x;
  }

  return ((low < n && wc >= data[low * 2]) ? 1 : 0);
}

/* data format:
   [multi-byte-head-BitSet][n][from-1][to-1][from-2][to-2] ... [from-n][to-n]
   (all data size is WCInt)
 */
static int
add_wc_range(BBuf** pbuf, WCInt from, WCInt to, UChar cfrom, UChar cto)
{
#define INIT_MULTI_BYTE_RANGE_SIZE  (SIZE_WCINT * 5)

  int r, inc_n, pos;
  int low, high, bound, x;
  WCInt n, *data;
  BBuf* bbuf;

  if (from > to) {
    n = from; from = to; to = n;
  }

  if (IS_NULL(*pbuf)) {
    bbuf = *pbuf = (BBuf* )xmalloc(sizeof(BBuf));
    CHECK_NULL_RETURN_VAL(*pbuf, REGERR_MEMORY);
    r = bbuf_init(*pbuf, SIZE_BITSET + INIT_MULTI_BYTE_RANGE_SIZE);
    if (r) return r;

    n = 0;
    BBUF_WRITE_WCINT(bbuf, SIZE_BITSET, n);
    BITSET_CLEAR((BitSetRef )bbuf->p);
  }
  else {
    bbuf = *pbuf;
    GET_WCINT(n, bbuf->p + SIZE_BITSET);
    if (n >= 255)
      return REGERR_TOO_MANY_MULTI_BYTE_RANGES;

    data = (WCInt* )(bbuf->p + SIZE_BITSET);
    data++;
  }

  for (low = 0, bound = n; low < bound; ) {
    x = (low + bound) >> 1;
    if (from > data[x*2 + 1] - 1)
      low = x + 1;
    else
      bound = x;
  }

  for (high = low, bound = n; high < bound; ) {
    x = (high + bound) >> 1;
    if (to >= data[x*2] - 1)
      to = x + 1;
    else
      bound = x;
  }

  inc_n = low + 1 - high;

  if (inc_n != 1) {
    if (from > data[low*2])
      from = data[low*2];
    if (to < data[(high - 1)*2 + 1])
      to = data[(high - 1)*2 + 1];
  }

  if (inc_n != 0 && high < n) {
    int from_pos = SIZE_BITSET + SIZE_WCINT * (1 + high * 2);
    int to_pos   = SIZE_BITSET + SIZE_WCINT * (1 + (low + 1) * 2);
    int size = (n - high) * 2 * SIZE_WCINT;

    if (inc_n > 0) {
      BBUF_MOVE_RIGHT(bbuf, from_pos, to_pos, size);
    }
    else {
      BBUF_MOVE_LEFT_REDUCE(bbuf, from_pos, to_pos);
    }
  }

  pos = SIZE_BITSET + SIZE_WCINT * (1 + low * 2);
  BBUF_ENSURE_SIZE(bbuf, pos + SIZE_WCINT * 2);
  BBUF_WRITE_WCINT(bbuf, pos, from);
  BBUF_WRITE_WCINT(bbuf, pos + SIZE_WCINT, to);
  n += inc_n;
  BBUF_WRITE_WCINT(bbuf, SIZE_BITSET, n);

  if (inc_n > 0) {
    int i;
    UChar tmp;

    if (cfrom > cto) {
      tmp = cfrom; cfrom = cto; cto = tmp;
    }

    for (i = cfrom; i <= cto; i++) {
      BITSET_SET_BIT((BitSetRef)bbuf->p, i);      
    }
  }
  return 0;
}

static int
add_multi_byte_range(BBuf** pbuf, RegCharEncoding code,
		     UChar* from, UChar* from_end, UChar* to, UChar* to_end)
{
  int mb_maxlen;
  WCInt wc_from, wc_to;

  if (! MB2WC_AVAILABLE(code))
    return REGERR_SPECIFIED_ENCODING_CANT_CONVERT_TO_WIDE_CHAR;

  wc_from = MB2WC(from, from_end, code);
  wc_to   = MB2WC(to,   to_end,   code);

  if (wc_from > wc_to)
    return REGERR_RIGHT_SMALLER_THAN_LEFT_IN_CLASS_RANGE;

  return add_wc_range(pbuf, wc_from, wc_to, from[0], to[0]);
}

static int
scan_backslash_multi_byte_sequence(UChar** src, UChar* end,
				   RegCharEncoding code, UChar* buf, int len)
{
  int c, i, r = 0;
  UChar* p = *src;

  for (i = 0; i < len; i++) {
    if (PEND)
      return REGERR_TOO_SHORT_MULTI_BYTE_STRING;

    PFETCH(c);
    switch (c) {
    case '\\':
      if (PEND) return REGERR_END_PATTERN_AT_BACKSLASH;
      PFETCH(c);
      switch (c) {
      case 'x':
	r = scan_unsigned_hexadecimal_number(&p, end, 2);
	if (r < 0) return REGERR_TOO_BIG_NUMBER;
	if (p == *src) {  /* can't read nothing. */
	  goto end;
	}
	buf[i] = r;
	break;
	
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7':
	PUNFETCH;
	r = scan_unsigned_octal_number(&p, end, 3);
	if (r < 0) return REGERR_TOO_BIG_NUMBER;
	if (p == *src) {  /* can't read nothing. */
	  goto end;
	}
	buf[i] = r;
	break;

      default:
	buf[i] = c;
      }
      break;

    default:
      PUNFETCH;
      goto end;
      break;
    }
  }

 end:
  *src = p;
  return i;
}

static int
scan_range_qualifier(UChar** src, UChar* end, int* lower, int* upper)
{
  int low, up;
  int c;
  UChar* p = *src;

#if 1
  if (PEND) return 1;  /* "....{" : OK! */
#else
  if (PEND) return REGERR_END_PATTERN_AT_LEFT_BRACE;  /* "....{" syntax error */
  c = PPEEK;
  if (c == ')' || c == '(' || c == '|') {
    return REGERR_END_PATTERN_AT_LEFT_BRACE;
  }
#endif

  low = scan_unsigned_number(&p, end);
  if (low < 0) return REGERR_TOO_BIG_NUMBER_FOR_REPEAT_RANGE;
  if (low > REG_MAX_REPEAT_NUM)
    return REGERR_TOO_BIG_NUMBER_FOR_REPEAT_RANGE;

  if (p == *src) return 1;  /* can't read low, 1: not error */

  if (PEND) return 1;
  PFETCH(c);
  if (c == ',') {
    UChar* prev = p;
    up = scan_unsigned_number(&p, end);
    if (up < 0) return REGERR_TOO_BIG_NUMBER_FOR_REPEAT_RANGE;
    if (up > REG_MAX_REPEAT_NUM)
      return REGERR_TOO_BIG_NUMBER_FOR_REPEAT_RANGE;

    if (p == prev) up = REPEAT_INFINITE;  /* {n,} : {n,infinite} */
  }
  else {
    PUNFETCH;
    up = low;  /* {n} : exact n times */
  }

  if (PEND) return 1;
  PFETCH(c);
  if (c != '}') return 1;

  if (!IS_REPEAT_INFINITE(up) && low > up) {
    return REGERR_UPPER_SMALLER_THAN_LOWER_IN_REPEAT_RANGE;
  }

  *lower = low;
  *upper = up;
  *src = p;
  return 0;
}

static int
conv_backslash_value(int c)
{
  switch (c) {
  case 'n':  return '\n';
  case 't':  return '\t';
  case 'r':  return '\r';
  case 'f':  return '\f';
  case 'v':  return '\v';
  case 'a':  return '\007';
  case 'b':  return '\010';
  case 'e':  return '\033';
  }
  return c;
}


/* \M-, \C-, \c, or \... */
static int
scan_meta_control_backslash(UChar** src, UChar* end)
{
  int c;
  UChar* p = *src;

  if (PEND) return REGERR_END_PATTERN_AFTER_BACKSLASH;

  PFETCH(c);
  switch (c) {
  case 'M':
    if (PEND) return REGERR_END_PATTERN_AT_META;
    PFETCH(c);
    if (c != '-') return REGERR_META_CODE_SYNTAX;
    if (PEND) return REGERR_END_PATTERN_AT_META;
    PFETCH(c);
    if (c == '\\') {
      c = scan_meta_control_backslash(&p, end);
      if (c < 0) return c;
    }
    c = ((c & 0xff) | 0x80);
    break;

  case 'C':
    if (PEND) return REGERR_END_PATTERN_AT_CONTROL;
    PFETCH(c);
    if (c != '-') return REGERR_CONTROL_CODE_SYNTAX;

    /* fall through */
  case 'c':
    if (PEND) return REGERR_END_PATTERN_AT_CONTROL;
    PFETCH(c);
    if (c == '\\') {
      c = scan_meta_control_backslash(&p, end);
      if (c < 0) return c;
    }
    else if (c == '?')
      c = 0177;
    else
       c &= 0x9f;
    break;

  default:
    c = conv_backslash_value(c);
  }

  *src = p;
  return c;
}

static int
scan_backslash(UChar** src, UChar* end, ScanEnv* env, Node** node)
{
  int c, num;
  UChar* prev;
  UChar* p = *src;

  if (PEND) return REGERR_END_PATTERN_AT_BACKSLASH;

  PFETCH(c);
  switch (c) {
  case 'd': case 'D': case 's': case 'S':
    *node = node_new_cclass();
    CHECK_NULL_RETURN_VAL(*node, REGERR_MEMORY);
    switch (c) {
    case 'd':
      for (c = 0; c < SINGLE_BYTE_SIZE; c++) {
	if (IS_CODE_DIGIT(env->code,c)) BITSET_SET_BIT(NCCLASS(*node).bs, c);
      }
      break;
    case 'D':
      for (c = 0; c < SINGLE_BYTE_SIZE; c++) {
	if (!IS_CODE_DIGIT(env->code,c)) BITSET_SET_BIT(NCCLASS(*node).bs, c);
      }
      break;
    case 's':
      for (c = 0; c < SINGLE_BYTE_SIZE; c++) {
	if (IS_CODE_SPACE(env->code,c)) BITSET_SET_BIT(NCCLASS(*node).bs, c);
      }
      break;
    case 'S':
      for (c = 0; c < SINGLE_BYTE_SIZE; c++) {
	if (!IS_CODE_SPACE(env->code,c)) BITSET_SET_BIT(NCCLASS(*node).bs, c);
      }
      break;
    }
    break;

  case 'w':
    *node = node_new_ctype(CTYPE_WORD);
    break;
  case 'W':
    *node = node_new_ctype(CTYPE_NOT_WORD);
    break;

  case 'b':
    *node = node_new_ctype(CTYPE_WORD_BOUND);
    break;
  case 'B':
    *node = node_new_ctype(CTYPE_NOT_WORD_BOUND);
    break;

#ifdef USE_WORD_BEGIN_END
  case '<':
    *node = node_new_ctype(CTYPE_WORD_BEGIN);
    break;
  case '>':
    *node = node_new_ctype(CTYPE_WORD_END);
    break;
#endif

#ifdef USE_ONIGURUMA_EXTENSION
  case 'k':
    *node = node_new_ctype(CTYPE_WORD_SB);
    break;
  case 'K':
    *node = node_new_ctype(CTYPE_WORD_MB);
    break;
#endif

  case 'A':
    *node = node_new_anchor(ANCHOR_BEGIN_BUF);
    break;

  case 'Z':
    if (! IS_SINGLELINE(env->option)) {
      *node = node_new_anchor(ANCHOR_SEMI_END_BUF);
      break;
    }
    /* fall through */
  case 'z':
    *node = node_new_anchor(ANCHOR_END_BUF);
    break;

  case 'G':
    *node = node_new_anchor(ANCHOR_BEGIN_POSITION);
    break;

  case 'x':
    prev = p;
    num = scan_unsigned_hexadecimal_number(&p, end, 2);
    if (num < 0) return REGERR_TOO_BIG_NUMBER;
    if (p == prev) {  /* can't read nothing. */
      num = 0; /* but, it's not error */
    }
    goto string_raw;
    break;

  case '1': case '2': case '3': case '4':
  case '5': case '6': case '7': case '8': case '9':
    PUNFETCH;
    prev = p;
    num = scan_unsigned_number(&p, end);
    if (num < 0)  return REGERR_TOO_BIG_NUMBER;
    if (num > REG_MAX_BACKREF_NUM) return REGERR_TOO_BIG_BACKREF_NUMBER;

    if (num <= env->regnum || num <= 9) { /* ?? num <= 9, but it's spec. */
      *node = node_new_backref(num);
      break;
    }

    if (c == '8' || c == '9') {
      *node = node_new_str_char((UChar )c);
      p = prev + 1;
      break;
    }

    p = prev;
    /* fall through */
  case '0':
    prev = p;
    num = scan_unsigned_octal_number(&p, end, (c == '0' ? 2 : 3));
    if (num < 0) return REGERR_TOO_BIG_NUMBER;
    if (p == prev) {  /* can't read nothing. */
      num = 0; /* but, it's not error */
    }
    goto string_raw;
    break;

  default:
    PUNFETCH;
    num = scan_meta_control_backslash(&p, end);
    if (num < 0) return num;
    goto string_raw;
    break;
  }

 end:
  CHECK_NULL_RETURN_VAL(*node, REGERR_MEMORY);
  *src = p;
  return 0;

 string_raw:
  if (env->last_node && NTYPE(env->last_node) == N_STRING_RAW) {
    node_str_cat_char(env->last_node, (UChar )num);
    *src = p;
    return 1;  /* string_raw concat */
  }
  else {
    *node = node_new_str_raw_char(num);
    goto end;
  }
}

#define ADD_ALL_MULTI_BYTE_RANGE(code, mbuf) do {\
  if (! IS_SINGLEBYTE_CODE(code)) {\
    r = add_wc_range(&(mbuf), (WCInt )0x80, ~((WCInt )0),\
                    (UChar )0x80, (UChar )0xff);\
    if (r) return r;\
  }\
} while (0)

static int
scan_backslash_in_char_class(UChar** src, UChar* end,
			     RegCharEncoding code, Node* node, int* num)
{
  int c, r = 0;
  UChar* p = *src;
  BitSetRef bs;

  bs = NCCLASS(node).bs;

  if (PEND) return REGERR_END_PATTERN_AT_BACKSLASH;

  PFETCH(c);
  switch (c) {
  case 'd':
    for (c = 0; c < SINGLE_BYTE_SIZE; c++) {
      if (IS_CODE_DIGIT(code,c)) BITSET_SET_BIT(bs, c);
    }
    break;
  case 'D':
    for (c = 0; c < SINGLE_BYTE_SIZE; c++) {
      if (!IS_CODE_DIGIT(code,c)) BITSET_SET_BIT(bs, c);
    }
    ADD_ALL_MULTI_BYTE_RANGE(code, NCCLASS(node).mbuf);
    break;

  case 's':
    for (c = 0; c < SINGLE_BYTE_SIZE; c++) {
      if (IS_CODE_SPACE(code,c)) BITSET_SET_BIT(bs, c);
    }
    break;
  case 'S':
    for (c = 0; c < SINGLE_BYTE_SIZE; c++) {
      if (!IS_CODE_SPACE(code,c)) BITSET_SET_BIT(bs, c);
    }
    ADD_ALL_MULTI_BYTE_RANGE(code, NCCLASS(node).mbuf);
    break;

  case 'w':
    for (c = 0; c < SINGLE_BYTE_SIZE; c++) {
      if (IS_CODE_WORD(code,c)) BITSET_SET_BIT(bs, c);
    }
    ADD_ALL_MULTI_BYTE_RANGE(code, NCCLASS(node).mbuf);
    break;
  case 'W':
    for (c = 0; c < SINGLE_BYTE_SIZE; c++) {
      if (ISNOT_CODE_WORD(code,c)) {
	  BITSET_SET_BIT(bs, c);
      }
    }
    break;

#ifdef USE_ONIGURUMA_EXTENSION
  case 'k':
    for (c = 0; c < SINGLE_BYTE_SIZE; c++) {
      if (IS_SB_WORD(code,c)) BITSET_SET_BIT(bs, c);
    }
    break;

  case 'K':
    ADD_ALL_MULTI_BYTE_RANGE(code, NCCLASS(node).mbuf);
    break;
#endif

  case 'x':
    *num = scan_unsigned_hexadecimal_number(&p, end, 2);
    if (*num < 0) return REGERR_TOO_BIG_NUMBER;
    if (p == *src) {  /* can't read nothing. */
      *num = 0; /* but, it's not error */
    }
    r = 1; /* read number */
    break;

  case '0': case '1': case '2': case '3': case '4':
  case '5': case '6': case '7':
    PUNFETCH;
    *num = scan_unsigned_octal_number(&p, end, 3);
    if (*num < 0) return REGERR_TOO_BIG_NUMBER;
    if (p == *src) {  /* can't read nothing. */
      *num = 0; /* but, it's not error */
    }
    r = 1; /* read number */
    break;

  default:
    r = 1; /* read number */
    PUNFETCH;
    *num = scan_meta_control_backslash(&p, end);
    if (*num < 0) return *num;
  }

  *src = p;
  return r;
}


static void
bitset_by_pred_func(BitSetRef bs, int (*pf)(RegCharEncoding, UChar),
		    RegCharEncoding code, int not)
{
  int c;

  if (not) {
    for (c = 0; c < SINGLE_BYTE_SIZE; c++) {
      if (! pf(code,c)) BITSET_SET_BIT(bs, c);
    }
  }
  else {
    for (c = 0; c < SINGLE_BYTE_SIZE; c++) {
      if (pf(code,c)) BITSET_SET_BIT(bs, c);
    }
  }
}

typedef struct {
  UChar *name;
  int (*pf)(RegCharEncoding, UChar);
  short int len;
} PosixBracketEntryType;

static int
scan_posix_bracket_char_class(UChar** src, UChar* end,
			      RegCharEncoding code, Node* node)
{
#define POSIX_BRACKET_CHECK_LIMIT_LENGTH  16

#define CHECK_POSIX_BRACKET_END \
  if (end - p < 2 || *p != ':' || *(p+1) != ']')\
    return 1

  UChar *p = *src;
  int not = 0;
  BitSetRef bs;
  PosixBracketEntryType *pbsp;

  static PosixBracketEntryType PBS[] = {
    { "alnum",  is_code_alnum,  5 },
    { "alpha",  is_code_alpha,  5 },
    { "blank",  is_code_blank,  5 },
    { "cntrl",  is_code_cntrl,  5 },
    { "digit",  is_code_digit,  5 },
    { "graph",  is_code_graph,  5 },
    { "lower",  is_code_lower,  5 },
    { "print",  is_code_print,  5 },
    { "punct",  is_code_punct,  5 },
    { "space",  is_code_space,  5 },
    { "upper",  is_code_upper,  5 },
    { "xdigit", is_code_xdigit, 6 },
    { "ascii",  is_code_ascii,  5 }, /* I don't know origin. Perl? */
    { (UChar* )NULL, is_code_alnum, 0 }
  };

  bs = NCCLASS(node).bs;
  if (PPEEK == '^') {
    not = 1;
    PINC;
  }

  if (end - p < 7)
    goto check_bracket_end;

  for (pbsp = PBS; IS_NOT_NULL(pbsp->name); pbsp++) {
    if (k_strncmp(p, pbsp->name, pbsp->len) == 0) {
      p += pbsp->len;
      CHECK_POSIX_BRACKET_END;
      bitset_by_pred_func(bs, pbsp->pf, code, not);
      break;
    }
  }

  if (IS_NULL(pbsp->name)) {
    int i, c;

  check_bracket_end:
    i = 0;
    while ((c = PPEEK) != PEND_VALUE && c != ':' && c != ']') {
      PINC;
      if (++i > POSIX_BRACKET_CHECK_LIMIT_LENGTH) break;
    }
    if (c == ':' && *(p+1) == ']')
      return REGERR_INVALID_POSIX_BRACKET_TYPE;
    else
      return 1;   /* 1: is not POSIX bracket, but no error. */
  }

  p += 2;
  *src = p;
  return 0;
}

static int
scan_char_class(UChar** src, UChar* end, RegCharEncoding code, Node* node)
{
#define SCC_ST_START       0
#define SCC_ST_NORMAL      1
#define SCC_ST_RANGE       2

#define SCC_LAST_NONE      0
#define SCC_LAST_VALUE     1
#define SCC_LAST_MBVALUE   2
#define SCC_LAST_CLASS     3

#define SCC_BUF_LEN  8

  int c, r, isnum, last_val, state, last;
  int curr_mblen, curr_true_mblen, last_mblen, last_true_mblen;
  int curr_mbs_alloc_len, last_mbs_alloc_len;
  UChar curr_mbs_buf[SCC_BUF_LEN], *curr_mbs;
  UChar last_mbs_buf[SCC_BUF_LEN], *last_mbs;
  UChar* pp;
  UChar* p = *src;

  if (PEND) return REGERR_END_PATTERN_AT_LEFT_BRACKET;  /* "....[" */

  curr_mbs = curr_mbs_buf;
  last_mbs = last_mbs_buf;
  curr_mbs_alloc_len = SCC_BUF_LEN;
  last_mbs_alloc_len = SCC_BUF_LEN;

  PFETCH(c);
  if (c == '^') {
    NCCLASS(node).not = 1;
  }
  else  PUNFETCH;

  state = SCC_ST_START;
  last  = SCC_LAST_NONE;
  while (1) {
    if (PEND) {
      r = REGERR_PREMATURE_END_OF_CHAR_CLASS;
      goto err_exit;
    }

    isnum = 0;
    PFETCH(c);
    if (c == ']') {
      if ((p == *src + 1) ||
	  (NCCLASS(node).not && p == *src + 2)) { /* empty: [] or [^] */
	if (PEND) {
	  r = REGERR_EMPTY_CHAR_CLASS;
	  goto err_exit;
	}
      }
      else
	break; /* char-class terminate */
    }
    else if (c == '\\') {
      pp = p - 1;
      r = scan_backslash_in_char_class(&p, end, code, node, &c);
      if (r < 0) goto err_exit;
      if (r == 0) { /* char class */
	if (state == SCC_ST_RANGE) {
	  r = REGERR_CHAR_CLASS_VALUE_AT_END_OF_RANGE;
	  goto err_exit;
	}
	last  = SCC_LAST_CLASS;
	state = SCC_ST_NORMAL;
	continue;
      }
      else { /* 1: number */
	isnum = 1;
      }
    }
    else if (c == '[' && PPEEK == ':') {
      UChar* psave = p;
      p++; /* skip ":" */
      r = scan_posix_bracket_char_class(&p, end, code, node);
      if (r < 0) goto err_exit;
      if (r == 0) { /* char class */
	if (state == SCC_ST_RANGE) {
	  r = REGERR_CHAR_CLASS_VALUE_AT_END_OF_RANGE;
	  goto err_exit;
	}
	last  = SCC_LAST_CLASS;
	state = SCC_ST_NORMAL;
	continue;
      }
      else {
	p = psave;
      }
    }

    if (ismb(code, c)) {
      curr_mblen = mblen(code, c);
      if (curr_mblen > curr_mbs_alloc_len) {
	if (curr_mbs == curr_mbs_buf) {
	  curr_mbs = (UChar* )xmalloc(curr_mblen);
	}
	else {
	  curr_mbs = (UChar* )xrealloc(curr_mbs, curr_mblen);
	}
	if (IS_NULL(curr_mbs)) {
	  r = REGERR_MEMORY;
	  goto err_exit;
	}
	curr_mbs_alloc_len = curr_mblen;
      }

      if (isnum) {
	p = pp;
	curr_true_mblen = scan_backslash_multi_byte_sequence(&p, end, code,
							 curr_mbs, curr_mblen);
	if (curr_true_mblen < 0) {
	  r = curr_true_mblen;
	  goto err_exit;
	}
      }
      else {
	PUNFETCH;
	curr_true_mblen = ((end - p) > curr_mblen ? curr_mblen : (end - p));
	xmemcpy(curr_mbs, p, curr_true_mblen);
	p += curr_true_mblen;
      }

      if (state == SCC_ST_RANGE) {
#ifndef REG_RUBY_M17N
	if (last == SCC_LAST_VALUE && code == REGCODE_UTF8) {
	  /* allow only if utf-8 */
	  int i;
	  for (i = last_val; i <= 0x7f; i++) {
	    BITSET_SET_BIT(NCCLASS(node).bs, i);
	  }
	  last_mbs[0] = (UChar )0x80;
	  last_mbs[1] = (UChar )0;
	  last_true_mblen = 1;
	}
	else
#endif
	if (last != SCC_LAST_MBVALUE) {
	  r = REGERR_MISMATCH_CODE_LENGTH_IN_CLASS_RANGE;
	  goto err_exit;
	}
	last  = SCC_LAST_NONE;
      }
      else {
	if (curr_mblen > last_mbs_alloc_len) {
	  if (last_mbs == last_mbs_buf)
	    last_mbs = (UChar* )xmalloc(curr_mblen);
	  else
	    last_mbs = (UChar* )xrealloc(last_mbs, curr_mblen);
	  if (IS_NULL(last_mbs)) {
	    r = REGERR_MEMORY;
	    goto err_exit;
	  }
	  last_mbs_alloc_len = curr_mblen;
	}
	xmemcpy(last_mbs, curr_mbs, curr_true_mblen);
	last_mblen      = curr_mblen;
	last_true_mblen = curr_true_mblen;
	last  = SCC_LAST_MBVALUE;  /* It is correct. */
      }
      state = SCC_ST_NORMAL;

      r = add_multi_byte_range(&(NCCLASS(node).mbuf), code,
			       last_mbs, last_mbs + last_true_mblen,
			       curr_mbs, curr_mbs + curr_true_mblen);
      if (r < 0) goto err_exit;
    }
    else if (c == '-' && !isnum) {
      if (state == SCC_ST_START) goto normal_char;
      if (state == SCC_ST_RANGE) {
	r = REGERR_UNMATCHED_RANGE_SPECIFIER_IN_CHAR_CLASS;
	goto err_exit;
      }
      if (PPEEK == ']')          goto normal_char;

      if (last == SCC_LAST_NONE) {
	r = REGERR_UNMATCHED_RANGE_SPECIFIER_IN_CHAR_CLASS;
	goto err_exit;
      }
      if (last == SCC_LAST_CLASS) {
	r = REGERR_CHAR_CLASS_VALUE_AT_START_OF_RANGE;
	goto err_exit;
      }
      state = SCC_ST_RANGE;
    }
    else {
    normal_char:
      if (state == SCC_ST_RANGE) {
	if (last == SCC_LAST_VALUE) {
	  if (c < last_val) {
	    r = REGERR_RIGHT_SMALLER_THAN_LEFT_IN_CLASS_RANGE;
	    goto err_exit;
	  }
	  for (; last_val <= c; last_val++) {
	    BITSET_SET_BIT(NCCLASS(node).bs, last_val);
	  }
	}
	else { /* last: SCC_LAST_MBVALUE */
	  r = REGERR_MISMATCH_CODE_LENGTH_IN_CLASS_RANGE;
	  goto err_exit;
	}
	last  = SCC_LAST_NONE;
      }
      else {
	BITSET_SET_BIT(NCCLASS(node).bs, c);
	last_val = c;
	last  = SCC_LAST_VALUE;
      }
      state = SCC_ST_NORMAL;
    }
  }

  *src = p;
  r = 0;

 err_exit:
  if (curr_mbs && (curr_mbs != curr_mbs_buf))
    xfree(curr_mbs);
  if (last_mbs && (last_mbs != last_mbs_buf))
    xfree(last_mbs);

  return r;
}


/* scan code */
#define SC_END            0   /* end of input */
#define SC_VALUE          1   /* string, anychar, char class, back-ref */
#define SC_QUALIFIER      2   /* ?, +, *, {n,m} */
#define SC_ANCHOR         3   /* position */
#define SC_GROUP_START    4   /* ( */
#define SC_GROUP_END      5   /* ) */
#define SC_ALT            6   /* | */
#define SC_COMMENT        7   /* (# ...) */
#define SC_OPTION_ONLY    8   /* (?options) */

static int
scan_make_node(UChar** src, UChar* end, UChar term, ScanEnv* env, Node** rnode)
{
  int c, r;
  int lower, upper, type;
  UChar* p = *src;

  *rnode = NULL;

 start:
  if (PEND) {
    r = SC_END; goto end;
  }
  PFETCH(c);
  if (c == term && term != NOT_TERM) {
    PUNFETCH;
    r = SC_END; goto end;
  }

  switch (c) {
  case '(':  r = SC_GROUP_START; break;
  case ')':  r = SC_GROUP_END;   break;
  case '|':  r = SC_ALT;         break;

  case '^':
    type = (IS_SINGLELINE(env->option) ? ANCHOR_BEGIN_BUF : ANCHOR_BEGIN_LINE);
    *rnode = node_new_anchor(type);
    CHECK_NULL_RETURN_VAL(*rnode, REGERR_MEMORY);
    r = SC_ANCHOR;
    break;

  case '$':
    type = (IS_SINGLELINE(env->option) ? ANCHOR_END_BUF : ANCHOR_END_LINE);
    *rnode = node_new_anchor(type);
    CHECK_NULL_RETURN_VAL(*rnode, REGERR_MEMORY);
    r = SC_ANCHOR;
    break;

  case '?':
  case '+':
  case '*':
    lower = (c == '+' ? 1 : 0);
    upper = (c == '?' ? 1 : REPEAT_INFINITE);
    goto repeat_range;
    break;

  case '{':
    r = scan_range_qualifier(&p, end, &lower, &upper);
    if (r) {
      if (r > 0) {
	*src = p - 1;
	goto normal_string_2;
      }
      return r; /* error */
    }

  repeat_range:
    *rnode = node_new_qualifier(lower, upper, (c == '{' ? 1 : 0));
    CHECK_NULL_RETURN_VAL(*rnode, REGERR_MEMORY);
    if (!PEND) {
      PFETCH(c);
      if (c == '?')
	NQUALIFIER(*rnode).greedy = 0;
      else
	PUNFETCH;
    }
    r = SC_QUALIFIER;
    break;

  case '[':
    *rnode = node_new_cclass();
    CHECK_NULL_RETURN_VAL(*rnode, REGERR_MEMORY);
    r = scan_char_class(&p, end, env->code, *rnode);
    if (r) return r;
    r = SC_VALUE;
    break;

  case '.':
    *rnode = node_new_anychar();
    CHECK_NULL_RETURN_VAL(*rnode, REGERR_MEMORY);
    r = SC_VALUE;
    break;

  case '\\':
    r = scan_backslash(&p, end, env, rnode);
    if (r) {
      if (r == 1) goto start;
      return r;
    }
    r = SC_VALUE;
    break;

  case '#':
    if (IS_EXTEND(env->option)) {
      while (!PEND) {
	PFETCH(c);
	if (IS_NEWLINE(c))
	  goto start;
      }
      r = SC_END;
      break;
    }
    goto normal_string;

  case ' ': case '\t': case '\n': case '\r': case '\f':
    if (IS_EXTEND(env->option))
      goto start;
    /* fall through */

  default:
  normal_string:
    *src = p - 1;
    while (1) {
      if (c) {
	if (IS_EXTEND(env->option)) {
	  if (strchr("# \t\n\r\f", c)) {
	    PUNFETCH;
	    break;
	  }
	}
	if (strchr("()|^$?+*{[.\\", c)) {
	  PUNFETCH;
	  break;
	}
      }

      if (ismb(env->code, c)) {
	r = mblen(env->code, c);
	while (--r && !PEND) PFETCH(c);
      }
      if (PEND) break;
      PFETCH(c);
    }

  normal_string_2:
    if (env->last_node && NTYPE(env->last_node) == N_STRING) {
      node_str_cat(env->last_node, *src, p);
      goto start;
    }
    else {
      *rnode = node_new_str(*src, p);
      CHECK_NULL_RETURN_VAL(*rnode, REGERR_MEMORY);
      r = SC_VALUE;
    }
    break;
  }

 end:
  *src = p;
  return r;
}

static int scan_make_tree(UChar** src, UChar* end, UChar term, ScanEnv* env,
			  Node** top);

#define ONOFF(v,f,negative)    (negative) ? ((v) &= ~(f)) : ((v) |= (f))

static int
scan_make_paren(UChar** src, UChar* end, UChar term, ScanEnv* env,
		Node** top, int *last_is_group)
{
  Node *curr;
  RegOptionType option;
  int c, type, csub, mem, pnest;
  UChar* p = *src;

  *top = NULL;
  if (PEND) return REGERR_END_PATTERN_WITH_UNMATCHED_PARENTHESIS;

  option = env->option;
  *last_is_group = 0;

  PFETCH(c);
  if (c == '?') {
    if (PEND) return REGERR_END_PATTERN_AT_GROUP_OPTION;
    PFETCH(c);
    switch (c) {
    case '#':   /* (#...) comment */
      while (1) {
	if (PEND) return REGERR_END_PATTERN_AT_GROUP_COMMENT;
	PFETCH(c);
	if (c == ')') break;
      }
      break;

    case ':':            /* (?:...) grouping only */
    case '=': case '!':  /*         preceding read */
    case '>':            /* (?>...) stop backtrack */
      break;

    case '<':   /* look behind (?<=...), (?<!...) */
      PFETCH(csub);
      if (csub != '=' && csub != '!')
	return REGERR_UNDEFINED_GROUP_OPTION;
      break;

    case '-': case 'i': case 'm': case 'p': case 'x':
      {
	int neg = 0;

	while (1) {
	  switch (c) {
	  case ':':
	  case ')':
	  break;

	  case '-':  neg = 1; break;
	  case 'x':  ONOFF(option, REG_OPTION_EXTEND, neg);     break;
	  case 'i':  ONOFF(option, REG_OPTION_IGNORECASE, neg); break;
	  case 'm':  ONOFF(option, REG_OPTION_MULTILINE, neg);  break;
	  case 'p':
	    ONOFF(option, REG_OPTION_MULTILINE|REG_OPTION_SINGLELINE, neg);
	    break;

	  default:
	    return REGERR_UNDEFINED_GROUP_OPTION;
	  }

	  if (c == ':' || c == ')') break;
	  if (PEND) return REGERR_END_PATTERN_AT_GROUP_OPTION;
	  PFETCH(c);
	}
      }
      break;

    default:
      return REGERR_UNDEFINED_GROUP_OPTION;
    }
  }
  else {
    PUNFETCH;
    c = '(';
    env->regnum++;
    mem = env->regnum;
  }

  if (c == ')') {
    if (option == env->option) {
      *src = p;
      return SC_COMMENT;
    }
    else {
      *top = node_new_option(option);
      CHECK_NULL_RETURN_VAL(*top, REGERR_MEMORY);
      *src = p;
      return SC_OPTION_ONLY;
    }
  }

  pnest = env->paren_nest;
  env->paren_nest++;
  if (c == ':') { /* not in memory */
    if (env->option == option) {
      type = scan_make_tree(&p, end, term, env, top);
      *src = p;
      if (type == SC_END) {
	if (env->paren_nest != pnest)
	  return REGERR_END_PATTERN_WITH_UNMATCHED_PARENTHESIS;

	*last_is_group = 1;
      }
      return type;
    }
    else {
      RegOptionType prev = env->option;
      env->option = option;
      type = scan_make_tree(&p, end, term, env, &curr);
      env->option = prev;
      if (type < 0) return type;

      if (env->paren_nest != pnest)
	return REGERR_END_PATTERN_WITH_UNMATCHED_PARENTHESIS;

      *top = node_new_option(option);
      CHECK_NULL_RETURN_VAL(*top, REGERR_MEMORY);
      NOPTION(*top).target = curr;
      *src = p;
      return type;
    }
  }

  type = scan_make_tree(&p, end, term, env, &curr);
  if (type < 0) return type;
  if (env->paren_nest != pnest)
    return REGERR_END_PATTERN_WITH_UNMATCHED_PARENTHESIS;

  switch (c) {
  case '(':
    *top = node_new_effect(EFFECT_MEMORY);
    CHECK_NULL_RETURN_VAL(*top, REGERR_MEMORY);
    NEFFECT(*top).regnum = mem;
    break;

  case '=':
    *top = node_new_effect(EFFECT_PREC_READ);
    CHECK_NULL_RETURN_VAL(*top, REGERR_MEMORY);
    break;

  case '!':
    *top = node_new_effect(EFFECT_PREC_READ_NOT);
    CHECK_NULL_RETURN_VAL(*top, REGERR_MEMORY);
    break;

  case '>':
    *top = node_new_effect(EFFECT_STOP_BACKTRACK);
    CHECK_NULL_RETURN_VAL(*top, REGERR_MEMORY);
    break;

  case '<':
    {
      int etype;
      etype = (csub == '=' ? EFFECT_LOOK_BEHIND : EFFECT_LOOK_BEHIND_NOT);
      *top = node_new_effect(etype);
      CHECK_NULL_RETURN_VAL(*top, REGERR_MEMORY);
    }
    break;
  }
  NEFFECT(*top).target = curr;

  *src = p;
  return type;
}

static int
is_valid_qualifier_target(Node* node)
{
  switch (NTYPE(node)) {
  case N_CTYPE:
    switch (NCTYPE(node).type) {
    case CTYPE_WORD_BOUND:
    case CTYPE_NOT_WORD_BOUND:
#ifdef USE_WORD_BEGIN_END
    case CTYPE_WORD_BEGIN:
    case CTYPE_WORD_END:
#endif
      return 0;
      break;
    }
    break;

  case N_ANCHOR:
    return 0;
    break;

  case N_EFFECT:
    switch (NEFFECT(node).type) {
    case EFFECT_PREC_READ:
    case EFFECT_PREC_READ_NOT:
    case EFFECT_LOOK_BEHIND:
    case EFFECT_LOOK_BEHIND_NOT:
      return 0;
      break;
    }
    break;

  case N_OPTION:
    return is_valid_qualifier_target(NOPTION(node).target);
    break;

  case N_LIST: /* ex. (?:\G\A)* */
    do {
      if (is_valid_qualifier_target(NCONS(node).left)) return 1;
    } while (IS_NOT_NULL(node = NCONS(node).right));
    return 0;
    break;

  case N_ALT:  /* ex. (?:abc|\A)* */
    do {
      if (! is_valid_qualifier_target(NCONS(node).left)) return 0;
    } while (IS_NOT_NULL(node = NCONS(node).right));
    break;

  default:
    break;
  }
  return 1;
}

static int
scan_make_tree(UChar** src, UChar* end, UChar term, ScanEnv* env, Node** top)
{
  int type;
  int last_is_group = 0;   /* "(?:...)" */
  Node *curr, **last, **last_concat;
  QualifierNode* qn;

  *top = curr = NULL;
  last_concat = NULL;
  last = top;
  while (1) {
    env->last_node = curr;
    type = scan_make_node(src, end, term, env, &curr);
    switch (type) {
    case SC_END:
      goto end;
      break;

    case SC_QUALIFIER:  /* ?, +, *, {n,m} */
      if (IS_NULL(*last))
	return REGERR_TARGET_OF_REPEAT_OPERATOR_NOT_SPECIFIED;

      qn = &(NQUALIFIER(curr));
      if (qn->lower == 1 && qn->upper == 1) {
	if (! is_valid_qualifier_target(*last))
	  return REGERR_TARGET_OF_REPEAT_OPERATOR_INVALID;
	break;
      }

      switch (NTYPE(*last)) {
      case N_STRING:
      case N_STRING_RAW:
	if (!last_is_group) {
	  Node* n;
	  StrNode* sn = &(NSTRING(*last));

	  n = str_node_split_last_char(sn, NTYPE(*last), env->code);
	  if (IS_NOT_NULL(n)) {
	    NQUALIFIER(curr).target = n;
	    last_is_group = 0;
	    goto concat;
	  }
	}
	break;

      case N_QUALIFIER:
	{ /* check double repeat '**', '+*', "?+", "{1,infinite}*" etc. */
	  QualifierNode* qnlast = &(NQUALIFIER(*last));

	  if (qn->by_number == 0 && qnlast->by_number == 0) {
	    if ((qnlast->lower == 0 || qnlast->lower == 1)) {
	      if (IS_REPEAT_INFINITE(qnlast->upper) || /* "*", "+" */
		  qnlast->upper == 1) {                /* "?" */
		if (qn->lower <= 1 && IS_REPEAT_INFINITE(qn->upper))
		  return REGERR_NESTED_REPEAT_OPERATOR;
		if (qn->lower == 0 && qn->upper == 1)
		  return REGERR_NESTED_REPEAT_OPERATOR;
	      }
	    }
	  }
	}
	break;

      default:
	if (! is_valid_qualifier_target(*last))
	  return REGERR_TARGET_OF_REPEAT_OPERATOR_INVALID;
	break;
      }

      NQUALIFIER(curr).target = *last;
      *last = curr;
      last_is_group = 0;
      break;

    case SC_VALUE:    /* string, anychar, char class, back-ref */
    case SC_ANCHOR:
    last_is_group = 0;
    concat:
      {
	if (IS_NULL(last_concat) && IS_NULL(*last)) {
	  *last = curr;
	}
	else {
	  if (IS_NULL(last_concat)) {
	    *last = node_new_list(*last, NULL);
	    last_concat = &(NCONS(*last).right);
	  }
	  *last_concat = node_new_list(curr, NULL);
	  last = &(NCONS(*last_concat).left);
	  last_concat = &(NCONS(*last_concat).right);
	}
      }
      break;

    case SC_ALT:
      {
	if (IS_NULL(*top)) {
	  *top = node_new_empty();
	}
	*top = node_new_alt(*top, NULL);
	env->last_node = NULL;
	type = scan_make_tree(src, end, term, env, &(NCONS(*top).right));
	if (type == SC_END && NTYPE(NCONS(*top).right) != N_ALT) {
	  NCONS(*top).right = node_new_alt(NCONS(*top).right, NULL);
	}
	return type;
      }
      break;

    case SC_GROUP_START:
      type = scan_make_paren(src, end, NOT_TERM, env, &curr, &last_is_group);
      if (type == SC_END) {
	goto concat;
      }
      else if (type == SC_OPTION_ONLY) {
	RegOptionType prev = env->option;
	env->option = NOPTION(curr).option;
	type = scan_make_tree(src, end, ')', env, &(NOPTION(curr).target));
	env->option = prev;
	if (type < 0) return type;
	goto concat;
      }
      else if (type == SC_COMMENT) {
	continue;
      }
      else
	return type;
      break;

    case SC_GROUP_END:
      if (env->paren_nest <= 0) {
	return REGERR_UNMATCHED_RIGHT_PARENTHESIS;
      }
      env->paren_nest--;
      goto end;
      break;

    default: /* erorr code */
      return type;
    }
  }

 end:
  if (IS_NULL(*top))
    *top = node_new_empty();
  return SC_END;
}

static int
parse_make_tree(Node** root, UChar* pattern, UChar* end, regex_t* reg)
{
  int r;
  UChar* p;
  ScanEnv env;

  env.option     = reg->options;
  env.code       = reg->code;
  env.regnum     = 0;
  env.last_node  = NULL;
  env.paren_nest = 0;

  *root = NULL;
  p = pattern;
  r = scan_make_tree(&p, end, NOT_TERM, &env, root);
  reg->max_mem = env.regnum;
  return r;
}


/* operation code */
enum OpCode {
  OP_FINISH = 0,        /* matching process terminator (no more alternative) */
  OP_END    = 1,        /* pattern code terminator (success end) */

  OP_EXACT1 = 2,        /* single byte, N = 1 */
  OP_EXACT2,            /* single byte, N = 2 */
  OP_EXACT3,            /* single byte, N = 3 */
  OP_EXACT4,            /* single byte, N = 4 */
  OP_EXACT5,            /* single byte, N = 5 */
  OP_EXACTN,            /* single byte */
  OP_EXACTMB2N1,        /* mb-length = 2 N = 1 */
  OP_EXACTMB2N2,        /* mb-length = 2 N = 2 */
  OP_EXACTMB2N3,        /* mb-length = 2 N = 3 */
  OP_EXACTMB2N,         /* mb-length = 2 */
  OP_EXACTMB3N,         /* mb length = 3 */
  OP_EXACTMBN,          /* other length */

  OP_EXACT1_IC,         /* single byte, N = 1, ignore case */
  OP_EXACTN_IC,         /* single byte,        ignore case */

  OP_CCLASS,
  OP_CCLASS_MB,
  OP_CCLASS_MIX,
  OP_CCLASS_NOT,
  OP_CCLASS_MB_NOT,
  OP_CCLASS_MIX_NOT,

  OP_ANYCHAR,                /* "."  */
  OP_ANYCHAR_STAR,           /* ".*" */
  OP_ANYCHAR_STAR_PEEK_NEXT,

  OP_WORD,
  OP_NOT_WORD,
  OP_WORD_SB,
  OP_WORD_MB,
  OP_WORD_BOUND,
  OP_NOT_WORD_BOUND,
  OP_WORD_BEGIN,
  OP_WORD_END,

  OP_BEGIN_BUF,
  OP_END_BUF,
  OP_BEGIN_LINE,
  OP_END_LINE,
  OP_SEMI_END_BUF,
  OP_BEGIN_POSITION,

  OP_BACKREF1,
  OP_BACKREF2,
  OP_BACKREF3,
  OP_BACKREFN,

  OP_MEMORY_START_PUSH,  /* push stack */
  OP_MEMORY_END_PUSH,    /* push stack */
  OP_MEMORY_END,

  OP_SET_OPTION_PUSH,    /* set option and push recover option */
  OP_SET_OPTION,         /* set option */

  OP_FAIL,               /* pop stack and move */
  OP_JUMP,
  OP_PUSH,
  OP_PUSH_OR_JUMP_EXACT1, /* if match exact then push, else jump. */
  OP_PUSH_IF_PEEK_NEXT,   /* if match exact then push, else none. */
  OP_REPEAT,             /* {n,m} */
  OP_REPEAT_NG,          /* {n,m}? (non greedy) */
  OP_REPEAT_INC,
  OP_REPEAT_INC_NG,      /* non greedy */
  OP_NULL_CHECK_START,   /* null loop checker start */
  OP_NULL_CHECK_END,     /* null loop checker end   */

  OP_PUSH_POS,             /* (?=...)  start */
  OP_POP_POS,              /* (?=...)  end   */
  OP_PUSH_POS_NOT,         /* (?!...)  start */
  OP_FAIL_POS,             /* (?!...)  end   */
  OP_PUSH_STOP_BT,         /* (?>...)  start */
  OP_POP_STOP_BT,          /* (?>...)  end   */
  OP_LOOK_BEHIND,          /* (?<=...) start (no needs end opcode) */
  OP_PUSH_LOOK_BEHIND_NOT, /* (?<!...) start */
  OP_FAIL_LOOK_BEHIND_NOT  /* (?<!...) end   */
};

/* arguments type */
#define ARG_SPECIAL     -1
#define ARG_NON          0
#define ARG_RELADDR      1
#define ARG_LENGTH       2
#define ARG_MEMNUM       3
#define ARG_OPTION       4


#define INVALID_STACK_INDEX   -1
typedef int  StackIndex;

typedef struct _StackType {
  int type;
  union {
    struct {
      UChar *pcode;      /* byte code position */
      UChar *pstr;       /* string position */
      UChar *pstr_prev;  /* previous char position of pstr */
    } state;
    struct {
      UChar *pcode;      /* byte code position (head of repeated target) */
      int lower;         /* for OP_REPEAT_INC, OP_REPEAT_INC_NG */
      int upper;         /* for OP_REPEAT_INC, OP_REPEAT_INC_NG */
      int count;         /* for OP_REPEAT_INC, OP_REPEAT_INC_NG */
    } repeat;
    struct {
      StackIndex si;     /* index of stack */
    } repeat_inc;
    struct {
      int num;           /* memory num */
      UChar *pstr;       /* start/end position */
      /* Following information is setted, if this stack type is MEM-START */
      StackIndex start;  /* prev. info (for backtrack  "(...)*" ) */
      StackIndex end;    /* prev. info (for backtrack  "(...)*" ) */
    } mem;
    struct {
      int num;
      UChar *pstr;              /* start position */
    } null_check;
  } u;
} StackType;


typedef short int   RelAddrType;
typedef short int   LengthType;
typedef short int   MemNumType;
typedef int         RepeatNumType;

#define SIZE_OPCODE        1
#define SIZE_RELADDR       sizeof(RelAddrType)
#define SIZE_LENGTH        sizeof(LengthType)
#define SIZE_MEMNUM        sizeof(MemNumType)
#define SIZE_REPEATNUM     sizeof(RepeatNumType)
#define SIZE_OPTION        sizeof(RegOptionType)

#ifdef UNALIGNED_WORD_ACCESS
#define GET_RELADDR_INC(addr,p) do{\
  addr = *((RelAddrType* )(p));\
  (p) += SIZE_RELADDR;\
} while(0)

#define GET_LENGTH_INC(len,p) do{\
  len = *((LengthType* )(p));\
  (p) += SIZE_LENGTH;\
} while(0)

#define GET_MEMNUM_INC(num,p) do{\
  num = *((MemNumType* )(p));\
  (p) += SIZE_MEMNUM;\
} while(0)

#define GET_REPEATNUM_INC(num,p) do{\
  num = *((RepeatNumType* )(p));\
  (p) += SIZE_REPEATNUM;\
} while(0)

#define GET_OPTION_INC(option,p) do{\
  option = *((RegOptionType* )(p));\
  (p) += SIZE_OPTION;\
} while(0)
#else

#define GET_RELADDR_INC(addr,p)      GET_SHORT_INC(addr,p)
#define GET_LENGTH_INC(len,p)        GET_SHORT_INC(len,p)
#define GET_MEMNUM_INC(num,p)        GET_SHORT_INC(num,p)
#define GET_REPEATNUM_INC(num,p)     GET_INT_INC(num,p)
#define GET_OPTION_INC(option,p)     GET_UINT_INC(option,p)

#define SERIALIZE_RELADDR(addr,p)    SERIALIZE_SHORT(addr,p)
#define SERIALIZE_LENGTH(len,p)      SERIALIZE_SHORT(len,p)
#define SERIALIZE_MEMNUM(num,p)      SERIALIZE_SHORT(num,p)
#define SERIALIZE_REPEATNUM(num,p)   SERIALIZE_INT(num,p)
#define SERIALIZE_OPTION(option,p)   SERIALIZE_UINT(option,p)

#define SERIALIZE_BUFSIZE            SIZEOF_INT

#endif  /* UNALIGNED_WORD_ACCESS */

#define GET_BYTE_INC(byte,p) do{\
  byte = *(p);\
  (p)++;\
} while(0)


/* op-code + arg size */
#define SIZE_OP_ANYCHAR_STAR            SIZE_OPCODE
#define SIZE_OP_ANYCHAR_STAR_PEEK_NEXT (SIZE_OPCODE + 1)
#define SIZE_OP_JUMP                   (SIZE_OPCODE + SIZE_RELADDR)
#define SIZE_OP_PUSH                   (SIZE_OPCODE + SIZE_RELADDR)
#define SIZE_OP_PUSH_OR_JUMP_EXACT1    (SIZE_OPCODE + SIZE_RELADDR + 1)
#define SIZE_OP_PUSH_IF_PEEK_NEXT      (SIZE_OPCODE + SIZE_RELADDR + 1)
#define SIZE_OP_REPEAT_INC             (SIZE_OPCODE + SIZE_MEMNUM)
#define SIZE_OP_REPEAT_INC_NG          (SIZE_OPCODE + SIZE_MEMNUM)
#define SIZE_OP_PUSH_POS                SIZE_OPCODE
#define SIZE_OP_PUSH_POS_NOT           (SIZE_OPCODE + SIZE_RELADDR)
#define SIZE_OP_POP_POS                 SIZE_OPCODE
#define SIZE_OP_FAIL_POS                SIZE_OPCODE
#define SIZE_OP_SET_OPTION             (SIZE_OPCODE + SIZE_OPTION)
#define SIZE_OP_SET_OPTION_PUSH        (SIZE_OPCODE + SIZE_OPTION)
#define SIZE_OP_FAIL                    SIZE_OPCODE
#define SIZE_OP_MEMORY_START_PUSH      (SIZE_OPCODE + SIZE_MEMNUM)
#define SIZE_OP_MEMORY_END_PUSH        (SIZE_OPCODE + SIZE_MEMNUM)
#define SIZE_OP_MEMORY_END             (SIZE_OPCODE + SIZE_MEMNUM)
#define SIZE_OP_PUSH_STOP_BT            SIZE_OPCODE
#define SIZE_OP_POP_STOP_BT             SIZE_OPCODE
#define SIZE_OP_NULL_CHECK_START       (SIZE_OPCODE + SIZE_MEMNUM)
#define SIZE_OP_NULL_CHECK_END         (SIZE_OPCODE + SIZE_MEMNUM)
#define SIZE_OP_LOOK_BEHIND            (SIZE_OPCODE + SIZE_LENGTH)
#define SIZE_OP_PUSH_LOOK_BEHIND_NOT   (SIZE_OPCODE + SIZE_RELADDR + SIZE_LENGTH)
#define SIZE_OP_FAIL_LOOK_BEHIND_NOT    SIZE_OPCODE

#define IS_NEED_STR_LEN_OP_EXACT(op) \
   ((op) == OP_EXACTN    || (op) == OP_EXACTMB2N ||\
    (op) == OP_EXACTMB3N || (op) == OP_EXACTMBN  || (op) == OP_EXACTN_IC)


static int
add_opcode(regex_t* reg, int opcode)
{
  BBUF_ADD1(reg, opcode);
  return 0;
}

static int
add_rel_addr(regex_t* reg, int addr)
{
  RelAddrType ra = (RelAddrType )addr;

#ifdef UNALIGNED_WORD_ACCESS
  BBUF_ADD(reg, &ra, SIZE_RELADDR);
#else
  UChar buf[SERIALIZE_BUFSIZE];
  SERIALIZE_RELADDR(ra, buf);
  BBUF_ADD(reg, buf, SIZE_RELADDR);
#endif
  return 0;
}

static int
add_length(regex_t* reg, int len)
{
  LengthType l = (LengthType )len;

#ifdef UNALIGNED_WORD_ACCESS
  BBUF_ADD(reg, &l, SIZE_LENGTH);
#else
  UChar buf[SERIALIZE_BUFSIZE];
  SERIALIZE_LENGTH(l, buf);
  BBUF_ADD(reg, buf, SIZE_LENGTH);
#endif
  return 0;
}

static int
add_mem_num(regex_t* reg, int num)
{
  MemNumType n = (MemNumType )num;

#ifdef UNALIGNED_WORD_ACCESS
  BBUF_ADD(reg, &n, SIZE_MEMNUM);
#else
  UChar buf[SERIALIZE_BUFSIZE];
  SERIALIZE_MEMNUM(n, buf);
  BBUF_ADD(reg, buf, SIZE_MEMNUM);
#endif
  return 0;
}

static int
add_repeat_num(regex_t* reg, int num)
{
  RepeatNumType n = (RepeatNumType )num;

#ifdef UNALIGNED_WORD_ACCESS
  BBUF_ADD(reg, &n, SIZE_REPEATNUM);
#else
  UChar buf[SERIALIZE_BUFSIZE];
  SERIALIZE_REPEATNUM(n, buf);
  BBUF_ADD(reg, buf, SIZE_REPEATNUM);
#endif
  return 0;
}

static int
add_option(regex_t* reg, RegOptionType option)
{
#ifdef UNALIGNED_WORD_ACCESS
  BBUF_ADD(reg, &option, SIZE_OPTION);
#else
  UChar buf[SERIALIZE_BUFSIZE];
  SERIALIZE_OPTION(option, buf);
  BBUF_ADD(reg, buf, SIZE_OPTION);
#endif
  return 0;
}

static int
add_opcode_rel_addr(regex_t* reg, int opcode, int addr)
{
  int r;

  r = add_opcode(reg, opcode);
  if (r) return r;
  r = add_rel_addr(reg, addr);
  return r;
}

static int
add_bytes(regex_t* reg, UChar* bytes, int len)
{
  BBUF_ADD(reg, bytes, len);
  return 0;
}

static int
add_bitset(regex_t* reg, BitSetRef bs)
{
  BBUF_ADD(reg, bs, SIZE_BITSET);
  return 0;
}

static int
add_opcode_option(regex_t* reg, int opcode, RegOptionType option)
{
  int r;

  r = add_opcode(reg, opcode);
  if (r) return r;
  r = add_option(reg, option);
  return r;
}

static int compile_length_tree(Node* node, regex_t* reg);
static int compile_tree(Node* node, regex_t* reg);

static int
select_str_opcode(int mb_len, int str_len, int ignore_case)
{
  int op;

  switch (mb_len) {
  case 1:
    if (ignore_case) {
      switch (str_len) {
      case 1:  op = OP_EXACT1_IC; break;
      default: op = OP_EXACTN_IC; break;
      }
    }
    else {
      switch (str_len) {
      case 1:  op = OP_EXACT1; break;
      case 2:  op = OP_EXACT2; break;
      case 3:  op = OP_EXACT3; break;
      case 4:  op = OP_EXACT4; break;
      case 5:  op = OP_EXACT5; break;
      default: op = OP_EXACTN; break;
      }
    }
    break;

  case 2:
    switch (str_len) {
    case 1:  op = OP_EXACTMB2N1; break;
    case 2:  op = OP_EXACTMB2N2; break;
    case 3:  op = OP_EXACTMB2N3; break;
    default: op = OP_EXACTMB2N; break;
    }
    break;

  case 3:
    op = OP_EXACTMB3N;
    break;

  default:
    op = OP_EXACTMBN;
    break;
  }
  return op;
}

static int
compile_tree_empty_check(Node* node, regex_t* reg, int empty)
{
  int r;
  int saved_num_null_check = reg->num_null_check;

  if (empty) {
    r = add_opcode(reg, OP_NULL_CHECK_START);
    if (r) return r;
    r = add_mem_num(reg, reg->num_null_check); /* NULL CHECK ID */
    if (r) return r;
    reg->num_null_check++;
  }

  r = compile_tree(node, reg);
  if (r) return r;

  if (empty) {
    r = add_opcode(reg, OP_NULL_CHECK_END);
    if (r) return r;
    r = add_mem_num(reg, saved_num_null_check); /* NULL CHECK ID */
  }
  return r;
}

static int
compile_tree_n_times(Node* node, int n, regex_t* reg)
{
  int i, r;

  for (i = 0; i < n; i++) {
    r = compile_tree(node, reg);
    if (r) return r;
  }
  return 0;
}

static int
add_compile_string_length(UChar* s, int mb_len, int str_len, regex_t* reg)
{
  int len;
  int op = select_str_opcode(mb_len, str_len, IS_IGNORECASE(reg->options));

  len = SIZE_OPCODE;
  if (op == OP_EXACTMBN)
    len += SIZE_LENGTH;

  if (IS_NEED_STR_LEN_OP_EXACT(op))
    len += SIZE_LENGTH;

  len += mb_len * str_len;
  return len;
}

static int
add_compile_string(UChar* s, int mb_len, int str_len, regex_t* reg)
{
  int op = select_str_opcode(mb_len, str_len, IS_IGNORECASE(reg->options));
  add_opcode(reg, op);

  if (op == OP_EXACTMBN)
    add_length(reg, mb_len);

  if (IS_NEED_STR_LEN_OP_EXACT(op))
    add_length(reg, str_len);

  add_bytes(reg, s, mb_len * str_len);
  return 0;
}


static int
compile_length_string_node(StrNode* sn, regex_t* reg)
{
  int rlen, r, len, prev_len, slen;
  RegCharEncoding code = reg->code;
  UChar *p, *prev;

  if (sn->end <= sn->s)
    return 0;

  p = prev = sn->s;
  prev_len = mblen(code, *p);
  p += prev_len;
  slen = 1;
  rlen = 0;

  for (; p < sn->end; ) {
    len = mblen(code, *p);
    if (len == prev_len) {
      slen++;
    }
    else {
      r = add_compile_string_length(prev, prev_len, slen, reg);
      rlen += r;
      prev = p;
      slen = 1;
      prev_len = len;
    }

    p += len;
  }
  r = add_compile_string_length(prev, prev_len, slen, reg);
  rlen += r;
  return rlen;
}

static int
compile_length_string_raw_node(StrNode* sn, regex_t* reg)
{
  int len;
  RegOptionType option;

  if (sn->end <= sn->s)
    return 0;

  option = reg->options;
  REG_OPTION_OFF(reg->options, REG_OPTION_IGNORECASE);
  len = add_compile_string_length(sn->s, 1 /* sb */, sn->end - sn->s, reg);
  reg->options = option;
  return len;
}

static int
compile_string_node(StrNode* sn, regex_t* reg)
{
  int r, len, prev_len, slen;
  RegCharEncoding code = reg->code;
  UChar *p, *prev;

  if (sn->end <= sn->s)
    return 0;

  p = prev = sn->s;
  prev_len = mblen(code, *p);
  p += prev_len;
  slen = 1;

  for (; p < sn->end; ) {
    len = mblen(code, *p);
    if (len == prev_len) {
      slen++;
    }
    else {
      r = add_compile_string(prev, prev_len, slen, reg);
      if (r) return r;
      prev = p;
      slen = 1;
      prev_len = len;
    }

    p += len;
  }
  return add_compile_string(prev, prev_len, slen, reg);
}

static int
compile_string_raw_node(StrNode* sn, regex_t* reg)
{
  int r;
  RegOptionType option;

  if (sn->end <= sn->s)
    return 0;

  option = reg->options;
  REG_OPTION_OFF(reg->options, REG_OPTION_IGNORECASE);
  r = add_compile_string(sn->s, 1 /* sb */, sn->end - sn->s, reg);
  reg->options = option;
  return r;
}

static int
add_multi_byte_cclass_offset(BBuf* mbuf, regex_t* reg, int offset)
{
#ifdef UNALIGNED_WORD_ACCESS
  add_length(reg, mbuf->used - offset);
  return add_bytes(reg, mbuf->p + offset, mbuf->used - offset);
#else
  int r, pad_size;
  UChar* p = BBUF_GET_ADD_POSITION(reg) + SIZE_LENGTH;

  GET_ALIGNMENT_PAD_SIZE(p, pad_size);
  add_length(reg, mbuf->used - offset + (WORD_ALIGNMENT_SIZE - 1));
  if (pad_size) add_bytes(reg, PadBuf, pad_size);

  r = add_bytes(reg, mbuf->p + offset, mbuf->used - offset);

  pad_size = (WORD_ALIGNMENT_SIZE - 1) - pad_size;
  if (pad_size) add_bytes(reg, PadBuf, pad_size);
  return r;
#endif
}

static int
compile_length_cclass_node(CClassNode* cc, regex_t* reg)
{
  int len;

  if (IS_NULL(cc->mbuf)) {
    len = SIZE_OPCODE + SIZE_BITSET;
  }
  else {
    if (bitset_is_empty(cc->bs)) {
      len = SIZE_OPCODE;  /* SIZE_BITSET is included in mbuf->used. */
    }
    else {
      len = SIZE_OPCODE;  /* SIZE_BITSET is included in mbuf->used. */
    }
#ifdef UNALIGNED_WORD_ACCESS
    len += SIZE_LENGTH + cc->mbuf->used;
#else
    len += SIZE_LENGTH + cc->mbuf->used + (WORD_ALIGNMENT_SIZE - 1);
#endif
  }

  return len;
}

static int
compile_cclass_node(CClassNode* cc, regex_t* reg)
{
  int r;

  if (IS_NULL(cc->mbuf)) {
    if (cc->not)  add_opcode(reg, OP_CCLASS_NOT);
    else          add_opcode(reg, OP_CCLASS);

    r = add_bitset(reg, cc->bs);
  }
  else {
    if (bitset_is_empty(cc->bs)) {
      if (cc->not)  add_opcode(reg, OP_CCLASS_MB_NOT);
      else          add_opcode(reg, OP_CCLASS_MB);

      r = add_bitset(reg, (BitSetRef )cc->mbuf->p);
      if (r) return r;
      r = add_multi_byte_cclass_offset(cc->mbuf, reg, SIZE_BITSET);
    }
    else {
      if (cc->not)  add_opcode(reg, OP_CCLASS_MIX_NOT);
      else          add_opcode(reg, OP_CCLASS_MIX);

      r = add_bitset(reg, cc->bs);
      if (r) return r;
      r = add_multi_byte_cclass_offset(cc->mbuf, reg, SIZE_BITSET);
    }
  }

  return r;
}

static int
compile_range_repeat_node(QualifierNode* qn, int target_len, regex_t* reg)
{
  int r;
  int saved_num_repeat = reg->num_repeat;

  r = add_opcode(reg, qn->greedy ? OP_REPEAT : OP_REPEAT_NG);
  if (r) return r;
  r = add_mem_num(reg, reg->num_repeat); /* OP_REPEAT ID */
  reg->num_repeat++;
  if (r) return r;
  r = add_rel_addr(reg, target_len + SIZE_OP_REPEAT_INC);
  if (r) return r;
  r = add_repeat_num(reg, qn->lower);
  if (r) return r;
  r = add_repeat_num(reg, qn->upper);
  if (r) return r;

  r = compile_tree_empty_check(qn->target, reg, qn->target_empty);
  if (r) return r;

  r = add_opcode(reg, qn->greedy ? OP_REPEAT_INC : OP_REPEAT_INC_NG);
  if (r) return r;
  r = add_mem_num(reg, saved_num_repeat); /* OP_REPEAT ID */
  return r;
}

#define QUALIFIER_EXPAND_LIMIT_SIZE   50

static int
compile_length_qualifier_node(QualifierNode* qn, regex_t* reg)
{
  int len, mod_tlen;
  int infinite = IS_REPEAT_INFINITE(qn->upper);
  int tlen = compile_length_tree(qn->target, reg);

  if (tlen < 0) return tlen;

  /* anychar repeat */
  if (NTYPE(qn->target) == N_ANYCHAR) {
    if (qn->greedy && infinite) {
      if (IS_NOT_NULL(qn->next_head_exact))
	return SIZE_OP_ANYCHAR_STAR_PEEK_NEXT + tlen * qn->lower;
      else
	return SIZE_OP_ANYCHAR_STAR + tlen * qn->lower;
    }
  }

  if (qn->target_empty)
    mod_tlen = tlen + (SIZE_OP_NULL_CHECK_START + SIZE_OP_NULL_CHECK_END);
  else
    mod_tlen = tlen;

  if (infinite &&
      (qn->lower <= 1 || tlen * qn->lower <= QUALIFIER_EXPAND_LIMIT_SIZE)) {
    if (qn->lower == 1 && tlen > QUALIFIER_EXPAND_LIMIT_SIZE) {
      len = SIZE_OP_JUMP;
    }
    else {
      len = tlen * qn->lower;
    }

    if (qn->greedy) {
      if (IS_NOT_NULL(qn->head_exact))
	len += SIZE_OP_PUSH_OR_JUMP_EXACT1 + mod_tlen + SIZE_OP_JUMP;
      else if (IS_NOT_NULL(qn->next_head_exact))
	len += SIZE_OP_PUSH_IF_PEEK_NEXT + mod_tlen + SIZE_OP_JUMP;
      else
	len += SIZE_OP_PUSH + mod_tlen + SIZE_OP_JUMP;
    }
    else
      len += SIZE_OP_JUMP + mod_tlen + SIZE_OP_PUSH;
  }
  else if (!infinite && qn->greedy &&
	   (tlen + SIZE_OP_PUSH) * qn->upper <= QUALIFIER_EXPAND_LIMIT_SIZE) {
    len = tlen * qn->lower;
    len += (SIZE_OP_PUSH + tlen) * (qn->upper - qn->lower);
  }
  else {
    len = SIZE_OP_REPEAT_INC + SIZE_REPEATNUM * 2 + SIZE_MEMNUM
        + mod_tlen + SIZE_OPCODE + SIZE_RELADDR;
  }

  return len;
}

static int
is_anychar_star_qualifier(QualifierNode* qn)
{
  if (qn->greedy && IS_REPEAT_INFINITE(qn->upper) &&
      NTYPE(qn->target) == N_ANYCHAR)
    return 1;
  else
    return 0;
}

static int
compile_qualifier_node(QualifierNode* qn, regex_t* reg)
{
  int i, r, mod_tlen;
  int infinite = IS_REPEAT_INFINITE(qn->upper);
  int tlen = compile_length_tree(qn->target, reg);

  if (tlen < 0) return tlen;

  if (is_anychar_star_qualifier(qn)) {
    r = compile_tree_n_times(qn->target, qn->lower, reg);
    if (r) return r;
    if (IS_NOT_NULL(qn->next_head_exact)) {
      r = add_opcode(reg, OP_ANYCHAR_STAR_PEEK_NEXT);
      if (r) return r;
      return add_bytes(reg, NSTRING(qn->next_head_exact).s, 1);
    }
    else
      return add_opcode(reg, OP_ANYCHAR_STAR);
  }

  if (qn->target_empty)
    mod_tlen = tlen + (SIZE_OP_NULL_CHECK_START + SIZE_OP_NULL_CHECK_END);
  else
    mod_tlen = tlen;

  if (infinite &&
      (qn->lower <= 1 || tlen * qn->lower <= QUALIFIER_EXPAND_LIMIT_SIZE)) {
    if (qn->lower == 1 && tlen > QUALIFIER_EXPAND_LIMIT_SIZE) {
      if (qn->greedy) {
	if (IS_NOT_NULL(qn->head_exact))
	  r = add_opcode_rel_addr(reg, OP_JUMP, SIZE_OP_PUSH_OR_JUMP_EXACT1);
	else if (IS_NOT_NULL(qn->next_head_exact))
	  r = add_opcode_rel_addr(reg, OP_JUMP, SIZE_OP_PUSH_IF_PEEK_NEXT);
	else
	  r = add_opcode_rel_addr(reg, OP_JUMP, SIZE_OP_PUSH);
      }
      else {
	r = add_opcode_rel_addr(reg, OP_JUMP, SIZE_OP_JUMP);
      }
      if (r) return r;
    }
    else {
      r = compile_tree_n_times(qn->target, qn->lower, reg);
      if (r) return r;
    }

    if (qn->greedy) {
      if (IS_NOT_NULL(qn->head_exact)) {
	r = add_opcode_rel_addr(reg, OP_PUSH_OR_JUMP_EXACT1,
			     mod_tlen + SIZE_OP_JUMP);
	if (r) return r;
	add_bytes(reg, NSTRING(qn->head_exact).s, 1);
	r = compile_tree_empty_check(qn->target, reg, qn->target_empty);
	if (r) return r;
	r = add_opcode_rel_addr(reg, OP_JUMP,
		     -(mod_tlen + SIZE_OP_JUMP + SIZE_OP_PUSH_OR_JUMP_EXACT1));
      }
      else if (IS_NOT_NULL(qn->next_head_exact)) {
	r = add_opcode_rel_addr(reg, OP_PUSH_IF_PEEK_NEXT,
				mod_tlen + SIZE_OP_JUMP);
	if (r) return r;
	add_bytes(reg, NSTRING(qn->next_head_exact).s, 1);
	r = compile_tree_empty_check(qn->target, reg, qn->target_empty);
	if (r) return r;
	r = add_opcode_rel_addr(reg, OP_JUMP,
		     -(mod_tlen + SIZE_OP_JUMP + SIZE_OP_PUSH_IF_PEEK_NEXT));
      }
      else {
	r = add_opcode_rel_addr(reg, OP_PUSH, mod_tlen + SIZE_OP_JUMP);
	if (r) return r;
	r = compile_tree_empty_check(qn->target, reg, qn->target_empty);
	if (r) return r;
	r = add_opcode_rel_addr(reg, OP_JUMP,
			     -(mod_tlen + SIZE_OP_JUMP + SIZE_OP_PUSH));
      }
    }
    else {
      r = add_opcode_rel_addr(reg, OP_JUMP, mod_tlen);
      if (r) return r;
      r = compile_tree_empty_check(qn->target, reg, qn->target_empty);
      if (r) return r;
      r = add_opcode_rel_addr(reg, OP_PUSH, -(mod_tlen + SIZE_OP_PUSH));
    }
  }
  else if (!infinite && qn->greedy &&
	   (tlen + SIZE_OP_PUSH) * qn->upper <= QUALIFIER_EXPAND_LIMIT_SIZE) {
    int n = qn->upper - qn->lower;

    r = compile_tree_n_times(qn->target, qn->lower, reg);
    if (r) return r;

    for (i = 0; i < n; i++) {
      r = add_opcode_rel_addr(reg, OP_PUSH,
			   (n - i) * tlen + (n - i - 1) * SIZE_OP_PUSH);
      if (r) return r;
      r = compile_tree(qn->target, reg);
      if (r) return r;
    }
  }
  else {
    r = compile_range_repeat_node(qn, mod_tlen, reg);
  }
  return r;
}

static int
compile_length_option(OptionNode* node, regex_t* reg)
{
  int tlen;
  RegOptionType prev = reg->options;

  reg->options = node->option;
  tlen = compile_length_tree(node->target, reg);
  reg->options = prev;

  if (tlen < 0) return tlen;

  return SIZE_OP_SET_OPTION_PUSH + SIZE_OP_SET_OPTION + SIZE_OP_FAIL
         + tlen + SIZE_OP_SET_OPTION;
}

static int
compile_option(OptionNode* node, regex_t* reg)
{
  int r;
  RegOptionType prev = reg->options;

  if (IS_IGNORECASE(node->option) && IS_NULL(reg->transtable))
    return REGERR_TABLE_FOR_IGNORE_CASE_IS_NOT_SETTED;

  r = add_opcode_option(reg, OP_SET_OPTION_PUSH, node->option);
  if (r) return r;
  r = add_opcode_option(reg, OP_SET_OPTION, prev);
  if (r) return r;
  r = add_opcode(reg, OP_FAIL);
  if (r) return r;

  reg->options = node->option;
  r = compile_tree(node->target, reg);
  reg->options = prev;
  if (r) return r;

  r = add_opcode_option(reg, OP_SET_OPTION, prev);
  return r;
}

static int
compile_length_effect_node(EffectNode* node, regex_t* reg)
{
  int len;
  int tlen = compile_length_tree(node->target, reg);

  if (tlen < 0) return tlen;

  switch (node->type) {
  case EFFECT_MEMORY:
    if (GET_MEM_STATS_AT(reg->mem_stats, node->regnum))
      len = SIZE_OP_MEMORY_START_PUSH + tlen + SIZE_OP_MEMORY_END_PUSH;
    else
      len = SIZE_OP_MEMORY_START_PUSH + tlen + SIZE_OP_MEMORY_END;
    break;
  case EFFECT_PREC_READ:
    len = SIZE_OP_PUSH_POS + tlen + SIZE_OP_POP_POS;
    break;
  case EFFECT_PREC_READ_NOT:
    len = SIZE_OP_PUSH_POS_NOT + tlen + SIZE_OP_FAIL_POS;
    break;
  case EFFECT_STOP_BACKTRACK:
    len = SIZE_OP_PUSH_STOP_BT + tlen + SIZE_OP_POP_STOP_BT;
    break;
  case EFFECT_LOOK_BEHIND:
    len = SIZE_OP_LOOK_BEHIND + tlen;
    break;
  case EFFECT_LOOK_BEHIND_NOT:
    len = SIZE_OP_PUSH_LOOK_BEHIND_NOT + tlen + SIZE_OP_FAIL_LOOK_BEHIND_NOT;
    break;
  }

  return len;
}

static int get_char_length_tree(Node* node, regex_t* reg, RegDistance* len);

static int
compile_effect_node(EffectNode* node, regex_t* reg)
{
  int r, len;

  switch (node->type) {
  case EFFECT_MEMORY:
    r = add_opcode(reg, OP_MEMORY_START_PUSH);
    if (r) return r;
    r = add_mem_num(reg, node->regnum);
    if (r) return r;
    r = compile_tree(node->target, reg);
    if (r) return r;

    if (GET_MEM_STATS_AT(reg->mem_stats, node->regnum))
      r = add_opcode(reg, OP_MEMORY_END_PUSH);
    else
      r = add_opcode(reg, OP_MEMORY_END);

    if (r) return r;
    r = add_mem_num(reg, node->regnum);
    break;

  case EFFECT_PREC_READ:
    r = add_opcode(reg, OP_PUSH_POS);
    if (r) return r;
    r = compile_tree(node->target, reg);
    if (r) return r;
    r = add_opcode(reg, OP_POP_POS);
    break;

  case EFFECT_PREC_READ_NOT:
    len = compile_length_tree(node->target, reg);
    if (len < 0) return len;
    r = add_opcode_rel_addr(reg, OP_PUSH_POS_NOT, len + SIZE_OP_FAIL_POS);
    if (r) return r;
    r = compile_tree(node->target, reg);
    if (r) return r;
    r = add_opcode(reg, OP_FAIL_POS);
    break;

  case EFFECT_STOP_BACKTRACK:
    r = add_opcode(reg, OP_PUSH_STOP_BT);
    if (r) return r;
    r = compile_tree(node->target, reg);
    if (r) return r;
    r = add_opcode(reg, OP_POP_STOP_BT);
    break;

  case EFFECT_LOOK_BEHIND:
    {
      RegDistance n;
      r = add_opcode(reg, OP_LOOK_BEHIND);
      if (r) return r;
      r = get_char_length_tree(node->target, reg, &n);
      if (r) return REGERR_INVALID_LOOK_BEHIND_PATTERN;
      r = add_length(reg, (int )n);
      if (r) return r;
      r = compile_tree(node->target, reg);
    }
    break;

  case EFFECT_LOOK_BEHIND_NOT:
    {
      RegDistance n;
      len = compile_length_tree(node->target, reg);
      r = add_opcode_rel_addr(reg, OP_PUSH_LOOK_BEHIND_NOT,
			   len + SIZE_OP_FAIL_LOOK_BEHIND_NOT);
      if (r) return r;
      r = get_char_length_tree(node->target, reg, &n);
      if (r) return REGERR_INVALID_LOOK_BEHIND_PATTERN;
      r = add_length(reg, (int )n);
      if (r) return r;
      r = compile_tree(node->target, reg);
      if (r) return r;
      r = add_opcode(reg, OP_FAIL_LOOK_BEHIND_NOT);
    }
    break;
  }

  return r;
}

static int
compile_length_tree(Node* node, regex_t* reg)
{
  int len, type, r;

  type = NTYPE(node);
  switch (type) {
  case N_EMPTY:
    r = 0;
    break;

  case N_LIST:
    len = 0;
    do {
      r = compile_length_tree(NCONS(node).left, reg);
      if (r < 0) return r;
      len += r;
    } while (IS_NOT_NULL(node = NCONS(node).right));
    r = len;
    break;

  case N_ALT:
    {
      int n;

      n = r = 0;
      do {
	r += compile_length_tree(NCONS(node).left, reg);
	n++;
      } while (IS_NOT_NULL(node = NCONS(node).right));
      r += (SIZE_OP_PUSH + SIZE_OP_JUMP) * (n - 1);
    }
    break;

  case N_STRING:
    r = compile_length_string_node(&(NSTRING(node)), reg);
    break;

  case N_STRING_RAW:
    r = compile_length_string_raw_node(&(NSTRING(node)), reg);
    break;

  case N_CCLASS:
    r = compile_length_cclass_node(&(NCCLASS(node)), reg);
    break;

  case N_CTYPE:
    r = SIZE_OPCODE;
    break;

  case N_ANYCHAR:
    r = SIZE_OPCODE;
    break;

  case N_ANCHOR:
    r = SIZE_OPCODE;
    break;

  case N_BACKREF:
    {
      int n = NBACKREF(node).regnum;
      if (n <= 3)
	r = SIZE_OPCODE;
      else
	r = SIZE_OPCODE + SIZE_MEMNUM;
    }
    break;

  case N_QUALIFIER:
    r = compile_length_qualifier_node(&(NQUALIFIER(node)), reg);
    break;

  case N_OPTION:
    r = compile_length_option(&(NOPTION(node)), reg);
    break;

  case N_EFFECT:
    r = compile_length_effect_node(&NEFFECT(node), reg);
    break;
  }

  return r;
}

static int
compile_tree(Node* node, regex_t* reg)
{
  int n, type, len, op, pos, r = 0;

  type = NTYPE(node);
  switch (type) {
  case N_EMPTY:
    r = 0;
    break;

  case N_LIST:
    do {
      r = compile_tree(NCONS(node).left, reg);
    } while (r == 0 && IS_NOT_NULL(node = NCONS(node).right));
    break;

  case N_ALT:
    {
      Node* x = node;
      len = 0;
      do {
	len += compile_length_tree(NCONS(x).left, reg);
	if (NCONS(x).right != NULL) {
	  len += SIZE_OP_PUSH + SIZE_OP_JUMP;
	}
      } while (IS_NOT_NULL(x = NCONS(x).right));
      pos = reg->used + len;  /* goal position */

      do {
	len = compile_length_tree(NCONS(node).left, reg);
	if (IS_NOT_NULL(NCONS(node).right)) {
	  r = add_opcode_rel_addr(reg, OP_PUSH, len + SIZE_OP_JUMP);
	  if (r) break;
	}
	r = compile_tree(NCONS(node).left, reg);
	if (r) break;
	if (IS_NOT_NULL(NCONS(node).right)) {
	  len = pos - (reg->used + SIZE_OP_JUMP);
	  r = add_opcode_rel_addr(reg, OP_JUMP, len);
	  if (r) break;
	}
      } while (IS_NOT_NULL(node = NCONS(node).right));
    }
    break;

  case N_STRING:
    r = compile_string_node(&(NSTRING(node)), reg);
    break;

  case N_STRING_RAW:
    r = compile_string_raw_node(&(NSTRING(node)), reg);
    break;

  case N_CCLASS:
    r = compile_cclass_node(&(NCCLASS(node)), reg);
    break;

  case N_CTYPE:
    switch (NCTYPE(node).type) {
    case CTYPE_WORD:            op = OP_WORD;           break;
    case CTYPE_NOT_WORD:        op = OP_NOT_WORD;       break;
    case CTYPE_WORD_BOUND:      op = OP_WORD_BOUND;     break;
    case CTYPE_NOT_WORD_BOUND:  op = OP_NOT_WORD_BOUND; break;
#ifdef USE_WORD_BEGIN_END
    case CTYPE_WORD_BEGIN:      op = OP_WORD_BEGIN;     break;
    case CTYPE_WORD_END:        op = OP_WORD_END;       break;
#endif
#ifdef USE_ONIGURUMA_EXTENSION
    case CTYPE_WORD_SB:         op = OP_WORD_SB;        break;
    case CTYPE_WORD_MB:         op = OP_WORD_MB;        break;
#endif
    }
    r = add_opcode(reg, op);
    break;

  case N_ANYCHAR:
    r = add_opcode(reg, OP_ANYCHAR);
    break;

  case N_ANCHOR:
    switch (NANCHOR(node).type) {
    case ANCHOR_BEGIN_BUF:      op = OP_BEGIN_BUF;      break;
    case ANCHOR_END_BUF:        op = OP_END_BUF;        break;
    case ANCHOR_BEGIN_LINE:     op = OP_BEGIN_LINE;     break;
    case ANCHOR_END_LINE:       op = OP_END_LINE;       break;
    case ANCHOR_SEMI_END_BUF:   op = OP_SEMI_END_BUF;   break;
    case ANCHOR_BEGIN_POSITION: op = OP_BEGIN_POSITION; break;
    }
    r = add_opcode(reg, op);
    break;

  case N_BACKREF:
    n = NBACKREF(node).regnum;
    switch (n) {
    case 1:  r = add_opcode(reg, OP_BACKREF1); break;
    case 2:  r = add_opcode(reg, OP_BACKREF2); break;
    case 3:  r = add_opcode(reg, OP_BACKREF3); break;
    default:
      add_opcode(reg, OP_BACKREFN);
      r = add_mem_num(reg, n);
      break;
    }
    break;

  case N_QUALIFIER:
    r = compile_qualifier_node(&(NQUALIFIER(node)), reg);
    break;

  case N_OPTION:
    r = compile_option(&(NOPTION(node)), reg);
    break;

  case N_EFFECT:
    r = compile_effect_node(&NEFFECT(node), reg);
    break;

  default:
#ifdef REG_DEBUG
    fprintf(stderr, "compile_tree: undefined node type %d\n", NTYPE(node));
#endif
    break;
  }

  return r;
}

static int
get_min_match_length(Node* node, RegDistance *min)
{
  RegDistance tmin;
  int r = 0;

  *min = 0;
  switch (NTYPE(node)) {
  case N_EMPTY:
  case N_ANCHOR:
  case N_BACKREF:
    break;

  case N_LIST:
    do {
      r = get_min_match_length(NCONS(node).left, &tmin);
      if (r == 0) *min += tmin;
    } while (r == 0 && IS_NOT_NULL(node = NCONS(node).right));
    break;

  case N_ALT:
    do {
      r = get_min_match_length(NCONS(node).left, &tmin);
      if (r == 0 && *min > tmin) *min = tmin;
    } while (r == 0 && IS_NOT_NULL(node = NCONS(node).right));
    break;

  case N_STRING:
  case N_STRING_RAW:
    {
      StrNode* sn = &(NSTRING(node));
      *min = sn->end - sn->s;
    }
    break;

  case N_CTYPE:
    switch (NCTYPE(node).type) {
    case CTYPE_WORD:     *min = 1; break;
    case CTYPE_NOT_WORD: *min = 1; break;
#ifdef USE_ONIGURUMA_EXTENSION
    case CTYPE_WORD_SB:  *min = 1; break;
    case CTYPE_WORD_MB:  *min = 2; break;
#endif
    default:
      break;
    }
    break;

  case N_CCLASS:
  case N_ANYCHAR:
    *min = 1;
    break;

  case N_QUALIFIER:
    {
      QualifierNode* qn = &(NQUALIFIER(node));

      if (qn->lower == 0) {
	break;
      }
      else {
	r = get_min_match_length(qn->target, min);
	if (r == 0) {
	  *min = distance_multiply(*min, qn->lower);
	}
      }
    }
    break;

  case N_OPTION:
    r = get_min_match_length(NOPTION(node).target, min);
    break;

  case N_EFFECT:
    {
      EffectNode* en = &(NEFFECT(node));
      switch (en->type) {
      case EFFECT_MEMORY:
      case EFFECT_STOP_BACKTRACK:
	r = get_min_match_length(en->target, min);
	break;

      case EFFECT_PREC_READ:
      case EFFECT_PREC_READ_NOT:
      case EFFECT_LOOK_BEHIND:
      case EFFECT_LOOK_BEHIND_NOT:
	break;
      }
    }
    break;

  default:
    break;
  }

  return r;
}

static int
get_max_match_length(Node* node, regex_t* reg, RegDistance *max)
{
  RegDistance tmax;
  int r = 0;

  *max = 0;
  switch (NTYPE(node)) {
  case N_EMPTY:
  case N_ANCHOR:
    break;

  case N_LIST:
    do {
      r = get_max_match_length(NCONS(node).left, reg, &tmax);
      if (r == 0)
	*max = distance_add(*max, tmax);
    } while (r == 0 && IS_NOT_NULL(node = NCONS(node).right));
    break;

  case N_ALT:
    do {
      r = get_max_match_length(NCONS(node).left, reg, &tmax);
      if (r == 0 && *max < tmax) *max = tmax;
    } while (r == 0 && IS_NOT_NULL(node = NCONS(node).right));
    break;

  case N_STRING:
  case N_STRING_RAW:
    {
      StrNode* sn = &(NSTRING(node));
      *max = sn->end - sn->s;
    }
    break;

  case N_CTYPE:
    switch (NCTYPE(node).type) {
    case CTYPE_WORD:
    case CTYPE_NOT_WORD:
#ifdef USE_ONIGURUMA_EXTENSION
    case CTYPE_WORD_MB:
#endif
      *max = mbmaxlen_dist(reg->code);
      break;

#ifdef USE_ONIGURUMA_EXTENSION
    case CTYPE_WORD_SB:
      *max = 1;
      break;
#endif

    default:
      break;
    }
    break;

  case N_CCLASS:
  case N_ANYCHAR:
    *max = mbmaxlen_dist(reg->code);
    break;

  case N_BACKREF:
    *max = INFINITE_DISTANCE;
    break;

  case N_QUALIFIER:
    {
      QualifierNode* qn = &(NQUALIFIER(node));

      if (qn->upper != 0) {
	r = get_max_match_length(qn->target, reg, max);
	if (r == 0 && *max != 0) {
	  if (! IS_REPEAT_INFINITE(qn->upper))
	    *max = distance_multiply(*max, qn->upper);
	  else
	    *max = INFINITE_DISTANCE;
	}
      }
    }
    break;

  case N_OPTION:
    r = get_max_match_length(NOPTION(node).target, reg, max);
    break;

  case N_EFFECT:
    {
      EffectNode* en = &(NEFFECT(node));
      switch (en->type) {
      case EFFECT_MEMORY:
      case EFFECT_STOP_BACKTRACK:
	r = get_max_match_length(en->target, reg, max);
	break;

      case EFFECT_PREC_READ:
      case EFFECT_PREC_READ_NOT:
      case EFFECT_LOOK_BEHIND:
      case EFFECT_LOOK_BEHIND_NOT:
	break;
      }
    }
    break;

  default:
    break;
  }

  return r;
}

/* fixed size pattern node only */
static int
get_char_length_tree(Node* node, regex_t* reg, RegDistance* len)
{
  RegDistance tlen;
  int r = 0;

  *len = 0;
  switch (NTYPE(node)) {
  case N_EMPTY:
  case N_ANCHOR:
    break;

  case N_LIST:
    do {
      r = get_char_length_tree(NCONS(node).left, reg, &tlen);
      if (r == 0)
	*len = distance_add(*len, tlen);
    } while (r == 0 && IS_NOT_NULL(node = NCONS(node).right));
    break;

  case N_STRING:
  case N_STRING_RAW:
    {
      StrNode* sn = &(NSTRING(node));
      UChar *s = sn->s;
      while (s < sn->end) {
	s += mblen(reg->code, *s);
	(*len)++;
      }
    }
    break;

  case N_CTYPE:
    switch (NCTYPE(node).type) {
    case CTYPE_WORD:
    case CTYPE_NOT_WORD:
#ifdef USE_ONIGURUMA_EXTENSION
    case CTYPE_WORD_SB:
    case CTYPE_WORD_MB:
#endif
      *len = 1;
      break;
    }
    break;

  case N_CCLASS:
  case N_ANYCHAR:
    *len = 1;
    break;

  case N_OPTION:
    r = get_char_length_tree(NOPTION(node).target, reg, len);
    break;

  case N_EFFECT:
    {
      EffectNode* en = &(NEFFECT(node));
      switch (en->type) {
      case EFFECT_MEMORY:
      case EFFECT_STOP_BACKTRACK:
	r = get_char_length_tree(en->target, reg, len);
	break;

      case EFFECT_PREC_READ:
      case EFFECT_PREC_READ_NOT:
      case EFFECT_LOOK_BEHIND:
      case EFFECT_LOOK_BEHIND_NOT:
	break;
      }
    }
    break;

  default:
    r = -1;
    break;
  }

  return r;
}

static Node*
get_head_exact_node(Node* node, regex_t* reg)
{
  Node* n = (Node* )NULL;

  switch (NTYPE(node)) {
  case N_EMPTY:
  case N_ANCHOR:
  case N_BACKREF:
  case N_ALT:
  case N_CTYPE:
  case N_CCLASS:
  case N_ANYCHAR:
    break;

  case N_LIST:
    n = get_head_exact_node(NCONS(node).left, reg);
    break;

  case N_STRING:
    if (IS_IGNORECASE(reg->options))
      break;
    /* fall */
  case N_STRING_RAW:
    {
      StrNode* sn = &(NSTRING(node));
      if (sn->end > sn->s)
	n = node;
    }
    break;

  case N_QUALIFIER:
    {
      QualifierNode* qn = &(NQUALIFIER(node));
      if (qn->lower > 0) {
	if (IS_NOT_NULL(qn->head_exact))
	  n = qn->head_exact;
	else
	  n = get_head_exact_node(qn->target, reg);
      }
    }
    break;

  case N_OPTION:
    {
      RegOptionType options = reg->options;

      reg->options = NOPTION(node).option;
      n = get_head_exact_node(NOPTION(node).target, reg);
      reg->options = options;
    }
    break;

  case N_EFFECT:
    {
      EffectNode* en = &(NEFFECT(node));
      switch (en->type) {
      case EFFECT_MEMORY:
      case EFFECT_STOP_BACKTRACK:
      case EFFECT_PREC_READ:
	n = get_head_exact_node(en->target, reg);
	break;

      case EFFECT_PREC_READ_NOT:
      case EFFECT_LOOK_BEHIND:
      case EFFECT_LOOK_BEHIND_NOT:
	break;
      }
    }
    break;

  default:
    break;
  }

  return n;
}

static int
check_type_tree(Node* node, int type_mask, int effect_mask, int anchor_mask)
{
  int type, r = 0;

  type = NTYPE(node);
  if ((type & type_mask) == 0)
    return 1;

  switch (type) {
  case N_LIST:
  case N_ALT:
    do {
      r = check_type_tree(NCONS(node).left, type_mask, effect_mask, anchor_mask);
    } while (r == 0 && IS_NOT_NULL(node = NCONS(node).right));
    break;

  case N_QUALIFIER:
    r = check_type_tree(NQUALIFIER(node).target, type_mask, effect_mask,
		      anchor_mask);
    break;
  case N_OPTION:
    r = check_type_tree(NOPTION(node).target, type_mask, effect_mask, anchor_mask);
    break;
  case N_EFFECT:
    {
      EffectNode* en = &(NEFFECT(node));
      if ((en->type & effect_mask) == 0)
	return 1;

      r = check_type_tree(en->target, type_mask, effect_mask, anchor_mask);
    }
    break;

  case N_ANCHOR:
    type = NANCHOR(node).type;
    if ((type & anchor_mask) == 0)
      return 1;
    break;

  default:
    break;
  }
  return r;
}

static void
next_setup(Node* node, Node* next_node, regex_t* reg)
{
#ifdef USE_QUALIFIER_PEEK_NEXT
  if (NTYPE(node) == N_QUALIFIER) {
    QualifierNode* qn = &(NQUALIFIER(node));
    if (qn->greedy && IS_REPEAT_INFINITE(qn->upper)) {
      qn->next_head_exact = get_head_exact_node(next_node, reg);
    }
  }
#endif
}


#define IN_ALT   (1<<0)
#define IN_NOT   (1<<1)

/* setup_tree does the following work.
 1. check empty loop. (set qn->target_empty)
 2. expand ignore-case in char class.
 3. set memory status bit flags. (reg->mem_stats)
 4. set qn->head_exact for [push, exact] -> [push_or_jump_exact1, exact].
 5. find invalid patterns in look-behind.
 6. expand repeated string.
 */
static int
setup_tree(Node* node, regex_t* reg, int state)
{
  int type;
  int r = 0;

  type = NTYPE(node);
  switch (type) {
  case N_EMPTY:
    break;

  case N_LIST:
    do {
      r = setup_tree(NCONS(node).left, reg, state);
      if (r == 0 && IS_NOT_NULL(NCONS(node).right))
	next_setup(NCONS(node).left, NCONS(NCONS(node).right).left, reg);
    } while (r == 0 && IS_NOT_NULL(node = NCONS(node).right));
    break;

  case N_ALT:
    do {
      r = setup_tree(NCONS(node).left, reg, (state | IN_ALT));
    } while (r == 0 && IS_NOT_NULL(node = NCONS(node).right));
    break;

  case N_CCLASS:
    if (IS_IGNORECASE(reg->options)) {
      int c, t;
      BitSetRef bs = NCCLASS(node).bs;
      for (c = 0; c < SINGLE_BYTE_SIZE; c++) {
	t = TTRANS(reg->transtable, c);
	if (t != c) {
	  if (BITSET_AT(bs, c)) BITSET_SET_BIT(bs, t);
	  if (BITSET_AT(bs, t)) BITSET_SET_BIT(bs, c);
	}
      }
    }
    break;

  case N_STRING:
  case N_STRING_RAW:
  case N_CTYPE:
  case N_ANYCHAR:
  case N_ANCHOR:
    break;

  case N_BACKREF:
    {
      int num = NBACKREF(node).regnum;

      MEM_STATS_ON_AT(reg->mem_stats, num);
    }
    break;

  case N_QUALIFIER:
    {
      RegDistance d;
      QualifierNode* qn = &(NQUALIFIER(node));
      Node* target = qn->target;

      if (IS_REPEAT_INFINITE(qn->upper) || qn->upper > 1) {
	r = get_min_match_length(target, &d);
	if (r) break;
	if (d == 0) qn->target_empty = 1;
      }
      r = setup_tree(target, reg, state);
      if (r) break;

      /* expand string */
#define EXPAND_STRING_MAX_LENGTH  100
      if (NTYPE(target) & N_STRING_MASK) {
	if (!IS_REPEAT_INFINITE(qn->lower) && qn->lower == qn->upper &&
	    qn->lower > 1 && qn->lower <= EXPAND_STRING_MAX_LENGTH) {
	  int len = NSTRING_LEN(target);
	  StrNode* sn = &(NSTRING(target));

	  if (len * qn->lower <= EXPAND_STRING_MAX_LENGTH) {
	    int i, n = qn->lower;
	    node_conv_to_str_node(node, target->type);
	    for (i = 0; i < n; i++) {
	      r = node_str_cat(node, sn->s, sn->end);
	      if (r) break;
	    }
	    node_free(target);
	    break; /* break case N_QUALIFIER: */
	  }
	}
      }

#ifdef USE_OP_PUSH_OR_JUMP_EXACT
      if (qn->greedy && !qn->target_empty) {
	StrNode* sn;
	qn->head_exact = get_head_exact_node(qn->target, reg);
	if (IS_NOT_NULL(qn->head_exact)) {
	  sn = &(NSTRING(qn->head_exact));
	  if (sn->s + 1 > sn->end)
	    qn->head_exact = (Node* )NULL;
	}
      }
#endif
    }
    break;

  case N_OPTION:
    {
      RegOptionType options = reg->options;

      reg->options = NOPTION(node).option;
      r = setup_tree(NOPTION(node).target, reg, state);
      reg->options = options;
    }
    break;

  case N_EFFECT:
    {
      EffectNode* en = &(NEFFECT(node));

      switch (en->type) {
      case EFFECT_MEMORY:
	if ((state & IN_ALT) || (state & IN_NOT))
	  MEM_STATS_ON_AT(reg->mem_stats, en->regnum);

	/* fall */
      case EFFECT_STOP_BACKTRACK:
      case EFFECT_PREC_READ:
	r = setup_tree(en->target, reg, state);
	break;
      case EFFECT_PREC_READ_NOT:
	r = setup_tree(en->target, reg, (state | IN_NOT));
	break;

/* allowed node types in look-behind */
#define ALLOWED_TYPE_IN_LB  \
  ( N_EMPTY | N_LIST | N_STRING | N_STRING_RAW | N_CCLASS | N_CTYPE | \
    N_ANYCHAR | N_ANCHOR | N_EFFECT )

#define ALLOWED_EFFECT_IN_LB       ( EFFECT_MEMORY | EFFECT_LOOK_BEHIND_NOT )
#define ALLOWED_EFFECT_IN_LB_NOT   ( EFFECT_LOOK_BEHIND )
#define ALLOWED_ANCHOR_IN_LB       \
( ANCHOR_BEGIN_LINE | ANCHOR_END_LINE | ANCHOR_BEGIN_BUF )
	/* can't allow all anchors, because \G in look-behind through Search().
	   ex. /(?<=\G)zz/.match("azz") => success. */

      case EFFECT_LOOK_BEHIND:
	r = check_type_tree(en->target, ALLOWED_TYPE_IN_LB,
			    ALLOWED_EFFECT_IN_LB, ALLOWED_ANCHOR_IN_LB);
	if (r < 0) return r;
	if (r > 0) return REGERR_INVALID_LOOK_BEHIND_PATTERN;
	r = setup_tree(en->target, reg, state);
	break;

      case EFFECT_LOOK_BEHIND_NOT:
	r = check_type_tree(en->target, ALLOWED_TYPE_IN_LB,
			    ALLOWED_EFFECT_IN_LB_NOT, ALLOWED_ANCHOR_IN_LB);
	if (r < 0) return r;
	if (r > 0) return REGERR_INVALID_LOOK_BEHIND_PATTERN;
	r = setup_tree(en->target, reg, (state | IN_NOT));
	break;
      }
    }
    break;

  default:
    break;
  }

  return r;
}


/* exact mode */
#define SO_CONTINUE   0
#define SO_END        1

#define MAX_OPT_EXACT_LEN       32
#define MAX_OPT_EXACT_LEN1      33

#define EXACT_BUF_FULL(ebuf)    ((ebuf)->len >= MAX_OPT_EXACT_LEN)
#define EXACT_BUF_VALID(ebuf)   ((ebuf)->len > 0)
#define MAP_BUF_VALID(mbuf)     ((mbuf)->n > 0)

typedef struct {
  RegDistance dmin;        /* min-distance from head */
  RegDistance dmax;        /* max-distance from head */
  int         pos_id;      /* position counter (for same position check) */
  int         anchor;

  RegCharEncoding   code;
  RegOptionType     options;
  RegTransTableType transtable;
  Node             *root;
} OptBuf;

typedef struct {
  RegDistance dmin;    /* min-distance from head */
  RegDistance dmax;    /* max-distance from head */
  int pos_id;          /* position counter (from OptBuf.pos) */
  int start_anchor;
  int end_anchor;
  int len;
  int ignore_case;
  int mode;
  UChar buf[MAX_OPT_EXACT_LEN1];
} ExactBuf;

typedef struct {
  RegDistance dmin;        /* min-distance from head */
  RegDistance dmax;        /* max-distance from head */
  int         pos_id;      /* position counter (from OptBuf.pos) */
  int         start_anchor;
  int         end_anchor;
  int         n;           /* map entry num or 0(invalid) */
  int         value;       /* weighted num value */
  UChar map[REG_CHAR_TABLE_SIZE];
} MapBuf;


static void
optbuf_init(OptBuf* obuf, regex_t* reg, Node* root)
{
  obuf->dmin = obuf->dmax = 0;
  obuf->pos_id     = 0;
  obuf->anchor     = 0;
  obuf->code       = reg->code;
  obuf->options    = reg->options;
  obuf->transtable = reg->transtable;
  obuf->root       = root;
}

static void
optbuf_copy(OptBuf* obuf, OptBuf* from)
{
  obuf->dmin       = from->dmin;
  obuf->dmax       = from->dmax;
  obuf->pos_id     = from->pos_id;
  obuf->anchor     = from->anchor;
  obuf->code       = from->code;
  obuf->options    = from->options;
  obuf->transtable = from->transtable;
  obuf->root       = from->root;
}

static void
optbuf_reset(OptBuf* obuf, RegDistance min, RegDistance max)
{
  if ((obuf->anchor & ANCHOR_BEGIN_BUF) == 0) {
    obuf->dmax = max;
    obuf->dmin = min;
  }
}

static void
optbuf_inc(OptBuf* obuf, RegDistance inc_min, RegDistance inc_max)
{
  obuf->dmax = distance_add(obuf->dmax, inc_max);
  obuf->dmin = distance_add(obuf->dmin, inc_min);

  obuf->pos_id++;
  obuf->anchor = 0;
}

static void
optbuf_merge_alt(OptBuf* obuf, OptBuf* from)
{
  if (obuf->dmin > from->dmin)
    obuf->dmin = from->dmin;

  if (obuf->dmax < from->dmax)
    obuf->dmax = from->dmax;

  if (obuf->pos_id < from->pos_id)
    obuf->pos_id = from->pos_id;

  obuf->pos_id++;
  obuf->anchor = 0;
}

#ifdef REG_DEBUG
static void
print_distance_range(FILE* f, RegDistance a, RegDistance b)
{
  if (a == INFINITE_DISTANCE)
    fputs("inf", f);
  else
    fprintf(f, "(%u)", a);

  fputs("-", f);

  if (b == INFINITE_DISTANCE)
    fputs("inf", f);
  else
    fprintf(f, "(%u)", b);
}

static void
print_anchor(FILE* f, int anchor)
{
  int q = 0;

  fprintf(f, "[");

  if (anchor & ANCHOR_BEGIN_BUF) {
    fprintf(f, "begin-buf");
    q = 1;
  }
  if (anchor & ANCHOR_BEGIN_LINE) {
    if (q) fprintf(f, ", ");
    q = 1;
    fprintf(f, "begin-line");
  }
  if (anchor & ANCHOR_BEGIN_POSITION) {
    if (q) fprintf(f, ", ");
    q = 1;
    fprintf(f, "begin-pos");
  }
  if (anchor & ANCHOR_END_BUF) {
    if (q) fprintf(f, ", ");
    q = 1;
    fprintf(f, "end-buf");
  }
  if (anchor & ANCHOR_SEMI_END_BUF) {
    if (q) fprintf(f, ", ");
    q = 1;
    fprintf(f, "semi-end-buf");
  }
  if (anchor & ANCHOR_END_LINE) {
    if (q) fprintf(f, ", ");
    q = 1;
    fprintf(f, "end-line");
  }
  if (anchor & ANCHOR_ANYCHAR_STAR) {
    if (q) fprintf(f, ", ");
    q = 1;
    fprintf(f, "anychar-star");
  }
  if (anchor & ANCHOR_ANYCHAR_STAR_PL) {
    if (q) fprintf(f, ", ");
    fprintf(f, "anychar-star-pl");
  }

  fprintf(f, "]");
}

static void
print_exactbuf(FILE* f, ExactBuf* buf)
{
  fprintf(f, "exact: ");

  print_distance_range(f, buf->dmin, buf->dmax);
  if (EXACT_BUF_VALID(buf))
    fprintf(f, ":%s ", buf->buf);
  else
    fprintf(f, ":!INVALID! ");

  if (buf->ignore_case) fprintf(f, "(ignore-case) ");
  print_anchor(f, buf->start_anchor);
  fprintf(f, ", ");
  print_anchor(f, buf->end_anchor);

  fprintf(f, "\n");
}
#endif


static void
exactbuf_init(ExactBuf* buf, OptBuf* obuf)
{
  buf->dmin   = obuf->dmin;
  buf->dmax   = obuf->dmax;
  buf->pos_id = obuf->pos_id;

  buf->start_anchor = obuf->anchor;
  buf->end_anchor   = 0;
  buf->buf[0]       = (UChar )NULL;
  buf->len          = 0;
  buf->ignore_case  = 0;
  buf->mode         = SO_CONTINUE;
}

static void
exactbuf_copy(ExactBuf* buf, ExactBuf* from)
{
  int i;

  buf->dmin         = from->dmin;
  buf->dmax         = from->dmax;
  buf->pos_id       = from->pos_id;
  buf->start_anchor = from->start_anchor;
  buf->end_anchor   = from->end_anchor;

  for (i = 0; i < from->len; i++)
    buf->buf[i] = from->buf[i];

  buf->buf[from->len] = (UChar )NULL;

  buf->len         = from->len;
  buf->ignore_case = from->ignore_case;
  buf->mode        = from->mode;
}

static void
exactbuf_add_str(ExactBuf* buf, UChar* s, UChar* end, RegCharEncoding code)
{
  int i, x, len;

  for (x = buf->len; s < end && x < MAX_OPT_EXACT_LEN; ) {
    len = mblen(code, *s);
    if (x + len > MAX_OPT_EXACT_LEN)
      break;

    for (i = 0; i < len; i++)
      buf->buf[x++] = *s++;
  }

  buf->len = x;
  buf->buf[buf->len] = (UChar )NULL;

  if (s < end || buf->len >= MAX_OPT_EXACT_LEN)
    buf->mode = SO_END;

  if (s < end)
    buf->end_anchor = 0;
}

static void
exactbuf_add(ExactBuf* buf, ExactBuf* from, RegCharEncoding code)
{
  exactbuf_add_str(buf, from->buf, &(from->buf[from->len]), code);
}

static void
exactbuf_intersect(ExactBuf* to, ExactBuf* from, RegCharEncoding code)
{
  int i, len, min;

  if (to->ignore_case != from->ignore_case) {
    to->start_anchor = 0;
    to->end_anchor   = 0;
    to->buf[0]       = (UChar )NULL;
    to->len          = 0;
    to->ignore_case  = 0;
    to->mode         = SO_CONTINUE;
    return ;
  }

  min = to->len;
  if (from->len < min) min = from->len;

  i = 0;
  while (1) {
    len = mblen(code, from->buf[i]);
    if (i + len > min)
      break;
    if (k_strncmp(from->buf + i, to->buf + i, len) != 0)
      break;

    i += len;
  }
  to->buf[i] = (UChar )NULL;
  to->len = i;
  to->start_anchor &= from->start_anchor;
  to->end_anchor   &= from->end_anchor;
}

#define EXACT_UPDATE_DMAX0_LEN_THRESHOLD   3
#define EXACT_IGNORE_CASE_POINT            3

static void
exactbuf_update_better(ExactBuf* to, ExactBuf* from)
{
  if (!EXACT_BUF_VALID(from))
    return ;

  if (!EXACT_BUF_VALID(to)) {
    goto update;
  }
  else if (to->dmax == INFINITE_DISTANCE) {
    if (from->dmax < INFINITE_DISTANCE || to->len < from->len)
      goto update;
  }
  else if (from->dmax < INFINITE_DISTANCE) {
    int z, to_len, from_len;
    RegDistance df, dt;

    df = from->dmax - from->dmin;
    dt = to->dmax   - to->dmin;

    if (from->ignore_case != to->ignore_case) {
      from_len = from->len + (from->ignore_case ? 0 : EXACT_IGNORE_CASE_POINT);
      to_len   = to->len   + (to->ignore_case   ? 0 : EXACT_IGNORE_CASE_POINT);
    }

    if (dt == 0) {
      if (to->dmax == 0 && to->len >= EXACT_UPDATE_DMAX0_LEN_THRESHOLD)
	return ;

      if (df == 0) {
	if (to_len < from_len)
	  goto update;
	if (to_len == from_len) {
	  if (to->start_anchor == 0 && from->start_anchor != 0)
	    goto update;
	}
      }
      return ;
    }
    else if (df == 0) {
      goto update;
    }

    df += from->dmax;
    dt += to->dmax;

    z = to_len * df;
    if (z < to_len) { /* overflow */
      if ((from_len / df) > (to_len / dt))
	goto update;
    }
    else {
      if (from_len * dt > z)
	goto update;
    }
  }

  return ;

 update:
  exactbuf_copy(to, from);
}

static void
exactbuf_revise(ExactBuf* best, ExactBuf* econt, OptBuf* obuf)
{
  if (econt->mode == SO_END) {
    exactbuf_update_better(best, econt);
    exactbuf_init(econt, obuf);
  }
}

static int set_exact_info(regex_t* reg, Node* node, OptBuf* obuf, 
			  ExactBuf* econt, ExactBuf* best);

static int
set_exact_info_node(regex_t* reg, OptBuf* obuf,
		    ExactBuf* econt, ExactBuf* best, Node* node)
{
  RegDistance len;
  int i, n, type;
  int r = 0;

  type = NTYPE(node);
  switch (type) {
  case N_EMPTY:
    return 0;
    break;

  case N_LIST:
    do {
      r = set_exact_info_node(reg, obuf, econt, best, NCONS(node).left);
      if (r == 0) exactbuf_revise(best, econt, obuf);
    } while (r == 0 && IS_NOT_NULL(node = NCONS(node).right));
    break;

  case N_ALT:
    {
      OptBuf zobuf, save;
      ExactBuf zebuf, mix, tbest;
      Node* x = node;

      optbuf_copy(&save, obuf);
      do {
	optbuf_copy(&zobuf,   &save);
	exactbuf_copy(&zebuf, econt);
	exactbuf_init(&tbest, &save);
	r = set_exact_info_node(reg, &zobuf, &zebuf, &tbest, NCONS(x).left);
	if (r == 0) {
	  if (EXACT_BUF_VALID(&tbest)) {
	    if (x == node)
	      exactbuf_copy(&mix, &tbest);
	    else
	      exactbuf_intersect(&mix, &tbest, save.code);
	  }
	  else {
	    if (x == node)
	      exactbuf_copy(&mix, &zebuf);
	    else
	      exactbuf_intersect(&mix, &zebuf, save.code);
	  }
	  optbuf_merge_alt(obuf, &zobuf);
	}
      } while ((r == 0) && IS_NOT_NULL(x = NCONS(x).right));

      if (r == 0) {
	if (mix.mode == SO_CONTINUE)
	  mix.mode = SO_END;

	exactbuf_copy(econt, &mix);
      }
    }
    break;

  case N_STRING:
    {
      UChar *p;
      StrNode* sn = &(NSTRING(node));
      int change_ignore_case = 0;

      if (sn->end <= sn->s)
	break;

      p = sn->end;
      if (econt->len > 0) {
	if (!econt->ignore_case && IS_IGNORECASE(obuf->options)) {
	  for (p = sn->s; p < sn->end; ) {
	    len = mblen(obuf->code, *p);
	    if (len == 1 && IS_AMBIGUITY_CHAR(obuf->transtable, *p)) {
	      change_ignore_case = 1;
	      break;
	    }
	    p += len;
	  }
	}
	else if (econt->ignore_case && !IS_IGNORECASE(obuf->options)) {
	  for (p = sn->s; p < sn->end; ) {
	    len = mblen(obuf->code, *p);
	    if (len == 1 && IS_AMBIGUITY_CHAR(obuf->transtable, *p)) {
	      change_ignore_case = 1;
	      break;
	    }
	    p += len;
	  }
	}
      }

      if (change_ignore_case) {
	if (p > sn->s)
	  exactbuf_add_str(econt, sn->s, p, obuf->code);
	econt->mode = SO_END;
	exactbuf_revise(best, econt, obuf);
	if (p < sn->end) {
	  exactbuf_add_str(econt, p, sn->end, obuf->code);
	  econt->ignore_case = IS_IGNORECASE(obuf->options);
	}
      }
      else {
	if (sn->s < p) {
	  int prev = (econt->len > 0);
	  exactbuf_add_str(econt, sn->s, p, obuf->code);
	  if (!prev)
	    econt->ignore_case = IS_IGNORECASE(obuf->options);
	}

	if (p < sn->end)
	  econt->mode = SO_END;
      }

      len = sn->end - sn->s;
      optbuf_inc(obuf, len, len);
    }
    break;

  case N_STRING_RAW:
    {
      UChar *p;
      StrNode* sn = &(NSTRING(node));
      int ambig = 0, mb = 0;

      if (sn->end <= sn->s)
	break;

      for (p = sn->s; p < sn->end; ) {
	len = mblen(obuf->code, *p);
	if (len == 1) {
	  if (IS_IGNORECASE(obuf->options) &&
	      IS_AMBIGUITY_CHAR(obuf->transtable, *p)) {
	    ambig = 1;
	    break;
	  }
	}
	else {
	  mb = 1;
	  break;
	}
	p += len;
      }

      if (mb == 0 && ambig == 0) {
	exactbuf_add_str(econt, sn->s, sn->end, obuf->code);
      }
      else
	econt->mode = SO_END;

      len = sn->end - sn->s;
      optbuf_inc(obuf, len, len);
    }
    break;

  case N_CCLASS:
    {
      int ignore_case;
      UChar tmp[1];
      CClassNode* cc = &(NCCLASS(node));

      if (IS_NULL(cc->mbuf) && cc->not == 0 &&
	  (n = bitset_on_num(cc->bs)) == 1) {
	for (i = 0; i < SINGLE_BYTE_SIZE; i++) {
	  if (BITSET_AT(cc->bs, i)) {
	    ignore_case = IS_IGNORECASE(obuf->options) &&
	                  !ismb(obuf->code, i) &&
	                  IS_AMBIGUITY_CHAR(obuf->transtable, i);
	    if ((ignore_case && econt->ignore_case) ||
		(!ignore_case && !econt->ignore_case)) {
	      tmp[0] = (UChar )i;
	      exactbuf_add_str(econt, tmp, tmp + 1, obuf->code);
	    }
	    else {
	      econt->mode = SO_END;
	    }
	    break;
	  }
	}
      }
      else {
	econt->mode = SO_END;
      }

      if (IS_NULL(cc->mbuf))
	len = 1;
      else
	len = mbmaxlen_dist(obuf->code);

      optbuf_inc(obuf, 1, len);
    }
    break;

  case N_CTYPE:
    switch (NCTYPE(node).type) {
    case CTYPE_WORD:
    case CTYPE_NOT_WORD:
#ifdef USE_ONIGURUMA_EXTENSION
    case CTYPE_WORD_SB:
    case CTYPE_WORD_MB:
#endif
      goto anychar;
      break;

    case CTYPE_WORD_BOUND:
    case CTYPE_NOT_WORD_BOUND:
#ifdef USE_WORD_BEGIN_END
    case CTYPE_WORD_BEGIN:
    case CTYPE_WORD_END:
#endif
      /* Nothing. */
      break;
    }
    break;

  case N_ANYCHAR:
  anychar:
    econt->mode = SO_END;
    len = mbmaxlen_dist(obuf->code);
    optbuf_inc(obuf, 1, len);
    break;

  case N_ANCHOR:
    econt->end_anchor |= NANCHOR(node).type;
    econt->mode = SO_END;

    switch (NANCHOR(node).type) {
    case ANCHOR_BEGIN_BUF:
      reg->anchor |= ANCHOR_BEGIN_BUF;
      optbuf_reset(obuf, 0, 0);
      break;

    case ANCHOR_BEGIN_POSITION:
      reg->anchor |= ANCHOR_BEGIN_POSITION;
      optbuf_reset(obuf, 0, 0);
      break;

    case ANCHOR_BEGIN_LINE:
      optbuf_reset(obuf, 0, INFINITE_DISTANCE);
      break;

    case ANCHOR_END_BUF:
      if (obuf->dmax != INFINITE_DISTANCE) {
	reg->anchor |= ANCHOR_END_BUF;
	reg->anchor_dmin = obuf->dmin;
	reg->anchor_dmax = obuf->dmax;
      }
      break;

    case ANCHOR_SEMI_END_BUF:
      if (obuf->dmax != INFINITE_DISTANCE &&
	  ((reg->anchor & ANCHOR_END_BUF) == 0)) {
	reg->anchor |= ANCHOR_SEMI_END_BUF;
	reg->anchor_dmin = obuf->dmin;
	reg->anchor_dmax = obuf->dmax;
      }
      break;

    case ANCHOR_END_LINE:
      break;
    }

    obuf->anchor |= NANCHOR(node).type;
    break;

  case N_BACKREF:
    econt->mode = SO_END;
    optbuf_inc(obuf, 0, INFINITE_DISTANCE);
    break;

  case N_QUALIFIER:
    {
      ExactBuf tcont, tbest;
      OptBuf tobuf;
      QualifierNode* qn = &(NQUALIFIER(node));

      if (qn->lower == 0 && IS_REPEAT_INFINITE(qn->upper)) {
	if (obuf->dmax == 0 && NTYPE(qn->target) == N_ANYCHAR && qn->greedy) {
	  if (IS_POSIXLINE(reg->options))
	    reg->anchor |= ANCHOR_ANYCHAR_STAR_PL;
	  else
	    reg->anchor |= ANCHOR_ANYCHAR_STAR;
	}

	econt->mode = SO_END;
	obuf->dmax  = INFINITE_DISTANCE;
      }
      else {
	optbuf_copy(&tobuf, obuf);      
	r = set_exact_info(reg, qn->target, &tobuf, &tcont, &tbest);
	if (r) break;

	if (EXACT_BUF_VALID(&tcont) && tcont.pos_id == obuf->pos_id) {
	  n = qn->lower;
	  if (n > 1 && tcont.mode == SO_END)
	    n = 1;

	  if (EXACT_BUF_VALID(econt)) {
	    if (econt->ignore_case != tcont.ignore_case) {
	      econt->mode = SO_END;
	      exactbuf_revise(best, econt, obuf);
	    }
	  }
	  else
	    econt->ignore_case = tcont.ignore_case;

	  for (i = 0; i < n; i++) {
	    exactbuf_add(econt, &tcont, obuf->code);
	    if (EXACT_BUF_FULL(econt)) {
	      econt->mode = SO_END;
	      break;
	    }
	  }

	  if (tcont.mode == SO_END || qn->lower != qn->upper)
	    econt->mode = SO_END;
	}
	else {
	  if (EXACT_BUF_VALID(&tbest) && tbest.pos_id == obuf->pos_id) {
	    n = qn->lower;
	    if (n > 1 && tbest.mode == SO_END)
	      n = 1;

	    if (EXACT_BUF_VALID(econt)) {
	      if (econt->ignore_case != tbest.ignore_case) {
		econt->mode = SO_END;
		exactbuf_revise(best, econt, obuf);
	      }
	    }
	    else
	      econt->ignore_case = tbest.ignore_case;

	    for (i = 0; i < n; i++) {
	      exactbuf_add(econt, &tbest, obuf->code);
	      if (EXACT_BUF_FULL(econt))
		break;
	    }
	  }
	  econt->mode = SO_END;
	}

	if (tobuf.dmax == INFINITE_DISTANCE) {
	  obuf->dmax = INFINITE_DISTANCE;
	}
	else {
	  if (IS_REPEAT_INFINITE(qn->upper))
	    obuf->dmax = INFINITE_DISTANCE;
	  else {
	    n = tobuf.dmax - obuf->dmax;
	    obuf->dmax = distance_add(obuf->dmax, (RegDistance)(n * qn->upper));
	  }
	}

	if (tobuf.dmin == INFINITE_DISTANCE) {
	  obuf->dmin = INFINITE_DISTANCE;
	}
	else {
	  n = tobuf.dmin - obuf->dmin;
	  obuf->dmin = distance_add(obuf->dmin, (RegDistance )(n * qn->lower));
	}
      }

      if (qn->upper > 0 || IS_REPEAT_INFINITE(qn->upper)) {
	optbuf_inc(obuf, 0, 0);
      }
    }
    break;

  case N_OPTION:
    {
      RegOptionType option = obuf->options;

      obuf->options = NOPTION(node).option;
      r = set_exact_info_node(reg, obuf, econt, best, NOPTION(node).target);
      obuf->options = option;
    }
    break;

  case N_EFFECT:
    {
      EffectNode* en = &(NEFFECT(node));

      switch (en->type) {
      case EFFECT_MEMORY:
	{
	  int save_anchor = reg->anchor;
	  r = set_exact_info_node(reg, obuf, econt, best, en->target);
	  if ((reg->anchor & ANCHOR_ANYCHAR_STAR_MASK) != 0 &&
	      (save_anchor & ANCHOR_ANYCHAR_STAR_MASK) == 0) {
	    if (GET_MEM_STATS_AT(reg->mem_stats, en->regnum))
	      reg->anchor &= ~ANCHOR_ANYCHAR_STAR_MASK;
	  }
	}
	break;

      case EFFECT_STOP_BACKTRACK:
	r = set_exact_info_node(reg, obuf, econt, best, en->target);
	break;

      case EFFECT_PREC_READ:
	if (NTYPE(en->target) == N_STRING) {
	  OptBuf save;

	  optbuf_copy(&save, obuf);
	  r = set_exact_info_node(reg, &save, econt, best, en->target);
	  econt->mode = SO_END;
	}
	break;

      case EFFECT_PREC_READ_NOT:
      case EFFECT_LOOK_BEHIND: /* Sorry, I can't make use of it. */
      case EFFECT_LOOK_BEHIND_NOT:
	/* Nothing. */
	break;
      }
    }
    break;

  default:
    r = REGERR_TYPE_BUG;
    break;
  }

  if (r == 0)
    exactbuf_revise(best, econt, obuf);
  return r;
}

static int
set_exact_info(regex_t* reg, Node* node, OptBuf* obuf,
	       ExactBuf* econt, ExactBuf* best)
{
  int r;

  exactbuf_init(best,  obuf);
  exactbuf_init(econt, obuf);
  r = set_exact_info_node(reg, obuf, econt, best, node);
  if (r == 0 && econt->mode == SO_END) {
    exactbuf_update_better(best, econt);
  }
  return r;
}

#ifdef REG_DEBUG
static void
print_mapbuf(FILE* f, MapBuf* buf)
{
  fprintf(f, "map: ");

  if (MAP_BUF_VALID(buf)) {
    print_distance_range(f, buf->dmin, buf->dmax);
    fprintf(f, ":%d:%d ", buf->n, buf->value);
    print_anchor(f, buf->start_anchor);
    fprintf(f, ", ");
    print_anchor(f, buf->end_anchor);
  }
  else {
    fprintf(f, "invalid");
  }
  fprintf(f, "\n\n");
}
#endif

static void
mapbuf_init(MapBuf* buf, OptBuf* obuf)
{
  int i;

  buf->dmin   = obuf->dmin;
  buf->dmax   = obuf->dmax;
  buf->pos_id = obuf->pos_id;
  buf->n      = 0;
  buf->value  = 0;
  buf->start_anchor = obuf->anchor;
  buf->end_anchor   = 0;

  for (i = 0; i < REG_CHAR_TABLE_SIZE; i++)
    buf->map[i] = (UChar )0;
}

static void
mapbuf_copy(MapBuf* buf, MapBuf* from)
{
  buf->dmin   = from->dmin;
  buf->dmax   = from->dmax;
  buf->pos_id = from->pos_id;
  buf->n      = from->n;
  buf->value  = from->value;
  buf->start_anchor = from->start_anchor;
  buf->end_anchor   = from->end_anchor;

  xmemcpy(buf->map, from->map, sizeof(UChar) * REG_CHAR_TABLE_SIZE);
}

static void
map_add(MapBuf* buf, UChar c)
{
  switch(c) {
  case ' ':  buf->value += 8; break;
  case '\t': buf->value += 4; break;
  case '\n': buf->value += 7; break;
  default:   buf->value += 1; break;
  }

  buf->map[c] = 1;
  buf->n++;
}

static void
mapbuf_add(MapBuf* buf, UChar c)
{
  if (buf->map[c] == 0) {
    map_add(buf, c);
  }
}

static void
mapbuf_reverse(MapBuf* buf)
{
  int i;

  buf->value = buf->n = 0;
  for (i = 0; i < REG_CHAR_TABLE_SIZE; i++) {
    if (buf->map[i])
      buf->map[i] = 0;
    else
      map_add(buf, i);
  }
}

static void
mapbuf_clear(MapBuf* buf)
{
  buf->n            = 0;
  buf->value        = 0;
  buf->start_anchor = 0;
  buf->end_anchor   = 0;
}

static void
mapbuf_union(MapBuf* to, MapBuf* from)
{
  int i;

  if (! MAP_BUF_VALID(to)) return ;
  if (! MAP_BUF_VALID(from)) {
    mapbuf_clear(to);
    return ;
  }

  if (to->pos_id != from->pos_id) {
    mapbuf_clear(to);
    return ;
  }

  for (i = 0; i < REG_CHAR_TABLE_SIZE; i++) {
    if (from->map[i]) {
      if (to->map[i] == 0)
	map_add(to, i);
    }
  }

  to->start_anchor &= from->start_anchor;
  to->end_anchor   &= from->end_anchor;
}

#define MAP_UPDATE_DMAX0_VALUE_THRESHOLD   16
#define MAP_VALUE_BASE                      4

static void
mapbuf_update_better(MapBuf* to, MapBuf* from)
{
  if (! MAP_BUF_VALID(from)) return ;

  if (! MAP_BUF_VALID(to)) {
    goto update;
  }
  if (to->dmax == INFINITE_DISTANCE) {
    if (from->dmax < INFINITE_DISTANCE || to->value > from->value)
      goto update;
  }
  else if (from->dmax < INFINITE_DISTANCE) {
    RegDistance df, dt;

    df = from->dmax - from->dmin;
    dt = to->dmax   - to->dmin;

    if (dt == 0) {
      if (to->dmax == 0 && to->value < MAP_UPDATE_DMAX0_VALUE_THRESHOLD)
	return ;

      if (df == 0) {
	if (to->value > from->value) goto update;
	if ((to->start_anchor == 0) && (from->start_anchor != 0)) goto update;
	return ;
      }
    }
    else if (df == 0) {
      goto update;
    }

    if (((double )(from->value + MAP_VALUE_BASE) * (double )(df + from->dmax))
      < ((double )(to->value   + MAP_VALUE_BASE) * (double )(dt + to->dmax)))
      goto update;

    if ((to->start_anchor == 0) && (from->start_anchor != 0) &&
	(df <= dt * 2)) goto update;
  }

  return ;

 update:
  mapbuf_copy(to, from);
}

static int set_map_info(Node* node, OptBuf* obuf, MapBuf* best);

static int
set_map_info_node(OptBuf* obuf, MapBuf* best, Node* node)
{
  MapBuf curr;
  RegDistance len;
  UChar c;
  int i, n, type;
  int r = 0;

  type = NTYPE(node);
  switch (type) {
  case N_EMPTY:
    return 0;
    break;

  case N_LIST:
    do {
      r = set_map_info_node(obuf, best, NCONS(node).left);
    } while (r == 0 && IS_NOT_NULL(node = NCONS(node).right));
    break;

  case N_ALT:
    {
      OptBuf zobuf, save;
      MapBuf zbuf;
      Node* x = node;

      optbuf_copy(&save, obuf);
      mapbuf_init(&curr, obuf);
      do {
	optbuf_copy(&zobuf, &save);
	if (x == node) {
	  r = set_map_info_node(&zobuf, &curr, NCONS(x).left);
	}
	else {
	  mapbuf_init(&zbuf, &save);
	  r = set_map_info_node(&zobuf, &zbuf, NCONS(x).left);
	  if (r == 0) {
	    mapbuf_union(&curr, &zbuf);
	  }
	}
	optbuf_merge_alt(obuf, &zobuf);
      } while ((r == 0) && IS_NOT_NULL(x = NCONS(x).right));

      if (r == 0)
	mapbuf_update_better(best, &curr);
    }
    break;

  case N_STRING:
    {
      UChar c;
      StrNode* sn = &(NSTRING(node));

      if (sn->end <= sn->s)
	break;

      mapbuf_init(&curr, obuf);
      c = *(sn->s);
      if (IS_IGNORECASE(obuf->options) && !ismb(obuf->code, c) &&
	  IS_AMBIGUITY_CHAR(obuf->transtable, c)) {
	  for (i = 0; i < REG_CHAR_TABLE_SIZE; i++) {
	    if (TTRANS(obuf->transtable, i) == c)
	      mapbuf_add(&curr, (UChar )i);
	  }
      }
      else {
	mapbuf_add(&curr, c);
      }

      mapbuf_update_better(best, &curr);
      len = sn->end - sn->s;
      optbuf_inc(obuf, len, len);
    }
    break;

  case N_STRING_RAW:
    {
      StrNode* sn = &(NSTRING(node));

      if (sn->end <= sn->s)
	break;

      mapbuf_init(&curr, obuf);
      c = *(sn->s);
      mapbuf_add(&curr, c);
      mapbuf_update_better(best, &curr);
      len = sn->end - sn->s;
      optbuf_inc(obuf, len, len);
    }
    break;

  case N_CCLASS:
    {
      CClassNode* cc = &(NCCLASS(node));

      mapbuf_init(&curr, obuf);
      for (i = 0; i < SINGLE_BYTE_SIZE; i++) {
	if (BITSET_AT(cc->bs, i)) {
	  mapbuf_add(&curr, (UChar )i);
	  if (IS_IGNORECASE(obuf->options)) {
	    mapbuf_add(&curr, (UChar )TTRANS(obuf->transtable, i));
	  }
	}
      }

      if (IS_NOT_NULL(cc->mbuf)) {
	for (i = 0; i < SINGLE_BYTE_SIZE; i++) {
	  if (BITSET_AT((BitSetRef )(cc->mbuf->p), i)) {
	    mapbuf_add(&curr, (UChar )i);
	  }
	}
      }

      if (cc->not) mapbuf_reverse(&curr);
      mapbuf_update_better(best, &curr);

      if (IS_NULL(cc->mbuf))
	len = 1;
      else
	len = mbmaxlen_dist(obuf->code);

      optbuf_inc(obuf, 1, len);
    }
    break;

  case N_CTYPE:
    switch (NCTYPE(node).type) {
    case CTYPE_WORD:
    case CTYPE_NOT_WORD:
      mapbuf_init(&curr, obuf);
      for (i = 0; i < SINGLE_BYTE_SIZE; i++) {
	c = (UChar )i;
	if (IS_WORD_HEAD(obuf->code, c))
	  mapbuf_add(&curr, c);
      }

      if (NCTYPE(node).type == CTYPE_NOT_WORD)
	mapbuf_reverse(&curr);

      mapbuf_update_better(best, &curr);
      goto anychar;
      break;

#ifdef USE_ONIGURUMA_EXTENSION
    case CTYPE_WORD_SB:
      mapbuf_init(&curr, obuf);
      for (i = 0; i < SINGLE_BYTE_SIZE; i++) {
	c = (UChar )i;
	if (IS_SB_WORD(obuf->code, c))
	  mapbuf_add(&curr, c);
      }
      mapbuf_update_better(best, &curr);
      optbuf_inc(obuf, 1, 1);
      break;

    case CTYPE_WORD_MB:
      mapbuf_init(&curr, obuf);
      for (i = 0; i < SINGLE_BYTE_SIZE; i++) {
	c = (UChar )i;
	if (IS_MB_WORD(obuf->code, c))
	  mapbuf_add(&curr, c);
      }
      mapbuf_update_better(best, &curr);
      optbuf_inc(obuf, 1, mbmaxlen_dist(obuf->code));
      break;
#endif

    case CTYPE_WORD_BOUND:
    case CTYPE_NOT_WORD_BOUND:
#ifdef USE_WORD_BEGIN_END
    case CTYPE_WORD_BEGIN:
    case CTYPE_WORD_END:
#endif
      /* Nothing. */
      break;
    }
    break;

  case N_ANYCHAR:
  anychar:
    len = mbmaxlen_dist(obuf->code);
    optbuf_inc(obuf, 1, len);
    break;

  case N_ANCHOR:
    if (best->pos_id == obuf->pos_id - 1)
      best->end_anchor |= NANCHOR(node).type;

    switch (NANCHOR(node).type) {
    case ANCHOR_BEGIN_BUF:
      optbuf_reset(obuf, 0, 0);
      break;
    case ANCHOR_BEGIN_POSITION:
      optbuf_reset(obuf, 0, 0);
      break;
    case ANCHOR_BEGIN_LINE:
      optbuf_reset(obuf, 0, INFINITE_DISTANCE);
      break;

    case ANCHOR_END_LINE:
    case ANCHOR_END_BUF:
    case ANCHOR_SEMI_END_BUF:
      break;
    }

    obuf->anchor |= NANCHOR(node).type;
    break;

  case N_BACKREF:
    optbuf_inc(obuf, 0, INFINITE_DISTANCE);
    break;

  case N_QUALIFIER:
    {
      OptBuf tobuf;
      QualifierNode* qn = &(NQUALIFIER(node));

      if (IS_REPEAT_INFINITE(qn->upper) || qn->upper > 0) {
	optbuf_copy(&tobuf, obuf);
	if (qn->lower > 0) {
	  r = set_map_info(qn->target, &tobuf, best);
	}
	else {
	  MapBuf tbest;
	  /* qn->lower == 0: tbest is not used. */
	  r = set_map_info(qn->target, &tobuf, &tbest);
	}
	if (r) break;

	if (tobuf.dmax == INFINITE_DISTANCE) {
	  obuf->dmax = INFINITE_DISTANCE;
	}
	else {
	  if (IS_REPEAT_INFINITE(qn->upper))
	    obuf->dmax = INFINITE_DISTANCE;
	  else {
	    n = tobuf.dmax - obuf->dmax;
	    obuf->dmax = distance_add(obuf->dmax, (RegDistance)(n * qn->upper));
	  }
	}

	if (tobuf.dmin == INFINITE_DISTANCE) {
	  obuf->dmin = INFINITE_DISTANCE;
	}
	else {
	  n = tobuf.dmin - obuf->dmin;
	  obuf->dmin = distance_add(obuf->dmin, (RegDistance)(n * qn->lower));
	}

	optbuf_inc(obuf, 0, 0);
      }
    }
    break;

  case N_OPTION:
    {
      RegOptionType option = obuf->options;

      obuf->options = NOPTION(node).option;
      r = set_map_info_node(obuf, best, NOPTION(node).target);
      obuf->options = option;
    }
    break;

  case N_EFFECT:
    {
      EffectNode* en = &(NEFFECT(node));

      switch (en->type) {
      case EFFECT_MEMORY:
      case EFFECT_STOP_BACKTRACK:
	r = set_map_info_node(obuf, best, en->target);
	break;

      case EFFECT_PREC_READ:
	if (NTYPE(en->target) == N_STRING ||
	    NTYPE(en->target) == N_STRING_RAW) {
	  OptBuf save;

	  optbuf_copy(&save, obuf);
	  r = set_map_info_node(&save, best, en->target);
	}
	break;

      case EFFECT_PREC_READ_NOT:
      case EFFECT_LOOK_BEHIND:
      case EFFECT_LOOK_BEHIND_NOT:
	/* Nothing. */
	break;
      }
    }
    break;

  default:
    r = REGERR_TYPE_BUG;
    break;
  }

  return r;
}

static int
set_map_info(Node* node, OptBuf* obuf, MapBuf* best)
{
  mapbuf_init(best, obuf);
  return set_map_info_node(obuf, best, node);
}

#ifdef REG_DEBUG
static void
print_optimize_info(FILE* f, regex_t* reg)
{
  static char* on[] = { "NONE", "EXACT", "EXACT_BM", "EXACT_IC",
			"EXACT_BM_IC(not used!)", "MAP" };

  fprintf(f, "optimize: %s\n", on[reg->optimize]);
  fprintf(f, "  anchor: "); print_anchor(f, reg->anchor);
  if ((reg->anchor & ANCHOR_END_BUF_MASK) != 0)
    print_distance_range(f, reg->anchor_dmin, reg->anchor_dmax);
  fprintf(f, "\n");

  if (reg->optimize) {
    fprintf(f, "  sub anchor: "); print_anchor(f, reg->sub_anchor);
    fprintf(f, "\n");
  }
  fprintf(f, "\n");
}
#endif

static void
set_optimize_map(regex_t* reg, MapBuf* m)
{
  int i;

  for (i = 0; i < REG_CHAR_TABLE_SIZE; i++)
    reg->map[i] = m->map[i];

  reg->optimize   = REG_OPTIMIZE_MAP;
  reg->sub_anchor = m->start_anchor;
  reg->dmin       = m->dmin;
  reg->dmax       = m->dmax;

  if (reg->dmax != INFINITE_DISTANCE) {
    reg->threshold_len = reg->dmax + 1;
    if (reg->threshold_len < POSITION_SEARCH_MIN_THRESHOLD_LENGTH)
      reg->threshold_len = POSITION_SEARCH_MIN_THRESHOLD_LENGTH;
  }
}

static int set_bm_skip(UChar* s, UChar* end, UChar* transtable,
		     UChar skip[], int** int_skip);

static int
set_optimize_exact(regex_t* reg, ExactBuf* e)
{
  int r;

  if (EXACT_BUF_VALID(e)) {
    reg->exact = k_strdup(e->buf, e->buf + e->len);
    if (IS_NULL(reg->exact)) return REGERR_MEMORY;
 
    reg->exact_end = reg->exact + e->len;
    if (e->ignore_case) {
      UChar *p;
      int len;
      for (p = reg->exact; p < reg->exact_end; ) {
	len = mblen(reg->code, *p);
	if (len == 1) *p = TTRANS(reg->transtable, *p);
	p += len;
      }
    }

    if (!e->ignore_case &&
	(e->len >= 3 || (e->len >= 2 && IS_INDEPENDENT_TRAIL(reg->code)))) {
      r = set_bm_skip(reg->exact, reg->exact_end,
		      (e->ignore_case ? reg->transtable : (UChar* )NULL),
		      reg->map, &(reg->int_map));
      if (r) return r;
      reg->optimize = REG_OPTIMIZE_EXACT_BM;
    }
    else
      reg->optimize = (e->ignore_case ?
		       REG_OPTIMIZE_EXACT_IC : REG_OPTIMIZE_EXACT);

    reg->dmin = e->dmin;
    reg->dmax = e->dmax;
    reg->sub_anchor = e->start_anchor;

    if (reg->dmax != INFINITE_DISTANCE) {
      reg->threshold_len = reg->dmax + (reg->exact_end - reg->exact);
      if (reg->threshold_len < POSITION_SEARCH_MIN_THRESHOLD_LENGTH)
	reg->threshold_len = POSITION_SEARCH_MIN_THRESHOLD_LENGTH;
    }
  }

  return 0;
}

static void
clear_optimize_info(regex_t* reg)
{
  reg->optimize      = REG_OPTIMIZE_NONE;
  reg->anchor        = 0;
  reg->sub_anchor    = 0;
  reg->exact_end     = (UChar* )NULL;
  reg->threshold_len = 0;
  if (IS_NOT_NULL(reg->exact)) {
    xfree(reg->exact);
    reg->exact = (UChar* )NULL;
  }
}

#define EXACT_LEN_FOR_MAP_EVAL_THRESHOLD    2
#define EXACT_DMAX_FOR_MAP_EVAL_THRESHOLD   4
#define MAP_VALUE_THRESHOLD                 4
#define MAP_VALUE_THRESHOLD2                9

static int
set_optimize_info_from_tree(Node* node, regex_t* reg)
{
  int r;
  OptBuf   obuf;
  ExactBuf ebest, econt;

  optbuf_init(&obuf, reg, node);
  r = set_exact_info(reg, node, &obuf, &econt, &ebest);
  if (r) return r;
  if (econt.mode != SO_END) {
    econt.mode = SO_END;
    exactbuf_update_better(&ebest, &econt);
  }

#ifdef REG_DEBUG_COMPILE
  fprintf(stderr, "best "); print_exactbuf(stderr, &ebest);
#endif

  if (!EXACT_BUF_VALID(&ebest) ||
      (ebest.len < EXACT_LEN_FOR_MAP_EVAL_THRESHOLD && ebest.dmax != 0) ||
      ebest.dmax >= EXACT_DMAX_FOR_MAP_EVAL_THRESHOLD) {
    MapBuf mbest;

    optbuf_init(&obuf, reg, node);
    r = set_map_info(node, &obuf, &mbest);
    if (r) return r;

#ifdef REG_DEBUG_COMPILE
    fprintf(stderr, "best "); print_mapbuf(stderr, &mbest);
#endif

    if (MAP_BUF_VALID(&mbest)) {
      if (!EXACT_BUF_VALID(&ebest) ||
	  (mbest.value < MAP_VALUE_THRESHOLD &&
	   distance_multiply(mbest.dmax, 2) < ebest.dmax) ||
	  (mbest.value < MAP_VALUE_THRESHOLD2 &&
	   distance_multiply(mbest.dmax, 3) < ebest.dmax)) {
	set_optimize_map(reg, &mbest);
      }
    }
  }

  if (reg->optimize == 0 && EXACT_BUF_VALID(&ebest)) {
    r = set_optimize_exact(reg, &ebest);
    if (r) return r;
  }

  /* only begin-line, end-line used. */
  reg->sub_anchor &= (ANCHOR_BEGIN_LINE | ANCHOR_END_LINE);

#if defined(REG_DEBUG_COMPILE) || defined(REG_DEBUG_MATCH)
  print_optimize_info(stderr, reg);
#endif
  return r;
}


extern void
regex_region_clear(RegRegion* region)
{
  int i;

  for (i = 0; i < region->num_regs; i++) {
    region->beg[i] = region->end[i] = -1;
  }
}

extern int
regex_region_resize(RegRegion* region, int n)
{
  int i;

  region->num_regs = n;

  if (n < REG_NREGION)
    n = REG_NREGION;

  if (region->allocated == 0) {
    region->beg = (int* )xmalloc(n * sizeof(int));
    region->end = (int* )xmalloc(n * sizeof(int));

    if (region->beg == 0 || region->end == 0)
      return REGERR_MEMORY;

    region->allocated = n;
  }
  else if (region->allocated < n) {
    region->beg = (int* )xrealloc(region->beg, n * sizeof(int));
    region->end = (int* )xrealloc(region->end, n * sizeof(int));

    if (region->beg == 0 || region->end == 0)
      return REGERR_MEMORY;

    region->allocated = n;
  }

  for (i = 0; i < region->num_regs; i++) {
    region->beg[i] = region->end[i] = -1;
  }
  return 0;
}

static void
regex_region_init(RegRegion* region)
{
  region->num_regs  = 0;
  region->allocated = 0;
  region->beg       = (int* )0;
  region->end       = (int* )0;
}

extern RegRegion*
regex_region_new()
{
  RegRegion* r;

  r = (RegRegion* )xmalloc(sizeof(RegRegion));
  regex_region_init(r);
  return r;
}

extern void
regex_region_free(RegRegion* r, int free_self)
{
  if (r) {
    if (r->allocated > 0) {
      if (r->beg) xfree(r->beg);
      if (r->end) xfree(r->end);
      r->allocated = 0;
    }
    if (free_self) xfree(r);
  }
}

extern void
re_free_registers(RegRegion* r)
{
  /* 0: don't free self */
  regex_region_free(r, 0);
}

extern void
regex_region_copy(RegRegion* r1, RegRegion* r2)
{
#define RREGC_SIZE   (sizeof(int) * r2->num_regs)
  int i;

  if (r1 == r2) return;

  if (r1->allocated == 0) {
    r1->beg = (int* )xmalloc(RREGC_SIZE);
    r1->end = (int* )xmalloc(RREGC_SIZE);
    r1->allocated = r2->num_regs;
  }
  else if (r1->allocated < r2->num_regs) {
    r1->beg = (int* )xrealloc(r1->beg, RREGC_SIZE);
    r1->end = (int* )xrealloc(r1->end, RREGC_SIZE);
    r1->allocated = r2->num_regs;
  }

  for (i = 0; i < r2->num_regs; i++) {
    r1->beg[i] = r2->beg[i];
    r1->end[i] = r2->end[i];
  }
  r1->num_regs = r2->num_regs;
}


/** stack data **/

/* stack type */
  /* used by normal-POP */
#define STK_ALT               1
#define STK_LOOK_BEHIND_NOT   2
#define STK_POS_NOT           3
  /* handled by normal-POP */
#define STK_MEM_START         4
#define STK_MEM_END           5
#define STK_REPEAT_INC        6
  /* avoided by normal-POP */
#define STK_POS               7   /* used when POP-POS */
#define STK_STOP_BT           8   /* mark for stop-backtrack "(?>...)" */
#define STK_REPEAT            9
#define STK_NULL_CHECK_START 10
#define STK_VOID             11   /* for fill a blank */


typedef struct {
  void* p;
  int   n;
} MatchStackAlloc;

#define MATCH_STACK_ALLOC_INIT(msa) do {\
  (msa).p = (void* )0;\
} while (0)

#define MATCH_STACK_ALLOC_FREE(msa)   if ((msa).p) xfree((msa).p)


#define STACK_INIT(alloc_addr, ptr_num, stack_num)  do {\
  if (msa->p) {\
    alloc_addr = (char* )xalloca(sizeof(char*) * (ptr_num));\
    stk_alloc  = (StackType* )(msa->p);\
    stk_base   = stk_alloc;\
    stk        = stk_base;\
    stk_end    = stk_base + msa->n;\
  }\
  else {\
    alloc_addr = (char* )xalloca(sizeof(char*) * (ptr_num)\
		       + sizeof(StackType) * (stack_num));\
    stk_alloc  = (StackType* )(alloc_addr + sizeof(char*) * (ptr_num));\
    stk_base   = stk_alloc;\
    stk        = stk_base;\
    stk_end    = stk_base + (stack_num);\
  }\
} while(0)

#define STACK_SAVE do{\
  if (stk_base != stk_alloc) {\
    msa->p = stk_base;\
    msa->n = stk_end - stk_base;\
  };\
} while(0)

#define STACK_ENSURE(n)	do {\
  if (stk_end - stk <= (n)) {\
    STACK_DOUBLE;\
  }\
} while(0)

#define STACK_DOUBLE do {\
  int n = stk_end - stk_base;\
  StackType* x;\
  if (stk_base == stk_alloc && IS_NULL(msa->p)) {\
    x = (StackType* )xmalloc(sizeof(StackType) * n * 2);\
    if (IS_NULL(x)) {\
      STACK_SAVE;\
      return REGERR_MEMORY;\
    }\
    xmemcpy(x, stk_base, n * sizeof(StackType));\
  }\
  else {\
    x = (StackType* )xrealloc(stk_base, sizeof(StackType) * n * 2);\
    if (IS_NULL(x)) {\
      STACK_SAVE;\
      return REGERR_MEMORY;\
    }\
  }\
  stk      = x + (stk - stk_base);\
  stk_base = x;\
  stk_end  = stk_base + n * 2;\
} while (0)

#define STACK_AT(index)        (stk_base + (index))
#define GET_STACK_INDEX(stk)   ((stk) - stk_base)

#define STACK_PUSH(stack_type,pat,s,sprev) do {\
  STACK_ENSURE(1);\
  stk->type = (stack_type);\
  stk->u.state.pcode     = (pat);\
  stk->u.state.pstr      = (s);\
  stk->u.state.pstr_prev = (sprev);\
  STACK_INC;\
} while(0)

#define STACK_PUSH_TYPE(stack_type) do {\
  STACK_ENSURE(1);\
  stk->type = (stack_type);\
  STACK_INC;\
} while(0)

#define STACK_PUSH_ALT(pat,s,sprev)     STACK_PUSH(STK_ALT,pat,s,sprev)
#define STACK_PUSH_POS(s,sprev)         STACK_PUSH(STK_POS,NULL_UCHARP,s,sprev)
#define STACK_PUSH_POS_NOT(pat,s,sprev) STACK_PUSH(STK_POS_NOT,pat,s,sprev)
#define STACK_PUSH_STOP_BT              STACK_PUSH_TYPE(STK_STOP_BT)
#define STACK_PUSH_LOOK_BEHIND_NOT(pat,s,sprev) \
        STACK_PUSH(STK_LOOK_BEHIND_NOT,pat,s,sprev)

#define STACK_PUSH_REPEAT(pat,low,up) do {\
  STACK_ENSURE(1);\
  stk->type = STK_REPEAT;\
  stk->u.repeat.pcode  = (pat);\
  stk->u.repeat.lower  = (low);\
  stk->u.repeat.upper  = (up);\
  stk->u.repeat.count  = 0;\
  STACK_INC;\
} while(0)

#define STACK_PUSH_REPEAT_INC(sindex) do {\
  STACK_ENSURE(1);\
  stk->type = STK_REPEAT_INC;\
  stk->u.repeat_inc.si  = (sindex);\
  STACK_INC;\
} while(0)

#define STACK_PUSH_MEM_START(mnum, s) do {\
  STACK_ENSURE(1);\
  stk->type = STK_MEM_START;\
  stk->u.mem.num      = (mnum);\
  stk->u.mem.pstr     = (s);\
  stk->u.mem.start    = mem_start_stk[mnum];\
  stk->u.mem.end      = mem_end_stk[mnum];\
  mem_start_stk[mnum] = GET_STACK_INDEX(stk);\
  mem_end_stk[mnum]   = INVALID_STACK_INDEX;\
  STACK_INC;\
} while(0)

#define STACK_PUSH_MEM_END(mnum, s) do {\
  STACK_ENSURE(1);\
  stk->type = STK_MEM_END;\
  stk->u.mem.num    = (mnum);\
  stk->u.mem.pstr   = (s);\
  stk->u.mem.start  = mem_start_stk[mnum];\
  mem_end_stk[mnum] = GET_STACK_INDEX(stk);\
  STACK_INC;\
} while(0)

#define STACK_PUSH_NULL_CHECK_START(cnum, s) do {\
  STACK_ENSURE(1);\
  stk->type = STK_NULL_CHECK_START;\
  stk->u.null_check.num  = (cnum);\
  stk->u.null_check.pstr = (s);\
  STACK_INC;\
} while(0)

#define STACK_POP  do {\
  while (1) {\
    stk--;\
    if (stk->type <= STK_POS_NOT)  break;\
    else if (stk->type == STK_MEM_START) {\
      mem_start_stk[stk->u.mem.num] = stk->u.mem.start;\
      mem_end_stk[stk->u.mem.num]   = stk->u.mem.end;\
    }\
    else if (stk->type == STK_MEM_END) {\
      mem_end_stk[stk->u.mem.num] = INVALID_STACK_INDEX;\
    }\
    else if (stk->type == STK_REPEAT_INC) {\
      STACK_AT(stk->u.repeat_inc.si)->u.repeat.count--;\
    }\
  }\
} while(0)

#define STACK_POP_TIL_POS_NOT  do {\
  while (1) {\
    stk--;\
    if (stk < stk_base)  goto stack_error;\
    if (stk->type == STK_POS_NOT) break;\
    else if (stk->type == STK_MEM_END) {\
      mem_end_stk[stk->u.mem.num] = INVALID_STACK_INDEX;\
    }\
    else if (stk->type == STK_MEM_START) {\
      mem_start_stk[stk->u.mem.num] = stk->u.mem.start;\
      mem_end_stk[stk->u.mem.num]   = stk->u.mem.end;\
    }\
    else if (stk->type == STK_REPEAT_INC) {\
      STACK_AT(stk->u.repeat_inc.si)->u.repeat.count--;\
    }\
  }\
} while(0)

#define STACK_POP_TIL_LOOK_BEHIND_NOT  do {\
  while (1) {\
    stk--;\
    if (stk < stk_base)  goto stack_error;\
    if (stk->type == STK_LOOK_BEHIND_NOT) break;\
    else if (stk->type == STK_MEM_END) {\
      mem_end_stk[stk->u.mem.num] = INVALID_STACK_INDEX;\
    }\
    else if (stk->type == STK_MEM_START) {\
      mem_start_stk[stk->u.mem.num] = stk->u.mem.start;\
      mem_end_stk[stk->u.mem.num]   = stk->u.mem.end;\
    }\
  }\
} while(0)

#define STACK_POS_END(k) do {\
  k = stk;\
  while (1) {\
    k--;\
    if (k <= stk_base)  goto stack_error;\
    if (k->type <= STK_POS_NOT || k->type == STK_NULL_CHECK_START) {\
      k->type = STK_VOID;\
    }\
    else if (k->type == STK_POS) {\
      k->type = STK_VOID;\
      break;\
    }\
  }\
} while(0)

#define STACK_STOP_BT_END do {\
  StackType *k = stk;\
  while (1) {\
    k--;\
    if (k <= stk_base)  goto stack_error;\
    if (k->type <= STK_POS_NOT || k->type == STK_NULL_CHECK_START) {\
      k->type = STK_VOID;\
    }\
    else if (k->type == STK_STOP_BT) {\
      k->type = STK_VOID;\
      break;\
    }\
  }\
} while(0)

#define STACK_NULL_CHECK(isnull,id,s) do {\
  StackType* k = stk;\
  k--;\
  while (k >= stk_base) {\
    if (k->type == STK_NULL_CHECK_START) {\
      if (k->u.null_check.num == (id)) {\
        (isnull) = (k->u.null_check.pstr == (s));\
        break;\
      }\
    }\
    k--;\
  }\
} while(0)


#define SBTRANSCMP(a,b)    (TTRANS(transtable,a) == TTRANS(transtable,b))
#define SBTRANS1CMP(a,c)   (TTRANS(transtable,a) == (c))
#define SBCMP(a,b)         (ignore_case ? SBTRANSCMP(a,b)  : (a) == (b))

#define STRING_CMP(a,b,len) do {\
  if (ignore_case) {\
    while (len-- > 0) {\
      if (! SBTRANSCMP(*a, *b)) goto fail;\
	a++; b++;\
    }\
  }\
  else {\
    while (len-- > 0) {\
      if (*a++ != *b++) goto fail;\
    }\
  }\
} while(0)

#define ON_STR_BEGIN(s)  ((s) == str)
#define ON_STR_END(s)    ((s) == end)
#define IS_EMPTY_STR     (str == end)

#define DATA_ENSURE(n) \
  if (s + (n) > end) goto fail

#define DATA_ENSURE_CHECK(n)   (s + (n) <= end)

#ifdef REG_DEBUG
static void print_compiled_byte_code(FILE* f, UChar* bp, UChar** nextp);
static void print_compiled_byte_code_list(FILE* f, regex_t* reg);
#endif

#ifdef REG_DEBUG_STATISTICS

#define USE_TIMEOFDAY

#ifdef USE_TIMEOFDAY
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
static struct timeval ts, te;
#define GETTIME(t)        gettimeofday(&(t), (struct timezone* )0)
#define TIMEDIFF(te,ts)   (((te).tv_usec - (ts).tv_usec) + \
                           (((te).tv_sec - (ts).tv_sec)*1000000))
#else
#ifdef HAVE_SYS_TIMES_H
#include <sys/times.h>
#endif
static struct tms ts, te;
#define GETTIME(t)         times(&(t))
#define TIMEDIFF(te,ts)   ((te).tms_utime - (ts).tms_utime)
#endif

static void print_statistics(FILE* f);
static int OpCounter[256];
static int OpPrevCounter[256];
static unsigned long OpTime[256];
static int OpCurr = OP_FINISH;
static int OpPrevTarget = OP_FAIL;
static int MaxStackDepth = 0;

#define STATISTICS_INIT do {\
  int i;\
  for (i = 0; i < 256; i++) {\
    OpCounter[i] = OpPrevCounter[i] = 0; OpTime[i] = 0;\
  }\
  MaxStackDepth = 0;\
} while(0)

#define STATISTICS_PRINT_RESULT  print_statistics(stderr)

#define STAT_OP_IN(opcode) do {\
  if (opcode == OpPrevTarget) OpPrevCounter[OpCurr]++;\
  OpCurr = opcode;\
  OpCounter[opcode]++;\
  GETTIME(ts);\
} while (0)

#define STAT_OP_OUT do {\
  GETTIME(te);\
  OpTime[OpCurr] += TIMEDIFF(te, ts);\
} while (0)

#ifdef RUBY_PLATFORM
static VALUE regex_stat_print() { STATISTICS_PRINT_RESULT; return Qnil; }

void Init_REGEX_STAT()
{
  rb_define_global_function("regex_stat_print", regex_stat_print, 0);
}
#endif

#define STACK_INC do {\
  stk++;\
  if (stk - stk_base > MaxStackDepth) \
    MaxStackDepth = stk - stk_base;\
} while (0)

#else
#define STACK_INC     stk++

#define STATISTICS_INIT
#define STAT_OP_IN(opcode)
#define STAT_OP_OUT
#define STATISTICS_PRINT_RESULT
#endif

/* match data(str - end) from position (sstart). */
/* if sstart == str then set sprev to NULL. */
static int
match_at(regex_t* reg, UChar* str, UChar* end, UChar* sstart,
	 UChar* sprev, RegRegion* region, MatchStackAlloc* msa)
{
  static UChar FinishCode[] = { OP_FINISH };

  int i, n, max_mem, best_len;
  LengthType tlen, tlen2;
  MemNumType mem;
  RelAddrType addr;
  RegOptionType option = reg->options;
  RegCharEncoding encode = reg->code;
  unsigned char* transtable = reg->transtable;
  int ignore_case;
  UChar *s, *q, *sbegin;
  UChar *p = reg->p;
  char *alloca_base;
  StackType *stk_alloc, *stk_base, *stk, *stk_end;
  StackType *stkp; /* used as any purpose. */
  StackIndex *repeat_stk;
  StackIndex *mem_start_stk, *mem_end_stk;
  n = reg->num_repeat + reg->max_mem * 2;

  STACK_INIT(alloca_base, n, INIT_MATCH_STACK_SIZE);
  ignore_case = IS_IGNORECASE(option);
  max_mem = reg->max_mem;
  repeat_stk = (StackIndex* )alloca_base;

  mem_start_stk = (StackIndex* )(repeat_stk + reg->num_repeat);
  mem_end_stk   = mem_start_stk + max_mem;
  mem_start_stk--; /* for index start from 1,
		      mem_start_stk[1]..mem_start_stk[max_mem] */
  mem_end_stk--;   /* for index start from 1,
		      mem_end_stk[1]..mem_end_stk[max_mem] */
  for (i = 1; i <= max_mem; i++) {
    mem_start_stk[i] = mem_end_stk[i] = INVALID_STACK_INDEX;
  }

#ifdef REG_DEBUG_MATCH
  fprintf(stderr, "match_at: str: %d, end: %d, start: %d, sprev: %d\n",
	  (int )str, (int )end, (int )sstart, (int )sprev);
  fprintf(stderr, "size: %d, start offset: %d\n",
	  (int )(end - str), (int )(sstart - str));
#endif

  /* bottom stack: pcode -> OP_FINISH position */
  STACK_PUSH(STK_ALT, FinishCode, NULL_UCHARP, NULL_UCHARP);
  best_len = REG_MISMATCH;
  s = sstart;
  while (1) {
#ifdef REG_DEBUG_MATCH
    {
      UChar *q, *bp, buf[50];
      int len;
      fprintf(stderr, "%4d> \"", (int )(s - str));
      bp = buf;
      for (i = 0, q = s; i < 7 && q < end; i++) {
	len = mblen(encode, *q);
	while (len-- > 0) *bp++ = *q++;
      }
      if (q < end) { xmemcpy(bp, "...\"", 4); bp += 4; }
      else         { xmemcpy(bp, "\"",    1); bp += 1; }
      *bp = 0;
      fputs(buf, stderr);
      for (i = 0; i < 20 - (bp - buf); i++) fputc(' ', stderr);
      print_compiled_byte_code(stderr, p, NULL);
      fprintf(stderr, "\n");
    }
#endif

    sbegin = s;
    switch (*p++) {
    case OP_END:  STAT_OP_IN(OP_END);
      n = s - sstart;
      if (n > best_len) {
	best_len = n;
	if (region) {
	  region->beg[0] = sstart - str;
	  region->end[0] = s      - str;
	  for (i = 1; i <= max_mem; i++) {
	    if (mem_end_stk[i] != INVALID_STACK_INDEX) {
	      if (GET_MEM_STATS_AT(reg->mem_stats, i)) {
		region->beg[i] = STACK_AT(mem_start_stk[i])->u.mem.pstr - str;
		region->end[i] = STACK_AT(mem_end_stk[i])->u.mem.pstr   - str;
	      }
	      else {
		region->beg[i] = STACK_AT(mem_start_stk[i])->u.mem.pstr - str;
		region->end[i] = (UChar* )((void* )mem_end_stk[i]) - str;
	      }
	    }
	    else {
	      region->beg[i] = region->end[i] = -1;
	    }
	  }
	}
      }
      STAT_OP_OUT;

      if (IS_FIND_CONDITION(option)) {
	if (IS_FIND_NOT_EMPTY(option) && s == sstart) {
	  best_len = REG_MISMATCH;
	  goto fail; /* for retry */
	}
	if (IS_FIND_LONGEST(option) && s < end) {
	  goto fail; /* for retry */
	}
      }
      else {
	/* default behavior: return first-matching result. */
	goto finish;
      }
      break;

    case OP_EXACT1:  STAT_OP_IN(OP_EXACT1);
#if 0
      DATA_ENSURE(1);
      if (*p != *s) goto fail;
      p++; s++;
#endif
      if (*p != *s++) goto fail;
      DATA_ENSURE(0);
      p++;
      STAT_OP_OUT;
      break;

    case OP_EXACT1_IC:  STAT_OP_IN(OP_EXACT1_IC);
      if (! SBTRANSCMP(*p, *s)) goto fail;
      DATA_ENSURE(1);
      p++; s++;
      STAT_OP_OUT;
      break;

    case OP_EXACT2:  STAT_OP_IN(OP_EXACT2);
      DATA_ENSURE(2);
      if (*p++ != *s++) goto fail;
      if (*p   != *s)   goto fail;
      sprev = s;
      p++; s++;
      STAT_OP_OUT;
      continue;
      break;

    case OP_EXACT3:  STAT_OP_IN(OP_EXACT3);
      DATA_ENSURE(3);
      if (*p++ != *s++) goto fail;
      if (*p++ != *s++) goto fail;
      if (*p++ != *s)   goto fail;
      sprev = s;
      s++;
      STAT_OP_OUT;
      continue;
      break;

    case OP_EXACT4:  STAT_OP_IN(OP_EXACT4);
      DATA_ENSURE(4);
      if (*p++ != *s++) goto fail;
      if (*p++ != *s++) goto fail;
      if (*p++ != *s++) goto fail;
      if (*p++ != *s)   goto fail;
      sprev = s;
      s++;
      STAT_OP_OUT;
      continue;
      break;

    case OP_EXACT5:  STAT_OP_IN(OP_EXACT5);
      DATA_ENSURE(5);
      if (*p++ != *s++) goto fail;
      if (*p++ != *s++) goto fail;
      if (*p++ != *s++) goto fail;
      if (*p++ != *s++) goto fail;
      if (*p++ != *s)   goto fail;
      sprev = s;
      s++;
      STAT_OP_OUT;
      continue;
      break;

    case OP_EXACTN:  STAT_OP_IN(OP_EXACTN);
      GET_LENGTH_INC(tlen, p);
      DATA_ENSURE(tlen);
      while (tlen-- > 0) {
	if (*p++ != *s++) goto fail;
      }
      sprev = s - 1;
      STAT_OP_OUT;
      continue;
      break;

    case OP_EXACTN_IC:  STAT_OP_IN(OP_EXACTN_IC);
      GET_LENGTH_INC(tlen, p);
      DATA_ENSURE(tlen);
      while (tlen-- > 0) {
	if (! SBTRANSCMP(*p, *s)) goto fail;
	p++; s++;
      }
      sprev = s - 1;
      STAT_OP_OUT;
      continue;
      break;

    case OP_EXACTMB2N1:  STAT_OP_IN(OP_EXACTMB2N1);
      DATA_ENSURE(2);
      if (*p++ != *s++) goto fail;
      if (*p++ != *s++) goto fail;
      STAT_OP_OUT;
      break;

    case OP_EXACTMB2N2:  STAT_OP_IN(OP_EXACTMB2N2);
      DATA_ENSURE(4);
      if (*p++ != *s++) goto fail;
      if (*p++ != *s++) goto fail;
      sprev = s;
      if (*p++ != *s++) goto fail;
      if (*p++ != *s++) goto fail;
      STAT_OP_OUT;
      continue;
      break;

    case OP_EXACTMB2N3:  STAT_OP_IN(OP_EXACTMB2N3);
      DATA_ENSURE(6);
      if (*p++ != *s++) goto fail;
      if (*p++ != *s++) goto fail;
      if (*p++ != *s++) goto fail;
      if (*p++ != *s++) goto fail;
      sprev = s;
      if (*p++ != *s++) goto fail;
      if (*p++ != *s++) goto fail;
      STAT_OP_OUT;
      continue;
      break;

    case OP_EXACTMB2N:  STAT_OP_IN(OP_EXACTMB2N);
      GET_LENGTH_INC(tlen, p);
      DATA_ENSURE(tlen * 2);
      while (tlen-- > 0) {
	if (*p++ != *s++) goto fail;
	if (*p++ != *s++) goto fail;
      }
      sprev = s - 2;
      STAT_OP_OUT;
      continue;
      break;

    case OP_EXACTMB3N:  STAT_OP_IN(OP_EXACTMB3N);
      GET_LENGTH_INC(tlen, p);
      DATA_ENSURE(tlen * 3);
      while (tlen-- > 0) {
	if (*p++ != *s++) goto fail;
	if (*p++ != *s++) goto fail;
	if (*p++ != *s++) goto fail;
      }
      sprev = s - 3;
      STAT_OP_OUT;
      continue;
      break;

    case OP_EXACTMBN:  STAT_OP_IN(OP_EXACTMBN);
      GET_LENGTH_INC(tlen,  p);  /* mb-len */
      GET_LENGTH_INC(tlen2, p);  /* string len */
      tlen2 *= tlen;
      DATA_ENSURE(tlen2);
      while (tlen2-- > 0) {
	if (*p++ != *s++) goto fail;
      }
      sprev = s - tlen;
      STAT_OP_OUT;
      continue;
      break;

    case OP_CCLASS:  STAT_OP_IN(OP_CCLASS);
      DATA_ENSURE(1);
      if (BITSET_AT(((BitSetRef )p), *s) == 0) goto fail;
      p += SIZE_BITSET;
      s += mblen(encode, *s);   /* OP_CCLASS can match mb-code. \D, \S */
      STAT_OP_OUT;
      break;

    case OP_CCLASS_MB:  STAT_OP_IN(OP_CCLASS_MB);
      if (BITSET_AT((BitSetRef )p, *s) == 0) goto fail;
      p += SIZE_BITSET;

    cclass_mb:
      GET_LENGTH_INC(tlen, p);
      {
	WCInt wc;
	UChar *ss;
	int mb_len = mblen(encode, *s);

	DATA_ENSURE(mb_len);
	ss = s;
	s += mb_len;
	wc = MB2WC(ss, s, encode);

#ifdef UNALIGNED_WORD_ACCESS
	if (! is_in_wc_range(p, wc)) goto fail;
#else
	q = p;
	ALIGNMENT_RIGHT(q);
	if (! is_in_wc_range(q, wc)) goto fail;
#endif
      }
      p += tlen;
      STAT_OP_OUT;
      break;

    case OP_CCLASS_MIX:  STAT_OP_IN(OP_CCLASS_MIX);
      DATA_ENSURE(1);
      if (ismb(encode, *s)) {
	p += SIZE_BITSET;
	goto cclass_mb;
      }
      else {
	if (BITSET_AT(((BitSetRef )p), *s) == 0)
	  goto fail;

	p += SIZE_BITSET;
	GET_LENGTH_INC(tlen, p);
	p += tlen;
	s++;
      }
      STAT_OP_OUT;
      break;

    case OP_CCLASS_NOT:  STAT_OP_IN(OP_CCLASS_NOT);
      DATA_ENSURE(1);
      if (BITSET_AT(((BitSetRef )p), *s) != 0) goto fail;
      p += SIZE_BITSET;
      s += mblen(encode, *s);
      STAT_OP_OUT;
      break;

    case OP_CCLASS_MB_NOT:  STAT_OP_IN(OP_CCLASS_MB_NOT);
      if (BITSET_AT((BitSetRef )p, *s) == 0) {
	s += mblen(encode, *s);
	if (s > end) s = end;
	p += SIZE_BITSET;
	GET_LENGTH_INC(tlen, p);
	p += tlen;
	goto success;
      }
      p += SIZE_BITSET;

    cclass_mb_not:
      GET_LENGTH_INC(tlen, p);
      {
	WCInt wc;
	UChar *ss;
	int mb_len = mblen(encode, *s);

	if (s + mb_len > end) {
	  s = end;
	  p += tlen;
	  goto success;
	}

	ss = s;
	s += mb_len;
	wc = MB2WC(ss, s, encode);

#ifdef UNALIGNED_WORD_ACCESS
	if (is_in_wc_range(p, wc)) goto fail;
#else
	q = p;
	ALIGNMENT_RIGHT(q);
	if (is_in_wc_range(q, wc)) goto fail;
#endif
      }
      p += tlen;

    success:
      STAT_OP_OUT;
      break;

    case OP_CCLASS_MIX_NOT:  STAT_OP_IN(OP_CCLASS_MIX_NOT);
      DATA_ENSURE(1);
      if (ismb(encode, *s)) {
	p += SIZE_BITSET;
	goto cclass_mb_not;
      }
      else {
	if (BITSET_AT(((BitSetRef )p), *s) != 0)
	  goto fail;

	p += SIZE_BITSET;
	GET_LENGTH_INC(tlen, p);
	p += tlen;
	s++;
      }
      STAT_OP_OUT;
      break;

    case OP_ANYCHAR:  STAT_OP_IN(OP_ANYCHAR);
      DATA_ENSURE(1);
      if (ismb(encode, *s)) {
	n = mblen(encode, *s);
	DATA_ENSURE(n);
	s += n;
      }
      else {
	if (! IS_MULTILINE(option)) {
	  if (SBCMP(*s, NEWLINE)) {
	    goto fail;
	  }
	}
	s++;
      }
      STAT_OP_OUT;
      break;

    case OP_ANYCHAR_STAR:  STAT_OP_IN(OP_ANYCHAR_STAR);
      while (s < end) {
	STACK_PUSH_ALT(p, s, sprev);
	if (ismb(encode, *s)) {
	  n = mblen(encode, *s);
	  DATA_ENSURE(n);
	  sprev = s;
	  s += n;
	}
	else {
	  if (! IS_MULTILINE(option)) {
	    if (SBCMP(*s, NEWLINE))
	      goto fail;
	  }
	  sprev = s;
	  s++;
	}
      }
      STAT_OP_OUT;
      break;

    case OP_ANYCHAR_STAR_PEEK_NEXT:  STAT_OP_IN(OP_ANYCHAR_STAR_PEEK_NEXT);
      while (s < end) {
	if (*p == *s) {
	  STACK_PUSH_ALT(p + 1, s, sprev);
	}
	if (ismb(encode, *s)) {
	  n = mblen(encode, *s);
	  DATA_ENSURE(n);
	  sprev = s;
	  s += n;
	}
	else {
	  if (! IS_MULTILINE(option)) {
	    if (SBCMP(*s, NEWLINE))
	      goto fail;
	  }
	  sprev = s;
	  s++;
	}
      }
      p++;
      STAT_OP_OUT;
      break;

    case OP_WORD:  STAT_OP_IN(OP_WORD);
      DATA_ENSURE(1);
      if (! IS_WORD_STR_INC(encode, s, end))
	goto fail;
      STAT_OP_OUT;
      break;

    case OP_NOT_WORD:  STAT_OP_IN(OP_NOT_WORD);
      DATA_ENSURE(1);
      if (IS_WORD_STR_INC(encode, s, end))
	goto fail;
      STAT_OP_OUT;
      break;

#ifdef USE_ONIGURUMA_EXTENSION
    case OP_WORD_SB:  STAT_OP_IN(OP_WORD_SB);
      DATA_ENSURE(1);
      if (! IS_SB_WORD(encode, *s))
	goto fail;
      s++;
      STAT_OP_OUT;
      break;

    case OP_WORD_MB:  STAT_OP_IN(OP_WORD_MB);
      DATA_ENSURE(1);
      if (! IS_MB_WORD(encode, *s))
	goto fail;

      n = mblen(encode, *s);
      DATA_ENSURE(n);
      s += n;
      STAT_OP_OUT;
      break;
#endif

    case OP_WORD_BOUND:  STAT_OP_IN(OP_WORD_BOUND);
      if (ON_STR_BEGIN(s)) {
	DATA_ENSURE(1);
	if (! IS_WORD_STR(encode, s, end))
	  goto fail;
      }
      else if (ON_STR_END(s)) {
	if (! IS_WORD_STR(encode, sprev, end))
	  goto fail;
      }
      else {
	if (IS_WORD_STR(encode, s, end) == IS_WORD_STR(encode, sprev, end))
	  goto fail;
      }
      STAT_OP_OUT;
      continue;
      break;

    case OP_NOT_WORD_BOUND:  STAT_OP_IN(OP_NOT_WORD_BOUND);
      if (ON_STR_BEGIN(s)) {
	if (DATA_ENSURE_CHECK(1) && IS_WORD_STR(encode, s, end))
	  goto fail;
      }
      else if (ON_STR_END(s)) {
	if (IS_WORD_STR(encode, sprev, end))
	  goto fail;
      }
      else {
	if (IS_WORD_STR(encode, s, end) != IS_WORD_STR(encode, sprev, end))
	  goto fail;
      }
      STAT_OP_OUT;
      continue;
      break;

#ifdef USE_WORD_BEGIN_END
    case OP_WORD_BEGIN:  STAT_OP_IN(OP_WORD_BEGIN);
      if (DATA_ENSURE_CHECK(1) && IS_WORD_STR(encode, s, end)) {
	if (ON_STR_BEGIN(s) || !IS_WORD_STR(encode, sprev, end)) {
	  STAT_OP_OUT;
	  continue;
	}
      }
      goto fail;
      break;

    case OP_WORD_END:  STAT_OP_IN(OP_WORD_END);
      if (!ON_STR_BEGIN(s) && IS_WORD_STR(encode, sprev, end)) {
	if (ON_STR_END(s) || !IS_WORD_STR(encode, s, end)) {
	  STAT_OP_OUT;
	  continue;
	}
      }
      goto fail;
      break;
#endif

    case OP_BEGIN_BUF:  STAT_OP_IN(OP_BEGIN_BUF);
      if (! ON_STR_BEGIN(s)) goto fail;

      STAT_OP_OUT;
      continue;
      break;

    case OP_END_BUF:  STAT_OP_IN(OP_END_BUF);
      if (! ON_STR_END(s)) goto fail;

      STAT_OP_OUT;
      continue;
      break;

    case OP_BEGIN_LINE:  STAT_OP_IN(OP_BEGIN_LINE);
      if (ON_STR_BEGIN(s)) {
	STAT_OP_OUT;
	continue;
      }
      else if (IS_NEWLINE(*sprev) && !ON_STR_END(s)) {
	STAT_OP_OUT;
	continue;
      }
      goto fail;
      break;

    case OP_END_LINE:  STAT_OP_IN(OP_END_LINE);
      if (ON_STR_END(s)) {
	if (IS_EMPTY_STR || !IS_NEWLINE(*sprev)) {
	  STAT_OP_OUT;
	  continue;
	}
      }
      else if (IS_NEWLINE(*s)) {
	STAT_OP_OUT;
	continue;
      }
      goto fail;
      break;

    case OP_SEMI_END_BUF:  STAT_OP_IN(OP_SEMI_END_BUF);
      if (ON_STR_END(s)) {
	if (IS_EMPTY_STR || !IS_NEWLINE(*sprev)) {
	  STAT_OP_OUT;
	  continue;
	}
      }
      if (IS_NEWLINE(*s) && ON_STR_END(s+1)) {
	STAT_OP_OUT;
	continue;
      }
      goto fail;
      break;

    case OP_BEGIN_POSITION:  STAT_OP_IN(OP_BEGIN_POSITION);
      if (s != sstart)
	goto fail;

      STAT_OP_OUT;
      continue;
      break;

    case OP_MEMORY_START_PUSH:  STAT_OP_IN(OP_MEMORY_START_PUSH);
      GET_MEMNUM_INC(mem, p);
      STACK_PUSH_MEM_START(mem, s);
      STAT_OP_OUT;
      continue;
      break;

    case OP_MEMORY_END_PUSH:  STAT_OP_IN(OP_MEMORY_END_PUSH);
      GET_MEMNUM_INC(mem, p);
      STACK_PUSH_MEM_END(mem, s);
      STAT_OP_OUT;
      continue;
      break;

    case OP_MEMORY_END:  STAT_OP_IN(OP_MEMORY_END);
      GET_MEMNUM_INC(mem, p);
      mem_end_stk[mem] = (StackIndex )((void* )s);
      STAT_OP_OUT;
      continue;
      break;

    case OP_BACKREF1:  STAT_OP_IN(OP_BACKREF1);
      mem = 1;
      goto backref;
      break;

    case OP_BACKREF2:  STAT_OP_IN(OP_BACKREF2);
      mem = 2;
      goto backref;
      break;

    case OP_BACKREF3:  STAT_OP_IN(OP_BACKREF3);
      mem = 3;
      goto backref;
      break;

    case OP_BACKREFN:  STAT_OP_IN(OP_BACKREFN);
      GET_MEMNUM_INC(mem, p);
    backref:
      {
	int len;
	UChar *pstart, *pend;

	if (mem > max_mem) goto fail;
	if (mem_end_stk[mem] == INVALID_STACK_INDEX) goto fail;

	stkp = STACK_AT(mem_end_stk[mem]);
	if (stkp) {
	  pend = stkp->u.mem.pstr;
	  stkp = STACK_AT(stkp->u.mem.start);
	  pstart = stkp->u.mem.pstr;
	  n = pend - pstart;
	  DATA_ENSURE(n);
	  sprev = s;
	  STRING_CMP(pstart, s, n);
	  while (sprev + (len = mblen(encode, *sprev)) < s)
	    sprev += len;

	  STAT_OP_OUT;
	  continue;
	}
      }
      goto fail;
      break;

    case OP_SET_OPTION_PUSH:  STAT_OP_IN(OP_SET_OPTION_PUSH);
      GET_OPTION_INC(option, p);
      ignore_case = IS_IGNORECASE(option);
      STACK_PUSH_ALT(p, s, sprev);
      p += SIZE_OP_SET_OPTION + SIZE_OP_FAIL;
      STAT_OP_OUT;
      continue;
      break;

    case OP_SET_OPTION:  STAT_OP_IN(OP_SET_OPTION);
      GET_OPTION_INC(option, p);
      ignore_case = IS_IGNORECASE(option);
      STAT_OP_OUT;
      continue;
      break;

    case OP_NULL_CHECK_START:  STAT_OP_IN(OP_NULL_CHECK_START);
      GET_MEMNUM_INC(mem, p);    /* mem: null check id */
      STACK_PUSH_NULL_CHECK_START(mem, s);
      STAT_OP_OUT;
      continue;
      break;

    case OP_NULL_CHECK_END:  STAT_OP_IN(OP_NULL_CHECK_END);
      {
	int isnull;

	GET_MEMNUM_INC(mem, p); /* mem: null check id */
	STACK_NULL_CHECK(isnull, mem, s);
	if (isnull) {
#ifdef REG_DEBUG_MATCH
	  fprintf(stderr, "NULL_CHECK_END: skip  id:%d, s:%d\n",
		  (int )mem, (int )s);
#endif
	  /* empty loop founded, skip next instruction */
	  switch (*p++) {
	  case OP_JUMP:
	  case OP_PUSH:
	    GET_RELADDR_INC(addr, p);
	    break;
	  case OP_REPEAT_INC:
	  case OP_REPEAT_INC_NG:
	    GET_MEMNUM_INC(mem, p);
	    break;
	  default:
	    goto unexpected_bytecode_error;
	    break;
	  }
	}
      }
      STAT_OP_OUT;
      continue;
      break;

    case OP_JUMP:  STAT_OP_IN(OP_JUMP);
      GET_RELADDR_INC(addr, p);
      p += addr;
      STAT_OP_OUT;
      continue;
      break;

    case OP_PUSH:  STAT_OP_IN(OP_PUSH);
      GET_RELADDR_INC(addr, p);
      STACK_PUSH_ALT(p + addr, s, sprev);
      STAT_OP_OUT;
      continue;
      break;

    case OP_PUSH_OR_JUMP_EXACT1:  STAT_OP_IN(OP_PUSH_OR_JUMP_EXACT1);
      GET_RELADDR_INC(addr, p);
      if (*p == *s && DATA_ENSURE_CHECK(1)) {
	p++;
	STACK_PUSH_ALT(p + addr, s, sprev);
	STAT_OP_OUT;
	continue;
      }
      p += (addr + 1);
      STAT_OP_OUT;
      continue;
      break;

    case OP_PUSH_IF_PEEK_NEXT:  STAT_OP_IN(OP_PUSH_IF_PEEK_NEXT);
      GET_RELADDR_INC(addr, p);
      if (*p == *s) {
	p++;
	STACK_PUSH_ALT(p + addr, s, sprev);
	STAT_OP_OUT;
	continue;
      }
      p++;
      STAT_OP_OUT;
      continue;
      break;

    case OP_REPEAT:  STAT_OP_IN(OP_REPEAT);
      {
	RepeatNumType lower, upper;

	GET_MEMNUM_INC(mem, p);    /* mem: OP_REPEAT ID */
	GET_RELADDR_INC(addr, p);
	GET_REPEATNUM_INC(lower, p);
	GET_REPEATNUM_INC(upper, p);

	STACK_ENSURE(1);
	repeat_stk[mem] = GET_STACK_INDEX(stk);
	STACK_PUSH_REPEAT(p, lower, upper);

	if (lower == 0) {
	  STACK_PUSH_ALT(p + addr, s, sprev);
	}
      }
      STAT_OP_OUT;
      continue;
      break;

    case OP_REPEAT_NG:  STAT_OP_IN(OP_REPEAT_NG);
      {
	RepeatNumType lower, upper;

	GET_MEMNUM_INC(mem, p);    /* mem: OP_REPEAT ID */
	GET_RELADDR_INC(addr, p);
	GET_REPEATNUM_INC(lower, p);
	GET_REPEATNUM_INC(upper, p);

	STACK_ENSURE(1);
	repeat_stk[mem] = GET_STACK_INDEX(stk);
	STACK_PUSH_REPEAT(p, lower, upper);

	if (lower == 0) {
	  STACK_PUSH_ALT(p, s, sprev);
	  p += addr;
	}
      }
      STAT_OP_OUT;
      continue;
      break;

    case OP_REPEAT_INC:  STAT_OP_IN(OP_REPEAT_INC);
      {
	StackIndex si;

	GET_MEMNUM_INC(mem, p); /* mem: OP_REPEAT ID */
	si = repeat_stk[mem];
	stkp = STACK_AT(si);
	stkp->u.repeat.count++;
	if (stkp->u.repeat.count == stkp->u.repeat.upper) {
	  /* end of repeat. Nothing to do. */
	}
	else if (stkp->u.repeat.count >= stkp->u.repeat.lower) {
	  STACK_PUSH_ALT(p, s, sprev);
	  p = stkp->u.repeat.pcode;
	}
	else {
	  p = stkp->u.repeat.pcode;
	}
	STACK_PUSH_REPEAT_INC(si);
      }
      STAT_OP_OUT;
      continue;
      break;

    case OP_REPEAT_INC_NG:  STAT_OP_IN(OP_REPEAT_INC_NG);
      {
	StackIndex si;

	GET_MEMNUM_INC(mem, p); /* mem: OP_REPEAT ID */
	si = repeat_stk[mem];
	stkp = STACK_AT(si);
	stkp->u.repeat.count++;
	if (stkp->u.repeat.count == stkp->u.repeat.upper) {
	  /* end of repeat. Nothing to do. */
	}
	else if (stkp->u.repeat.count >= stkp->u.repeat.lower) {
	  STACK_PUSH_ALT(stkp->u.repeat.pcode, s, sprev);
	}
	else {
	  p = stkp->u.repeat.pcode;
	}
	STACK_PUSH_REPEAT_INC(si);
      }
      STAT_OP_OUT;
      continue;
      break;

    case OP_PUSH_POS:  STAT_OP_IN(OP_PUSH_POS);
      STACK_PUSH_POS(s, sprev);
      STAT_OP_OUT;
      continue;
      break;

    case OP_POP_POS:  STAT_OP_IN(OP_POP_POS);
      {
	STACK_POS_END(stkp);
	s     = stkp->u.state.pstr;
	sprev = stkp->u.state.pstr_prev;
      }
      STAT_OP_OUT;
      continue;
      break;

    case OP_PUSH_POS_NOT:  STAT_OP_IN(OP_PUSH_POS_NOT);
      GET_RELADDR_INC(addr, p);
      STACK_PUSH_POS_NOT(p + addr, s, sprev);
      STAT_OP_OUT;
      continue;
      break;

    case OP_FAIL_POS:  STAT_OP_IN(OP_FAIL_POS);
      STACK_POP_TIL_POS_NOT;
      goto fail;
      break;

    case OP_PUSH_STOP_BT:  STAT_OP_IN(OP_PUSH_STOP_BT);
      STACK_PUSH_STOP_BT;
      STAT_OP_OUT;
      continue;
      break;

    case OP_POP_STOP_BT:  STAT_OP_IN(OP_POP_STOP_BT);
      STACK_STOP_BT_END;
      STAT_OP_OUT;
      continue;
      break;

    case OP_LOOK_BEHIND:  STAT_OP_IN(OP_LOOK_BEHIND);
      GET_LENGTH_INC(tlen, p);
      s = MBBACK(encode, str, s, (int )tlen);
      if (IS_NULL(s)) goto fail;
      sprev = get_prev_char_head(encode, str, s);
      STAT_OP_OUT;
      continue;
      break;

    case OP_PUSH_LOOK_BEHIND_NOT:  STAT_OP_IN(OP_PUSH_LOOK_BEHIND_NOT);
      GET_RELADDR_INC(addr, p);
      GET_LENGTH_INC(tlen, p);
      q = MBBACK(encode, str, s, (int )tlen);
      if (IS_NULL(q)) {
	/* too short case -> success. ex. /(?<!XXX)a/.match("a")
	   If you want to change to fail, replace following line. */
	p += addr;
	/* goto fail; */
      }
      else {
	STACK_PUSH_LOOK_BEHIND_NOT(p + addr, s, sprev);
	s = q;
	sprev = get_prev_char_head(encode, str, s);
      }
      STAT_OP_OUT;
      continue;
      break;

    case OP_FAIL_LOOK_BEHIND_NOT:  STAT_OP_IN(OP_FAIL_LOOK_BEHIND_NOT);
      STACK_POP_TIL_LOOK_BEHIND_NOT;
      goto fail;
      break;

    case OP_FINISH:
      goto finish;
      break;

    fail:
      STAT_OP_OUT;
    case OP_FAIL:
    STAT_OP_IN(OP_FAIL);

      STACK_POP;
      p     = stk->u.state.pcode;
      s     = stk->u.state.pstr;
      sprev = stk->u.state.pstr_prev;
      STAT_OP_OUT;
      continue;
      break;

    default:
      goto bytecode_error;

    } /* end of switch */
    sprev = sbegin;
  } /* end of while(1) */

 finish:
  STACK_SAVE;
  return best_len;

 stack_error:
  STACK_SAVE;
  return REGERR_STACK_BUG;

 bytecode_error:
  STACK_SAVE;
  return REGERR_UNDEFINED_BYTECODE;

 unexpected_bytecode_error:
  STACK_SAVE;
  return REGERR_UNEXPECTED_BYTECODE;
}

extern int
re_adjust_startpos(regex_t* reg, const char* string, int size,
		   int startpos, int range)
{
  if (startpos > 0 && mbmaxlen(reg->code) != 1 && startpos < size) {
    UChar *p;
    UChar *s = (UChar* )string + startpos;

    if (range > 0) {
      p = get_right_adjust_char_head(reg->code, (UChar* )string, s);
    }
    else {
      p = get_left_adjust_char_head(reg->code, (UChar* )string, s);
    }
    return p - (UChar* )string;
  }

  return startpos;
}

static UChar*
slow_search(RegCharEncoding code, UChar* target, UChar* target_end,
	    UChar* text, UChar* text_end, UChar* text_range)
{
  UChar *t, *p, *s, *end;

  end = text_end - (target_end - target) + 1;
  if (end > text_range)
    end = text_range;

  s = text;

  while (s < end) {
    if (*s == *target) {
      p = s + 1;
      t = target + 1;
      while (t < target_end) {
	if (*t != *p++)
	  break;
	t++;
      }
      if (t == target_end)
	return s;
    }
    s += mblen(code, *s);
  }

  return (UChar* )NULL;
}

static int
str_trans_match_after_head_byte(RegCharEncoding code, UChar* trans,
				int len, UChar* t, UChar* tend, UChar* p)
{
  while (--len > 0) {
    if (*t != *p) break;
    t++; p++;
  }

  if (len == 0) {
    while (t < tend) {
      len = mblen(code, *p);
      if (len == 1) {
	if (*t != TTRANS(trans, *p))
	  break;
	p++;
	t++;
      }
      else {
	if (*t != *p++)  break;
	t++;
	while (--len > 0) {
	  if (*t != *p) break;
	  t++; p++;
	}
	if (len > 0) break;
      }
    }
    if (t == tend)
      return 1;
  }

  return 0;
}

static UChar*
slow_search_ic(RegCharEncoding code, UChar* transtable, UChar* target,
	       UChar* target_end, UChar* text, UChar* text_end, UChar* text_range)
{
  int len;
  UChar *t, *p, *s, *end;

  end = text_end - (target_end - target) + 1;
  if (end > text_range)
    end = text_range;

  s = text;

  while (s < end) {
    len = mblen(code, *s);
    if (*s == *target || (len == 1 && TTRANS(transtable, *s) == *target)) {
      p = s + 1;
      t = target + 1;
      if (str_trans_match_after_head_byte(code, transtable, len, t, target_end, p))
	return s;
    }
    s += len;
  }

  return (UChar* )NULL;
}

static UChar*
slow_search_backward(RegCharEncoding code, UChar* target, UChar* target_end,
	     UChar* text, UChar* adjust_text, UChar* text_end, UChar* text_start)
{
  UChar *t, *p, *s;

  s = text_end - (target_end - target);
  if (s > text_start)
    s = text_start;
  else
    s = get_left_adjust_char_head(code, adjust_text, s);

  while (s >= text) {
    if (*s == *target) {
      p = s + 1;
      t = target + 1;
      while (t < target_end) {
	if (*t != *p++)
	  break;
	t++;
      }
      if (t == target_end)
	return s;
    }
    s = get_prev_char_head(code, adjust_text, s);
  }

  return (UChar* )NULL;
}

static UChar*
slow_search_backward_ic(RegCharEncoding code, UChar* transtable, UChar* target,
			UChar* target_end, UChar* text, UChar* adjust_text,
			UChar* text_end, UChar* text_start)
{
  int len;
  UChar *t, *p, *s;

  s = text_end - (target_end - target);
  if (s > text_start)
    s = text_start;
  else
    s = get_left_adjust_char_head(code, adjust_text, s);

  while (s >= text) {
    len = mblen(code, *s);
    if (*s == *target || (len == 1 && TTRANS(transtable, *s) == *target)) {
      p = s + 1;
      t = target + 1;
      if (str_trans_match_after_head_byte(code, transtable, len,
					  t, target_end, p))
	return s;
    }
    s = get_prev_char_head(code, adjust_text, s);
  }

  return (UChar* )NULL;
}

/* set skip map for Boyer-Moor search */
static int
set_bm_skip(UChar* s, UChar* end, UChar* transtable, UChar skip[], int** int_skip)
{
  int i, len;

  len = end - s;
  if (len < REG_CHAR_TABLE_SIZE) {
    for (i = 0; i < REG_CHAR_TABLE_SIZE; i++) skip[i] = len;

    if (transtable) {
      for (i = 0; i < len - 1; i++)
	skip[TTRANS(transtable, s[i])] = len - 1 - i;
    }
    else {
      for (i = 0; i < len - 1; i++)
	skip[s[i]] = len - 1 - i;
    }
  }
  else {
    if (IS_NULL(*int_skip)) {
      *int_skip = (int* )xmalloc(sizeof(int) * REG_CHAR_TABLE_SIZE);
      if (IS_NULL(*int_skip)) return REGERR_MEMORY;
    }
    for (i = 0; i < REG_CHAR_TABLE_SIZE; i++) (*int_skip)[i] = len;

    if (transtable) {
      for (i = 0; i < len - 1; i++)
	(*int_skip)[TTRANS(transtable, s[i])] = len - 1 - i;
    }
    else {
      for (i = 0; i < len - 1; i++)
	(*int_skip)[s[i]] = len - 1 - i;
    }
  }
  return 0;
}

static UChar*
bm_search(regex_t* reg, UChar* target, UChar* target_end,
	  UChar* text, UChar* text_end, UChar* text_range)
{
  UChar *s, *t, *p, *end;
  UChar *tail;
  int skip;

  end = text_range + (target_end - target) - 1;
  if (end > text_end)
    end = text_end;

  tail = target_end - 1;
  if (IS_INDEPENDENT_TRAIL(reg->code)) {
    s = text + (target_end - target) - 1;
    if (IS_NULL(reg->int_map)) {
      while (s < end) {
	p = s;
	t = tail;
	while (t >= target && *p == *t) {
	  p--; t--;
	}
	if (t < target) return p + 1;
	s += reg->map[*s];
      }
    }
    else { /* see int_map[] */
      while (s < end) {
	p = s;
	t = tail;
	while (t >= target && *p == *t) {
	  p--; t--;
	}
	if (t < target) return p + 1;
	s += reg->int_map[*s];
      }
    }
  }
  else {
    s = text;
    while ((s - text) < target_end - target) {
      s += mblen(reg->code, *s);
    }
    s--; /* set to text check tail position. */

    if (IS_NULL(reg->int_map)) {
      while (s < end) {
	p = s;
	t = tail;
	while (t >= target && *p == *t) {
	  p--; t--;
	}
	if (t < target) return p + 1;

	skip = reg->map[*s];
	p++;
	t = p;
	while ((p - t) < skip) {
	  p += mblen(reg->code, *p);
	}
	s += (p - t);
      }
    }
    else {
      while (s < end) {
	p = s;
	t = tail;
	while (t >= target && *p == *t) {
	  p--; t--;
	}
	if (t < target) return p + 1;

	skip = reg->int_map[*s];
	p++;
	t = p;
	while ((p - t) < skip) {
	  p += mblen(reg->code, *p);
	}
	s += (p - t);
      }
    }
  }

  return (UChar* )NULL;
}

static int
set_bm_backward_skip(UChar* s, UChar* end, UChar* transtable, int** skip)
{
  int i, len;

  if (IS_NULL(*skip)) {
    *skip = (int* )xmalloc(sizeof(int) * REG_CHAR_TABLE_SIZE);
    if (IS_NULL(*skip)) return REGERR_MEMORY;
  }

  len = end - s;
  for (i = 0; i < REG_CHAR_TABLE_SIZE; i++)
    (*skip)[i] = len;

  if (transtable) {
    for (i = len - 1; i > 0; i--)
      (*skip)[TTRANS(transtable, s[i])] = i;
  }
  else {
    for (i = len - 1; i > 0; i--)
      (*skip)[s[i]] = i;
  }
  return 0;
}

static UChar*
bm_search_backward(regex_t* reg, UChar* target, UChar* target_end, UChar* text,
		   UChar* adjust_text, UChar* text_end, UChar* text_start)
{
  UChar *s, *t, *p;

  s = text_end - (target_end - target);
  if (text_start < s)
    s = text_start;
  else
    s = get_left_adjust_char_head(reg->code, adjust_text, s);

  while (s >= text) {
    p = s;
    t = target;
    while (t < target_end && *p == *t) {
      p++; t++;
    }
    if (t == target_end)
      return s;

    s -= reg->int_map_backward[*s];
    s = get_left_adjust_char_head(reg->code, adjust_text, s);
  }

  return (UChar* )NULL;
}

static UChar*
map_search(RegCharEncoding code, UChar map[], UChar* text, UChar* text_range)
{
  UChar *s = text;

  while (s < text_range) {
    if (map[*s]) return s;

    s += mblen(code, *s);
  }
  return (UChar* )NULL;
}

static UChar*
map_search_backward(RegCharEncoding code, UChar map[],
		    UChar* text, UChar* adjust_text, UChar* text_start)
{
  UChar *s = text_start;

  while (s >= text) {
    if (map[*s]) return s;

    s = get_prev_char_head(code, adjust_text, s);
  }
  return (UChar* )NULL;
}

extern int
regex_match(regex_t* reg, UChar* str, UChar* end, UChar* at, RegRegion* region)
{
  int r;
  UChar *prev;
  MatchStackAlloc msa;

  MATCH_STACK_ALLOC_INIT(msa);

  r = regex_region_resize(region, reg->max_mem + 1);
  if (r == 0) {
    prev = get_prev_char_head(reg->code, str, at);
    r = match_at(reg, str, end, at, prev, region, &msa);
  }
  MATCH_STACK_ALLOC_FREE(msa);
  return r;
}

extern int
re_match(regex_t* reg, const char* str, int size, int pos,
	 struct re_registers* regs)
{
  return regex_match(reg, (UChar* )str, (UChar* )(str + size),
		     (UChar* )(str + pos), regs);
}

static int
forward_search_range(regex_t* reg, UChar* str, UChar* end, UChar* s,
		     UChar* range, UChar** low, UChar** high, UChar** low_prev)
{
  UChar *p, *pprev = (UChar* )NULL;

#ifdef REG_DEBUG_SEARCH
  fprintf(stderr, "forward_search_range: str: %d, end: %d, s: %d, range: %d\n",
	  (int )str, (int )end, (int )s, (int )range);
#endif

  p = s;
  if (reg->dmin > 0 && IS_SINGLEBYTE_CODE(reg->code))
    p += reg->dmin;

 retry:
  switch (reg->optimize) {
  case REG_OPTIMIZE_EXACT:
    p = slow_search(reg->code, reg->exact, reg->exact_end, p, end, range);
    break;
  case REG_OPTIMIZE_EXACT_IC:
    p = slow_search_ic(reg->code, reg->transtable,
		       reg->exact, reg->exact_end, p, end, range);
    break;

  case REG_OPTIMIZE_EXACT_BM:
    p = bm_search(reg, reg->exact, reg->exact_end, p, end, range);
    break;

  case REG_OPTIMIZE_MAP:
    p = map_search(reg->code, reg->map, p, range);
    break;
  }

  if (p && p < range) {
    if (p - reg->dmin < s) {
    retry_gate:
      pprev = p;
      p += mblen(reg->code, *p);
      goto retry;
    }

    if (reg->sub_anchor) {
      UChar* prev;

      switch (reg->sub_anchor) {
      case ANCHOR_BEGIN_LINE:
	if (!ON_STR_BEGIN(p)) {
	  prev = get_prev_char_head(reg->code, (pprev ? pprev : str), p);
	  if (!IS_NEWLINE(*prev))
	    goto retry_gate;
	}
	break;

      case ANCHOR_END_LINE:
	if (ON_STR_END(p)) {
	  prev = get_prev_char_head(reg->code, (pprev ? pprev : str), p);
	  if (prev && IS_NEWLINE(*prev))
	    goto retry_gate;
	}
	else if (!IS_NEWLINE(*p))
	  goto retry_gate;
	break;
      }
    }

    if (reg->dmax == 0) {
      *low = p;
      if (low_prev) {
	if (*low > s)
	  *low_prev = get_prev_char_head(reg->code, s, p);
	else
	  *low_prev = get_prev_char_head(reg->code, (pprev ? pprev : str), p);
      }
    }
    else {
      *low = p - reg->dmax;
      if (*low > s) {
	*low = get_right_adjust_char_head_with_prev(reg->code, s, *low, low_prev);
	if (low_prev && IS_NULL(*low_prev))
	  *low_prev = get_prev_char_head(reg->code, (pprev ? pprev : s), *low);
      }
      else {
	if (low_prev)
	  *low_prev = get_prev_char_head(reg->code, (pprev ? pprev : str), *low);
      }
    }
    /* no needs to adjust *high, *high is used as range check only */
    *high = p - reg->dmin;

#ifdef REG_DEBUG_SEARCH
    fprintf(stderr,
    "forward_search_range success: low: %d, high: %d, dmin: %d, dmax: %d\n",
	    (int )(*low - str), (int )(*high - str), reg->dmin, reg->dmax);
#endif
    return 1; /* success */
  }

  return 0; /* fail */
}

static int set_bm_backward_skip(UChar* s, UChar* end, UChar* transtbl, int** skip);

#define BM_BACKWARD_SEARCH_LENGTH_THRESHOLD   100

static int
backward_search_range(regex_t* reg, UChar* str, UChar* end, UChar* s,
		      UChar* range, UChar* adjrange, UChar** low, UChar** high)
{
  int r;
  UChar *p;

  range += reg->dmin;
  p = s;

 retry:
  switch (reg->optimize) {
  case REG_OPTIMIZE_EXACT:
  exact_method:
    p = slow_search_backward(reg->code, reg->exact, reg->exact_end,
			     range, adjrange, end, p);
    break;

  case REG_OPTIMIZE_EXACT_IC:
    p = slow_search_backward_ic(reg->code, reg->transtable, reg->exact,
				reg->exact_end, range, adjrange, end, p);
    break;

  case REG_OPTIMIZE_EXACT_BM:
    if (IS_NULL(reg->int_map_backward)) {
      if (s - range < BM_BACKWARD_SEARCH_LENGTH_THRESHOLD)
	goto exact_method;

      r = set_bm_backward_skip(reg->exact, reg->exact_end, (UChar* )NULL,
			       &(reg->int_map_backward));
      if (r) return r;
    }
    p = bm_search_backward(reg, reg->exact, reg->exact_end, range, adjrange,
			   end, p);
    break;

  case REG_OPTIMIZE_MAP:
    p = map_search_backward(reg->code, reg->map, range, adjrange, p);
    break;
  }

  if (p) {
    if (reg->sub_anchor) {
      UChar* prev;

      switch (reg->sub_anchor) {
      case ANCHOR_BEGIN_LINE:
	if (!ON_STR_BEGIN(p)) {
	  prev = get_prev_char_head(reg->code, adjrange, p);
	  if (!IS_NEWLINE(*prev)) {
	    p = prev;
	    goto retry;
	  }
	}
	break;

      case ANCHOR_END_LINE:
	if (ON_STR_END(p)) {
	  prev = get_prev_char_head(reg->code, adjrange, p);
	  if (IS_NULL(prev)) goto fail;
	  if (IS_NEWLINE(*prev)) {
	    p = prev;
	    goto retry;
	  }
	}
	else if (!IS_NEWLINE(*p)) {
	  p = get_prev_char_head(reg->code, adjrange, p);
	  if (IS_NULL(p)) goto fail;
	  goto retry;
	}
	break;
      }
    }

    /* no needs to adjust *high, *high is used as range check only */
    *low  = p - reg->dmax;
    *high = p - reg->dmin;
    *high = get_right_adjust_char_head(reg->code, adjrange, *high);

#ifdef REG_DEBUG_SEARCH
    fprintf(stderr, "backward_search_range: low: %d, high: %d\n",
	    (int )(*low - str), (int )(*high - str));
#endif
    return 1; /* success */
  }

 fail:
#ifdef REG_DEBUG_SEARCH
  fprintf(stderr, "backward_search_range: fail.\n");
#endif
  return 0; /* fail */
}


static void
regex_free_body(regex_t* reg)
{
  if (reg->p)       xfree(reg->p);
  if (reg->exact)   xfree(reg->exact);
  if (reg->int_map) xfree(reg->int_map);
  if (reg->int_map_backward)
    xfree(reg->int_map_backward);

  if (reg->chain)
    regex_free(reg->chain);
}

extern void
regex_free(regex_t* reg)
{
  if (reg) {
    regex_free_body(reg);
    xfree(reg);
  }
}

#define REGEX_TRANSFER(to,from) do {\
  (to)->state = REG_STATE_MODIFY;\
  regex_free_body(to);\
  xmemcpy(to, from, sizeof(regex_t));\
  xfree(from);\
} while (0)

static void
regex_transfer(regex_t* to, regex_t* from)
{
  THREAD_ATOMIC_START;
  REGEX_TRANSFER(to,from);
  THREAD_ATOMIC_END;
}

#define REGEX_CHAIN_HEAD(reg) do {\
  while (IS_NOT_NULL((reg)->chain)) {\
    (reg) = (reg)->chain;\
  }\
} while (0)

static void
regex_chain_link_add(regex_t* to, regex_t* add)
{
  THREAD_ATOMIC_START;
  REGEX_CHAIN_HEAD(to);
  to->chain = add;
  THREAD_ATOMIC_END;
}

static void
regex_chain_reduce(regex_t* reg)
{
  regex_t *head, *prev;

  THREAD_ATOMIC_START;
  prev = reg;
  head = prev->chain;
  if (IS_NOT_NULL(head)) {
    while (IS_NOT_NULL(head->chain)) {
      prev = head;
      head = head->chain;
    }
    prev->chain = (regex_t* )NULL;
    REGEX_TRANSFER(reg,head);
  }
  THREAD_ATOMIC_END;
}

extern int
regex_search(regex_t* reg, UChar* str, UChar* end,
	     UChar* start, UChar* range, RegRegion* region)
{
  int r;
  UChar *s, *prev;
  MatchStackAlloc msa;

  MATCH_STACK_ALLOC_INIT(msa);

  if (REG_STATE(reg) == REG_STATE_NORMAL) {
    regex_chain_reduce(reg);
  }
  reg->state++;  /* increment as search counter */

#ifdef REG_DEBUG_SEARCH
  fprintf(stderr, "regex_search (entry point): end: %d, start: %d, range: %d\n",
	  (int )(end - str), (int )(start - str), (int )(range - str));
#endif

  r = regex_region_resize(region, reg->max_mem + 1);
  if (r) goto finish;

  if (start > end || start < str) goto mismatch;

  /* anchor optimize: resume search range */
  if (reg->anchor != 0 && str < end) {
    UChar* semi_end;

    if (reg->anchor & ANCHOR_BEGIN_POSITION) {
      /* search start-position only */
    begin_position:
      if (range > start)
	range = start + 1;
      else
	range = start;
    }
    else if (reg->anchor & ANCHOR_BEGIN_BUF) {
      /* search str-position only */
      if (range > start) {
	if (start != str) goto mismatch;
	range = str + 1;
      }
      else {
	if (range <= str) {
	  start = str;
	  range = str;
	}
	else
	  goto mismatch;
      }
    }
    else if (reg->anchor & ANCHOR_END_BUF) {
      semi_end = end;

    end_buf:
      if (semi_end - str < reg->anchor_dmin)
	goto mismatch;

      if (range > start) {
	if (semi_end - start > reg->anchor_dmax) {
	  start = semi_end - reg->anchor_dmax;
	  if (start < end)
	    start = get_right_adjust_char_head(reg->code, str, start);
	  else { /* match with empty at end */
	    start = get_prev_char_head(reg->code, str, end);
	  }
	}
	if (semi_end - (range - 1) < reg->anchor_dmin) {
	  range = semi_end - reg->anchor_dmin + 1;
	}

	if (start >= range) goto mismatch;
      }
      else {
	if (semi_end - range > reg->anchor_dmax) {
	  range = semi_end - reg->anchor_dmax;
	}
	if (semi_end - start < reg->anchor_dmin) {
	  start = semi_end - reg->anchor_dmin;
	  start = get_left_adjust_char_head(reg->code, str, start);
	  if (range > start) goto mismatch;
	}
      }
    }
    else if (reg->anchor & ANCHOR_SEMI_END_BUF) {
      if (IS_NEWLINE(end[-1])) {
	semi_end = end - 1;
	if (semi_end > str && start <= semi_end) {
	  goto end_buf;
	}
      }
      else {
	semi_end = end;
	goto end_buf;
      }
    }
    else if ((reg->anchor & ANCHOR_ANYCHAR_STAR_PL)) {
      goto begin_position;
    }
  }

#ifdef REG_DEBUG_SEARCH
  fprintf(stderr, "regex_search(apply anchor): end: %d, start: %d, range: %d\n",
	  (int )(end - str), (int )(start - str), (int )(range - str));
#endif

#define MATCH_AND_RETURN_CHECK \
  r = match_at(reg, str, end, s, prev, region, &msa);\
  if (r != REG_MISMATCH) {\
    if (r >= 0) goto match;\
    goto finish; /* error */ \
  }

  s = start;
  if (range > start) {   /* forward search */
    if (s > str)
      prev = get_prev_char_head(reg->code, str, s);
    else
      prev = (UChar* )NULL;

    if (reg->optimize != REG_OPTIMIZE_NONE) {
      UChar *sch_range, *low, *high, *low_prev;

      sch_range = range;
      if (reg->dmax != 0) {
	if (reg->dmax == INFINITE_DISTANCE)
	  sch_range = end;
	else {
	  sch_range += reg->dmax;
	  if (sch_range > end) sch_range = end;
	}
      }
      if (reg->threshold_len != 0 && (end - start) >= reg->threshold_len) {
	do {
	  if (! forward_search_range(reg, str, end, s, sch_range,
				     &low, &high, &low_prev)) goto mismatch;
	  if (s < low) {
	    s    = low;
	    prev = low_prev;
	  }
	  while (s <= high) {
	    MATCH_AND_RETURN_CHECK;
	    prev = s;
	    s += mblen(reg->code, *s);
	  }
	  if ((reg->anchor & ANCHOR_ANYCHAR_STAR) != 0) {
	    if (IS_NOT_NULL(prev)) {
	      while (!IS_NEWLINE(*prev) && s < range) {
		prev = s;
		s += mblen(reg->code, *s);
	      }
	    }
	  }
	} while (s < range);
	goto mismatch;
      }
      else { /* check only. */
	if ((end - start) <= reg->dmin ||
	    ! forward_search_range(reg, str, end, s, sch_range,
				   &low, &high, (UChar** )NULL)) goto mismatch;
      }
    }

    do {
      MATCH_AND_RETURN_CHECK;
      prev = s;
      s += mblen(reg->code, *s);
    } while (s <= range);   /* exec s == range, because empty match with /$/. */
  }
  else {  /* backward search */
    if (reg->optimize != REG_OPTIMIZE_NONE) {
      UChar *low, *high, *adjrange, *sch_start;

      adjrange = get_left_adjust_char_head(reg->code, str, range);
      if (reg->threshold_len != 0 && (end - range) >= reg->threshold_len) {
	do {
	  sch_start = s + reg->dmax;
	  if (sch_start > end) sch_start = end;
	  if (backward_search_range(reg, str, end, sch_start, range, adjrange,
				    &low, &high) <= 0)
	    goto mismatch;

	  if (s > high)
	    s = high;

	  while (s >= low) {
	    prev = get_prev_char_head(reg->code, str, s);
	    MATCH_AND_RETURN_CHECK;
	    s = prev;
	  }
	} while (s >= range);
	goto mismatch;
      }
      else { /* check only. */
	if ((end - range) <= reg->dmin) goto mismatch;

	sch_start = s;
	if (reg->dmax != 0) {
	  if (reg->dmax == INFINITE_DISTANCE)
	    sch_start = end;
	  else {
	    sch_start += reg->dmax;
	    if (sch_start > end) sch_start = end;
	    else
	      sch_start = get_left_adjust_char_head(reg->code, start, sch_start);
	  }
	}
	if (backward_search_range(reg, str, end, sch_start, range, adjrange,
				  &low, &high) <= 0) goto mismatch;
      }
    }

    do {
      prev = get_prev_char_head(reg->code, str, s);
      MATCH_AND_RETURN_CHECK;
      s = prev;
    } while (s >= range);
  }

 mismatch:
  r = REG_MISMATCH;

 finish:
  regex_region_clear(region);
  reg->state--;  /* decrement as search counter */
  MATCH_STACK_ALLOC_FREE(msa);
  return r;

 match:
  reg->state--;  /* decrement as search counter */
  MATCH_STACK_ALLOC_FREE(msa);
  return s - str;
}

extern int
re_search(regex_t* bufp, const char* string, int size, int startpos, int range,
	  struct re_registers* regs)
{
  return regex_search(bufp, (UChar* )string, (UChar* )(string + size),
		      (UChar* )(string + startpos),
		      (UChar* )(string + startpos + range), regs);
}


extern int
regex_clone(regex_t* to, regex_t* from)
{
  int size;

  regex_free_body(to);
  xmemcpy(to, from, sizeof(regex_t));
  to->state = REG_STATE_NORMAL;

  if (from->p) {
    to->p = (UChar* )xmalloc(to->alloc);
    if (IS_NULL(to->p)) return REGERR_MEMORY;
    xmemcpy(to->p, from->p, to->alloc);
  }

  if (from->exact) {
    to->exact = (UChar* )xmalloc(from->exact_end - from->exact);
    if (IS_NULL(to->exact)) return REGERR_MEMORY;
    to->exact_end = to->exact + (from->exact_end - from->exact);
  }

  if (from->int_map) {
    size = sizeof(int) * REG_CHAR_TABLE_SIZE;
    to->int_map = (int* )xmalloc(size);
    if (IS_NULL(to->int_map)) return REGERR_MEMORY;
    xmemcpy(to->int_map, from->int_map, size);
  }

  if (from->int_map_backward) {
    size = sizeof(int) * REG_CHAR_TABLE_SIZE;
    to->int_map_backward = (int* )xmalloc(size);
    if (IS_NULL(to->int_map_backward)) return REGERR_MEMORY;
    xmemcpy(to->int_map_backward, from->int_map_backward, size);
  }

  return 0;
}

#ifdef REG_DEBUG_PARSE_TREE
static void print_tree(FILE* f, Node* node);
#endif

static int
regex_compile(regex_t* reg, UChar* pattern, UChar* pattern_end)
{
#define COMPILE_INIT_SIZE  20

  int r, init_size;
  Node* root;

  reg->state = REG_STATE_COMPILING;
  if (IS_IGNORECASE(reg->options) && IS_NULL(reg->transtable)) {
    r = REGERR_TABLE_FOR_IGNORE_CASE_IS_NOT_SETTED;
    goto end;
  }

  if (reg->alloc == 0) {
    init_size = (pattern_end - pattern) * 2;
    if (init_size <= 0) init_size = COMPILE_INIT_SIZE;
    r = BBUF_INIT(reg, init_size);
    if (r) goto end;
  }
  else
    reg->used = 0;

  reg->max_mem        = 0;
  reg->num_repeat     = 0;
  reg->num_null_check = 0;
  reg->mem_stats      = 0;

  r = parse_make_tree(&root, pattern, pattern_end, reg);
  if (r) goto err;

#ifdef REG_DEBUG_PARSE_TREE
  print_tree(stderr, root);
#endif

  r = setup_tree(root, reg, 0);
  if (r) goto err;

  clear_optimize_info(reg);
#ifndef REG_DONT_OPTIMIZE
  r = set_optimize_info_from_tree(root, reg);
  if (r) goto err;
#endif

  if (IS_FIND_CONDITION(reg->options))
    MEM_STATS_ON_ALL(reg->mem_stats);

  r = compile_tree(root, reg);
  if (r == 0) {
    r = add_opcode(reg, OP_END);
  }
  node_free(root);

#ifdef REG_DEBUG_COMPILE
  print_compiled_byte_code_list(stderr, reg);
#endif

 end:
  reg->state = REG_STATE_NORMAL;
  return r;

 err:
  if (root) node_free(root);
  return r;
}

extern int
regex_recompile(regex_t* reg, UChar* pattern, UChar* pattern_end,
		RegOptionType option, RegCharEncoding code, UChar* transtable)
{
  int r;
  regex_t *new_reg;

  r = regex_new(&new_reg, pattern, pattern_end, option, code, transtable);
  if (r) return r;
  if (REG_STATE(reg) == REG_STATE_NORMAL) {
    regex_transfer(reg, new_reg);
  }
  else {
    regex_chain_link_add(reg, new_reg);
  }
  return 0;
}

extern char*
re_compile_pattern(const char* pattern, int size, regex_t* reg)
{
  int r;

  r = regex_compile(reg, (UChar* )pattern, (UChar* )(pattern + size));
  if (r)
    return regex_error_code_to_str(r);

  return NULL;
}

extern char*
re_recompile_pattern(const char* pattern, int size, regex_t* reg)
{
  int r;

  r = regex_recompile(reg, (UChar* )pattern, (UChar* )(pattern + size),
		      reg->options, reg->code, reg->transtable);
  if (r)
    return regex_error_code_to_str(r);

  return NULL;
}

extern void
re_free_pattern(regex_t* reg)
{
  regex_free(reg);
}

static int regex_inited = 0;

static int
regex_alloc_init(regex_t** reg, RegOptionType option,
		 RegCharEncoding code, UChar* transtable)
{
  if (! regex_inited)
    regex_init();

  *reg = (regex_t* )xmalloc(sizeof(regex_t));
  if (IS_NULL(*reg)) return REGERR_MEMORY;

  if (IS_NULL(transtable) || transtable == REG_TRANSTABLE_USE_DEFAULT)
    transtable = DefaultTransTable;

  (*reg)->state            = REG_STATE_NORMAL;
  (*reg)->code             = code;
  (*reg)->options          = option;
  (*reg)->transtable       = transtable;
  (*reg)->optimize         = 0;
  (*reg)->exact            = (UChar* )NULL;
  (*reg)->int_map          = (int* )NULL;
  (*reg)->int_map_backward = (int* )NULL;
  (*reg)->chain            = (regex_t* )NULL;

  (*reg)->p                = (UChar* )NULL;
  (*reg)->alloc            = 0;
  (*reg)->used             = 0;

  return 0;
}

extern int
re_alloc_pattern(regex_t** reg)
{
  if (RegDefaultCharEncoding == REGCODE_UNDEF)
    return REGERR_DEFAULT_ENCODING_IS_NOT_SETTED;

  return regex_alloc_init(reg, REG_OPTION_DEFAULT, RegDefaultCharEncoding,
			  DefaultTransTable);
}

extern int
regex_new(regex_t** reg, UChar* pattern, UChar* pattern_end,
	  RegOptionType option, RegCharEncoding code, UChar* transtable)
{
  int r;

  r = regex_alloc_init(reg, option, code, transtable);
  if (r) return r;

  r = regex_compile(*reg, pattern, pattern_end);
  if (r) {
    regex_free(*reg);
    *reg = NULL;
  }
  return r;
}

static void
set_default_trans_table(UChar* table)
{
  int i;

  if (table && table != DefaultTransTable) {
    DefaultTransTable = table;

    for (i = 0; i < REG_CHAR_TABLE_SIZE; i++)
      AmbiguityTable[i] = 0;

    for (i = 0; i < REG_CHAR_TABLE_SIZE; i++) {
      AmbiguityTable[table[i]]++;
      if (table[i] != i)
	AmbiguityTable[i] += 2;
    }
  }
}

extern void
re_set_casetable(const char* table)
{
  set_default_trans_table((UChar* )table);
}

#ifdef USE_COMPATIBILITY_FOR_RUBY_EXTENSION_LIBRARY
static const unsigned char mbctab_ascii[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

static const unsigned char mbctab_euc[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
};

static const unsigned char mbctab_sjis[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0
};

static const unsigned char mbctab_utf8[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 0, 0,
};

const unsigned char *re_mbctab = mbctab_ascii;
#endif

extern void
#ifdef REG_RUBY_M17N
re_mbcinit(RegCharEncoding enc)
#else
re_mbcinit(int mb_code)
#endif
{
#ifdef REG_RUBY_M17N
  RegDefaultCharEncoding = enc;
#else
  RegDefaultCharEncoding = REG_MBLEN_TABLE[mb_code];
#endif

#ifdef USE_COMPATIBILITY_FOR_RUBY_EXTENSION_LIBRARY
  switch (mb_code) {
  case MBCTYPE_ASCII:
    re_mbctab = mbctab_ascii;
    break;
  case MBCTYPE_EUC:
    re_mbctab = mbctab_euc;
    break;
  case MBCTYPE_SJIS:
    re_mbctab = mbctab_sjis;
    break;
  case MBCTYPE_UTF8:
    re_mbctab = mbctab_utf8;
    break;
  }
#endif
}

extern int
regex_init()
{
#ifdef DEFAULT_TRANSTABLE_EXIST
  if (! DefaultTransTable)  /* check re_set_casetable() called already. */
    set_default_trans_table(DTT);
#endif

#ifdef REG_DEBUG_STATISTICS
  Init_REGEX_STAT();
  STATISTICS_INIT;
#endif

  regex_inited = 1;
  return 0;
}

extern int
regex_end()
{
#ifdef USE_RECYCLE_NODE
  FreeNode* n;

  while (FreeNodeList) {
    n = FreeNodeList;
    FreeNodeList = FreeNodeList->next;
    xfree(n);
  }
#endif

  regex_inited = 0;
  return 0;
}


#ifdef REG_DEBUG

typedef struct {
  short int opcode;
  char*     name;
  short int arg_type;
} OpInfoType;

static OpInfoType OpInfo[] = {
  { OP_FINISH,            "finish",          ARG_NON },
  { OP_END,               "end",             ARG_NON },
  { OP_EXACT1,            "exact1",          ARG_SPECIAL },
  { OP_EXACT2,            "exact2",          ARG_SPECIAL },
  { OP_EXACT3,            "exact3",          ARG_SPECIAL },
  { OP_EXACT4,            "exact4",          ARG_SPECIAL },
  { OP_EXACT5,            "exact5",          ARG_SPECIAL },
  { OP_EXACTN,            "exactn",          ARG_SPECIAL },
  { OP_EXACTMB2N1,        "exactmb2-n1",     ARG_SPECIAL },
  { OP_EXACTMB2N2,        "exactmb2-n2",     ARG_SPECIAL },
  { OP_EXACTMB2N3,        "exactmb2-n3",     ARG_SPECIAL },
  { OP_EXACTMB2N,         "exactmb2-n",      ARG_SPECIAL },
  { OP_EXACTMB3N,         "exactmb3n"  ,     ARG_SPECIAL },
  { OP_EXACTMBN,          "exactmbn",        ARG_SPECIAL },
  { OP_EXACT1_IC,         "exact1-ic",       ARG_SPECIAL },
  { OP_EXACTN_IC,         "exactn-ic",       ARG_SPECIAL },
  { OP_CCLASS,            "cclass",          ARG_SPECIAL },
  { OP_CCLASS_MB,         "cclass-mb",       ARG_SPECIAL },
  { OP_CCLASS_MIX,        "cclass-mix",      ARG_SPECIAL },
  { OP_CCLASS_NOT,        "cclass-not",      ARG_SPECIAL },
  { OP_CCLASS_MB_NOT,     "cclass-mb-not",   ARG_SPECIAL },
  { OP_CCLASS_MIX_NOT,    "cclass-mix-not",  ARG_SPECIAL },
  { OP_ANYCHAR,           "anychar",         ARG_NON },
  { OP_ANYCHAR_STAR,      "anychar*",        ARG_NON },
  { OP_ANYCHAR_STAR_PEEK_NEXT, "anychar*-peek-next", ARG_SPECIAL },
  { OP_WORD,              "word",            ARG_NON },
  { OP_NOT_WORD,          "not-word",        ARG_NON },
  { OP_WORD_SB,           "word-sb",         ARG_NON },
  { OP_WORD_MB,           "word-mb",         ARG_NON },
  { OP_WORD_BOUND,        "word-bound",      ARG_NON },
  { OP_NOT_WORD_BOUND,    "not-word-bound",  ARG_NON },
  { OP_WORD_BEGIN,        "word-begin",      ARG_NON },
  { OP_WORD_END,          "word-end",        ARG_NON },
  { OP_BEGIN_BUF,         "begin-buf",       ARG_NON },
  { OP_END_BUF,           "end-buf",         ARG_NON },
  { OP_BEGIN_LINE,        "begin-line",      ARG_NON },
  { OP_END_LINE,          "end-line",        ARG_NON },
  { OP_SEMI_END_BUF,      "semi-end-buf",    ARG_NON },
  { OP_BEGIN_POSITION,    "begin-position",  ARG_NON },
  { OP_BACKREF1,          "backref1",        ARG_NON },
  { OP_BACKREF2,          "backref2",        ARG_NON },
  { OP_BACKREF3,          "backref3",        ARG_NON },
  { OP_BACKREFN,          "backrefn",        ARG_MEMNUM  },
  { OP_MEMORY_START_PUSH, "mem-start_push",  ARG_MEMNUM  },
  { OP_MEMORY_END_PUSH,   "mem-end_push",    ARG_MEMNUM  },
  { OP_MEMORY_END,        "mem-end",         ARG_MEMNUM  },
  { OP_SET_OPTION_PUSH,   "set-option-push", ARG_OPTION  },
  { OP_SET_OPTION,        "set-option",      ARG_OPTION  },
  { OP_FAIL,              "fail",            ARG_NON },
  { OP_JUMP,              "jump",            ARG_RELADDR },
  { OP_PUSH,              "push",            ARG_RELADDR },
  { OP_PUSH_OR_JUMP_EXACT1, "push-or-jump-e1", ARG_SPECIAL },
  { OP_PUSH_IF_PEEK_NEXT, "push-if-peek-next", ARG_SPECIAL },
  { OP_REPEAT,            "repeat",          ARG_SPECIAL },
  { OP_REPEAT_NG,         "repeat-ng",       ARG_SPECIAL },
  { OP_REPEAT_INC,        "repeat-inc",      ARG_MEMNUM  },
  { OP_REPEAT_INC_NG,     "repeat-inx-ng",   ARG_MEMNUM  },
  { OP_NULL_CHECK_START,  "null-check-start",ARG_MEMNUM  },
  { OP_NULL_CHECK_END,    "null-check-end",  ARG_MEMNUM  },
  { OP_PUSH_POS,          "push-pos",        ARG_NON },
  { OP_POP_POS,           "pop-pos",         ARG_NON },
  { OP_PUSH_POS_NOT,      "push-pos-not",    ARG_RELADDR },
  { OP_FAIL_POS,          "fail-pos",        ARG_NON },
  { OP_PUSH_STOP_BT,      "push-stop-bt",    ARG_NON },
  { OP_POP_STOP_BT,       "pop-stop-bt",     ARG_NON },
  { OP_LOOK_BEHIND,       "look-behind",     ARG_SPECIAL },
  { OP_PUSH_LOOK_BEHIND_NOT, "push-look-behind-not", ARG_SPECIAL },
  { OP_FAIL_LOOK_BEHIND_NOT, "fail-look-behind-not", ARG_NON }, 
  { -1, "", ARG_NON }
};

static char*
op2name(int opcode)
{
  int i;

  for (i = 0; OpInfo[i].opcode >= 0; i++) {
    if (opcode == OpInfo[i].opcode)
      return OpInfo[i].name;
  }
  return "";
}

static int
op2arg_type(int opcode)
{
  int i;

  for (i = 0; OpInfo[i].opcode >= 0; i++) {
    if (opcode == OpInfo[i].opcode)
      return OpInfo[i].arg_type;
  }
  return ARG_SPECIAL;
}

static void
Indent(FILE* f, int indent)
{
  int i;
  for (i = 0; i < indent; i++) putc(' ', f);
}

static void
p_string(FILE* f, int len, UChar* s)
{
  fputs(":", f);
  while (len-- > 0) { fputc(*s++, f); }
}

static void
p_len_string(FILE* f, LengthType len, int mb_len, UChar* s)
{
  int x = len * mb_len;

  fprintf(f, ":%d:", len);
  while (x-- > 0) { fputc(*s++, f); }
}

static void
print_compiled_byte_code(FILE* f, UChar* bp, UChar** nextp)
{
  int i, n, arg_type;
  RelAddrType addr;
  LengthType len;
  MemNumType mem;
  WCInt wc;
  UChar *q;

  fprintf(f, "[%s", op2name(*bp));
  arg_type = op2arg_type(*bp);
  if (arg_type != ARG_SPECIAL) {
    bp++;
    switch (arg_type) {
    case ARG_NON:
      break;
    case ARG_RELADDR:
      addr = *((RelAddrType* )bp);
      bp += SIZE_RELADDR;
      fprintf(f, ":(%d)", addr);
      break;
    case ARG_LENGTH:
      GET_LENGTH_INC(len, bp);
      fprintf(f, ":%d", len);
      break;
    case ARG_MEMNUM:
      mem = *((MemNumType* )bp);
      bp += SIZE_MEMNUM;
      fprintf(f, ":%d", mem);
      break;
    case ARG_OPTION:
      {
	RegOptionType option = *((RegOptionType* )bp);
	bp += SIZE_OPTION;
	fprintf(f, ":%d", option);
      }
      break;
    }
  }
  else {
    switch (*bp++) {
    case OP_EXACT1:
    case OP_ANYCHAR_STAR_PEEK_NEXT:
      p_string(f, 1, bp++); break;
    case OP_EXACT2:
      p_string(f, 2, bp); bp += 2; break;
    case OP_EXACT3:
      p_string(f, 3, bp); bp += 3; break;
    case OP_EXACT4:
      p_string(f, 4, bp); bp += 4; break;
    case OP_EXACT5:
      p_string(f, 5, bp); bp += 5; break;
    case OP_EXACTN:
      GET_LENGTH_INC(len, bp);
      p_len_string(f, len, 1, bp);
      bp += len;
      break;
    
    case OP_EXACTMB2N1:
      p_string(f, 2, bp); bp += 2; break;
    case OP_EXACTMB2N2:
      p_string(f, 4, bp); bp += 4; break;
    case OP_EXACTMB2N3:
      p_string(f, 6, bp); bp += 6; break;
    case OP_EXACTMB2N:
      GET_LENGTH_INC(len, bp);
      p_len_string(f, len, 2, bp);
      bp += len * 2;
      break;
    case OP_EXACTMB3N:
      GET_LENGTH_INC(len, bp);
      p_len_string(f, len, 3, bp);
      bp += len * 3;
      break;
    case OP_EXACTMBN:
      {
	int mb_len;
      
	GET_LENGTH_INC(mb_len, bp);
	GET_LENGTH_INC(len, bp);
	fprintf(f, ":%d:%d:", mb_len, len);
	n = len * mb_len;
	while (n-- > 0) { fputc(*bp++, f); }
      }
      break;

    case OP_EXACT1_IC:
      p_string(f, 1, bp++);
      break;
    case OP_EXACTN_IC:
      GET_LENGTH_INC(len, bp);
      p_len_string(f, len, 1, bp);
      bp += len;
      break;

    case OP_CCLASS:
      n = bitset_on_num((BitSetRef )bp);
      bp += SIZE_BITSET;
      fprintf(f, ":%d", n);
      break;

    case OP_CCLASS_NOT:
      n = bitset_on_num((BitSetRef )bp);
      bp += SIZE_BITSET;
      fprintf(f, ":%d", n);
      break;

    case OP_CCLASS_MB:
    case OP_CCLASS_MB_NOT:
      n = bitset_on_num((BitSetRef )bp);
      bp += SIZE_BITSET;
      GET_LENGTH_INC(len, bp);
      q = bp;
#ifndef UNALIGNED_WORD_ACCESS
      ALIGNMENT_RIGHT(q);
#endif
      GET_WCINT(wc, q);
      bp += len;
      fprintf(f, ":%d:%d:%d", n, (int )wc, len);
      break;

    case OP_CCLASS_MIX:
    case OP_CCLASS_MIX_NOT:
      n = bitset_on_num((BitSetRef )bp);
      bp += SIZE_BITSET;
      GET_LENGTH_INC(len, bp);
      q = bp;
#ifndef UNALIGNED_WORD_ACCESS
      ALIGNMENT_RIGHT(q);
#endif
      GET_WCINT(wc, q);
      bp += len;
      fprintf(f, ":%d:%d:%d", n, (int )wc, len);
      break;
  
    case OP_REPEAT:
    case OP_REPEAT_NG:
      {
	RepeatNumType nf, nt;

	mem = *((MemNumType* )bp);
	bp += SIZE_MEMNUM;
	addr = *((RelAddrType* )bp);
	bp += SIZE_RELADDR;
	nf = *((RepeatNumType* )bp);
	bp += SIZE_REPEATNUM;
	nt = *((RepeatNumType* )bp);
	bp += SIZE_REPEATNUM;
	fprintf(f, ":%d:%d:%d-", mem, addr, nf);
	if (IS_REPEAT_INFINITE(nt))
	  fprintf(f, "*");
	else
	  fprintf(f, "%d", nt);
      }
      break;

    case OP_PUSH_OR_JUMP_EXACT1:
    case OP_PUSH_IF_PEEK_NEXT:
      addr = *((RelAddrType* )bp);
      bp += SIZE_RELADDR;
      fprintf(f, ":(%d)", addr);
      p_string(f, 1, bp);
      bp += 1;
      break;

    case OP_LOOK_BEHIND:
      GET_LENGTH_INC(len, bp);
      fprintf(f, ":%d", len);
      break;

    case OP_PUSH_LOOK_BEHIND_NOT:
      GET_RELADDR_INC(addr, bp);
      GET_LENGTH_INC(len, bp);
      fprintf(f, ":%d:(%d)", len, addr);
      break;

    default:
      fprintf(stderr, "print_compiled_byte_code: undefined code %d\n", *--bp);
    }
  }
  fputs("]", f);
  if (nextp) *nextp = bp;
}

static void
print_compiled_byte_code_list(FILE* f, regex_t* reg)
{
  int ncode;
  UChar* bp = reg->p;
  UChar* end = reg->p + reg->used;

  fprintf(f, "code length: %d\n", reg->used);

  ncode = 0;
  while (bp < end) {
    ncode++;
    if (bp > reg->p) {
      if (ncode % 5 == 0)
	fprintf(f, "\n");
      else
	fputs(" ", f);
    }
    print_compiled_byte_code(f, bp, &bp);
  }

  fprintf(f, "\n");
}

static void
print_indent_tree(FILE* f, Node* node, int indent)
{
  int i, type;
  int add = 3;
  UChar* p;

  Indent(f, indent);
  if (IS_NULL(node)) {
    fprintf(f, "ERROR: null node!!!\n");
    exit (0);
  }

  type = NTYPE(node);
  switch (type) {
  case N_EMPTY:
    fprintf(f, "<empty>");
    break;
  case N_LIST:
  case N_ALT:
    if (NTYPE(node) == N_LIST)
      fprintf(f, "<list>\n");
    else
      fprintf(f, "<alt>\n");

    print_indent_tree(f, NCONS(node).left, indent + add);
    while (IS_NOT_NULL(node = NCONS(node).right)) {
      if (NTYPE(node) != type) {
	fprintf(f, "ERROR: list/alt right is not a cons. %d\n", NTYPE(node));
	exit(0);
      }
      print_indent_tree(f, NCONS(node).left, indent + add);
    }
    break;

  case N_STRING:
    fprintf(f, "<string>");
    for (p = NSTRING(node).s; p < NSTRING(node).end; p++) fputc(*p, f);
    break;
  case N_STRING_RAW:
    fprintf(f, "<string-raw>");
    for (p = NSTRING(node).s; p < NSTRING(node).end; p++) fputc(*p, f);
    break;

  case N_CCLASS:
    fprintf(f, "<cclass>");
    if (NCCLASS(node).not) fputs(" not", f);
    if (NCCLASS(node).mbuf) {
      BBuf* bbuf = NCCLASS(node).mbuf;
      for (i = 0; i < bbuf->used; i++) {
	if (i > 0) fprintf(f, ",");
	fprintf(f, "%0x", bbuf->p[i]);
      }
    }
#if 0
    fprintf(f, "\n");
    Indent(f, indent);
    for (i = 0; i < SINGLE_BYTE_SIZE; i++)
      fputc((BITSET_AT(NCCLASS(node).bs, i) ? '1' : '0'), f);
#endif
    break;

  case N_CTYPE:
    fprintf(f, "<ctype> ");
    switch (NCTYPE(node).type) {
    case CTYPE_WORD:            fputs("word",           f); break;
    case CTYPE_NOT_WORD:        fputs("not word",       f); break;
    case CTYPE_WORD_BOUND:      fputs("word bound",     f); break;
    case CTYPE_NOT_WORD_BOUND:  fputs("not word bound", f); break;
#ifdef USE_WORD_BEGIN_END
    case CTYPE_WORD_BEGIN:      fputs("word begin", f);     break;
    case CTYPE_WORD_END:        fputs("word end", f);       break;
#endif
#ifdef USE_ONIGURUMA_EXTENSION
    case CTYPE_WORD_SB:         fputs("word-sb",        f); break;
    case CTYPE_WORD_MB:         fputs("word-mb",        f); break;
#endif
    default:
      fprintf(f, "ERROR: undefined ctype.\n");
      exit(0);
    }
    break;

  case N_ANYCHAR:
    fprintf(f, "<anychar>");
    break;

  case N_ANCHOR:
    fprintf(f, "<anchor> ");
    switch (NANCHOR(node).type) {
    case ANCHOR_BEGIN_BUF:      fputs("begin buf",      f); break;
    case ANCHOR_END_BUF:        fputs("end buf",        f); break;
    case ANCHOR_BEGIN_LINE:     fputs("begin line",     f); break;
    case ANCHOR_END_LINE:       fputs("end line",       f); break;
    case ANCHOR_SEMI_END_BUF:   fputs("semi end buf",   f); break;
    case ANCHOR_BEGIN_POSITION: fputs("begin position", f); break;
    default:
      fprintf(f, "ERROR: undefined anchor type.\n");
      break;
    }
    break;

  case N_BACKREF:
    fprintf(f, "<backref>%d", NBACKREF(node).regnum);
    break;
  case N_QUALIFIER:
    fprintf(f, "<qualifier>{%d,%d}%s\n",
	    NQUALIFIER(node).lower, NQUALIFIER(node).upper,
	    (NQUALIFIER(node).greedy ? "" : "?"));
    print_indent_tree(f, NQUALIFIER(node).target, indent + add);
    break;
  case N_OPTION:
    fprintf(f, "<option>%d\n", NOPTION(node).option);
    print_indent_tree(f, NOPTION(node).target, indent + add);
    break;
  case N_EFFECT:
    fprintf(f, "<effect> ");
    switch (NEFFECT(node).type) {
    case EFFECT_MEMORY:
      fprintf(f, "memory:%d", NEFFECT(node).regnum);
      break;
    case EFFECT_PREC_READ:       fputs("prec read",      f); break;
    case EFFECT_PREC_READ_NOT:   fputs("prec read not",  f); break;
    case EFFECT_STOP_BACKTRACK:  fputs("stop backtrack", f); break;
    case EFFECT_LOOK_BEHIND:     fputs("look_behind",    f); break;
    case EFFECT_LOOK_BEHIND_NOT: fputs("look_behind_not",f); break;
    default:
      break;
    }
    fprintf(f, "\n");
    print_indent_tree(f, NEFFECT(node).target, indent + add);
    break;

  default:
    fprintf(f, "print_indent_tree: undefined node type %d\n", NTYPE(node));
    break;
  }

  if (type != N_LIST && type != N_ALT && type != N_QUALIFIER &&
      type != N_OPTION && type != N_EFFECT)
    fprintf(f, "\n");
  fflush(f);
}

static void
print_tree(FILE* f, Node* node)
{
  print_indent_tree(f, node, 0);
}

#endif /* REG_DEBUG */

#ifdef REG_DEBUG_STATISTICS
static void
print_statistics(FILE* f)
{
  int i;
  fprintf(f, "count     prev      time\n");
  for (i = 0; OpInfo[i].opcode >= 0; i++) {
    fprintf(f, "%8d: %8d: %10d: %s\n",
	    OpCounter[i], OpPrevCounter[i], OpTime[i], OpInfo[i].name);
  }
  fprintf(f, "\nmax stack depth: %d\n", MaxStackDepth);
}
#endif
