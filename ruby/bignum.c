/**********************************************************************

  bignum.c -

  $Author$
  $Date$
  created at: Fri Jun 10 00:48:55 JST 1994

  Copyright (C) 1993-2000 Yukihiro Matsumoto

**********************************************************************/

#include "ruby.h"
#include <math.h>
#include <ctype.h>

VALUE rb_cBignum;
typedef unsigned short USHORT;

#define BDIGITS(x) ((USHORT*)RBIGNUM(x)->digits)
#define BITSPERDIG (sizeof(short)*CHAR_BIT)
#define BIGRAD (1L << BITSPERDIG)
#define DIGSPERLONG ((unsigned int)(sizeof(long)/sizeof(short)))
#define BIGUP(x) ((unsigned long)(x) << BITSPERDIG)
#define BIGDN(x) RSHIFT(x,BITSPERDIG)
#define BIGLO(x) ((USHORT)((x) & (BIGRAD-1)))

static VALUE
bignew_1(klass, len, sign)
    VALUE klass;
    long len;
    char sign;
{
    NEWOBJ(big, struct RBignum);
    OBJSETUP(big, klass, T_BIGNUM);
    big->sign = sign;
    big->len = len;
    big->digits = ALLOC_N(USHORT, len);

    return (VALUE)big;
}

#define bignew(len,sign) bignew_1(rb_cBignum,len,sign)

VALUE
rb_big_clone(x)
    VALUE x;
{
    VALUE z = bignew_1(CLASS_OF(x), RBIGNUM(x)->len, RBIGNUM(x)->sign);

    MEMCPY(BDIGITS(z), BDIGITS(x), USHORT, RBIGNUM(x)->len);
    return z;
}

void
rb_big_2comp(x)			/* get 2's complement */
    VALUE x;
{
    long i = RBIGNUM(x)->len;
    USHORT *ds = BDIGITS(x);
    long num;

    while (i--) ds[i] = ~ds[i];
    i = 0; num = 1;
    do {
	num += ds[i];
	ds[i++] = BIGLO(num);
	num = BIGDN(num);
    } while (i < RBIGNUM(x)->len);
    if (ds[0] == 1 || ds[0] == 0) {
	for (i=1; i<RBIGNUM(x)->len; i++) {
	    if (ds[i] != 0) return;
	}
	REALLOC_N(RBIGNUM(x)->digits, USHORT, RBIGNUM(x)->len++);
	ds = BDIGITS(x);
	ds[RBIGNUM(x)->len-1] = 1;
    }
}

static VALUE
bignorm(x)
    VALUE x;
{
    long len = RBIGNUM(x)->len;
    USHORT *ds = BDIGITS(x);

    while (len-- && !ds[len]) ;
    RBIGNUM(x)->len = ++len;

    if (len*sizeof(USHORT) <= sizeof(VALUE)) {
	long num = 0;
	while (len--) {
	    num = BIGUP(num) + ds[len];
	}
	if (num >= 0) {
	    if (RBIGNUM(x)->sign) {
		if (POSFIXABLE(num)) return INT2FIX(num);
	    }
	    else if (NEGFIXABLE(-(long)num)) return INT2FIX(-(long)num);
	}
    }
    return x;
}

VALUE
rb_big_norm(x)
    VALUE x;
{
    return bignorm(x);
}

VALUE
rb_uint2big(n)
    unsigned long n;
{
    unsigned int i = 0;
    USHORT *digits;
    VALUE big;

    i = 0;
    big = bignew(DIGSPERLONG, 1);
    digits = BDIGITS(big);
    while (i < DIGSPERLONG) {
	digits[i++] = BIGLO(n);
	n = BIGDN(n);
    }

    i = DIGSPERLONG;
    while (i-- && !digits[i]) ;
    RBIGNUM(big)->len = i+1;
    return big;
}

VALUE
rb_int2big(n)
    long n;
{
    long neg = 0;
    VALUE big;

    if (n < 0) {
	n = -n;
	neg = 1;
    }
    big = rb_uint2big(n);
    if (neg) {
	RBIGNUM(big)->sign = 0;
    }
    return big;
}

VALUE
rb_uint2inum(n)
    unsigned long n;
{
    if (POSFIXABLE(n)) return INT2FIX(n);
    return rb_uint2big(n);
}

VALUE
rb_int2inum(n)
    long n;
{
    if (FIXABLE(n)) return INT2FIX(n);
    return rb_int2big(n);
}

VALUE
rb_cstr2inum(str, base)
    const char *str;
    int base;
{
    const char *s = str;
    char *end;
    int badcheck = (base==0)?1:0;
    char sign = 1, c;
    unsigned long num;
    long len, blen = 1;
    long i;
    VALUE z;
    USHORT *zds;

    while (*str && ISSPACE(*str)) str++;

    if (*str == '+') {
	str++;
    }
    else if (*str == '-') {
	str++;
	sign = 0;
    }
    if (base == 0) {
	if (*str == '0') {
	    str++;
	    if (*str == 'x' || *str == 'X') {
		str++;
		base = 16;
	    }
	    else if (*str == 'b' || *str == 'B') {
		str++;
		base = 2;
	    }
	    else {
		base = 8;
		if (!*str) return INT2FIX(0);
	    }
	}
	else {
	    base = 10;
	    if (!*str) return INT2FIX(0);
	}
    }
    if (base == 8) {
	while (str[0] == '0') str++;
	len = 3*strlen(str)*sizeof(char);
    }
    else {			/* base == 10, 2 or 16 */
	if (base == 16 && str[0] == '0' && (str[1] == 'x'||str[1] == 'X')) {
	    str += 2;
	}
	if (base == 2 && str[0] == '0' && (str[1] == 'b'||str[1] == 'B')) {
	    str += 2;
	}
	while (*str && *str == '0') str++;
	if (!*str) str--;
	len = 4*strlen(str)*sizeof(char);
    }

    if (len <= (sizeof(VALUE)*CHAR_BIT)) {
	unsigned long val = strtoul((char*)str, &end, base);

	if (badcheck) {
	    if (end == str || *end)
		goto bad;
	    while (*end && ISSPACE(*end)) end++;
	    if (*end) {
	      bad:
		rb_raise(rb_eArgError, "invalid literal for Integer: %s", s);
	    }
	}

	if (POSFIXABLE(val)) {
	    if (sign) return INT2FIX(val);
	    else {
		long result = -(long)val;
		return INT2FIX(result);
	    }
	}
	else {
	    VALUE big = rb_uint2big(val);
	    RBIGNUM(big)->sign = sign;
	    return big;
	}
    }
    len = (len/BITSPERDIG)+1;

    z = bignew(len, sign);
    zds = BDIGITS(z);
    for (i=len;i--;) zds[i]=0;
    while (c = *str++) {
	switch (c) {
	  case '0': case '1': case '2': case '3': case '4':
	  case '5': case '6': case '7': case '8': case '9':
	    c = c - '0';
	    break;
	  case 'a': case 'b': case 'c':
	  case 'd': case 'e': case 'f':
	    c = c - 'a' + 10;
	    break;
	  case 'A': case 'B': case 'C':
	  case 'D': case 'E': case 'F':
	    c = c - 'A' + 10;
	    break;
	  default:
	    c = base;
	    if (badcheck) {
		if (ISSPACE(c)) {
		    while (*str && ISSPACE(*str)) str++;
		    if (*str) {
			break;
		    }
		}
		rb_raise(rb_eArgError, "invalid literal for Integer: %s", s);
	    }
	    break;
	}
	if (c >= base) break;
	i = 0;
	num = c;
	for (;;) {
	    while (i<blen) {
		num += zds[i]*base;
		zds[i++] = BIGLO(num);
		num = BIGDN(num);
	    }
	    if (num) {
		blen++;
		continue;
	    }
	    break;
	}
    }

    return bignorm(z);
}

VALUE
rb_str2inum(str, base)
    VALUE str;
    int base;
{
    char *s;
    int len;

    s = rb_str2cstr(str, &len);
    if (len != strlen(s)) {
	rb_raise(rb_eArgError, "string for Integer contains null byte");
    }
    return rb_cstr2inum(s, base); 
}

static char hexmap[] = "0123456789abcdef";
VALUE
rb_big2str(x, base)
    VALUE x;
    int base;
{
    VALUE t;
    USHORT *ds;
    unsigned long i, j, hbase;
    VALUE ss;
    char *s, c;

    if (FIXNUM_P(x)) {
	return rb_fix2str(x, base);
    }
    i = RBIGNUM(x)->len;
    if (i == 0) return rb_str_new2("0");
    if (base == 10) {
	j = (sizeof(USHORT)/sizeof(char)*CHAR_BIT*i*241L)/800+2;
	hbase = 10000;
    }
    else if (base == 16) {
	j = (sizeof(USHORT)/sizeof(char)*CHAR_BIT*i)/4+2;
	hbase = 0x10000;
    }
    else if (base == 8) {
	j = (sizeof(USHORT)/sizeof(char)*CHAR_BIT*i)+2;
	hbase = 010000;
    }
    else if (base == 2) {
	j = (sizeof(USHORT)*CHAR_BIT*i)+2;
	hbase = 020;
    }
    else {
	j = 0;
	hbase = 0;
	rb_raise(rb_eArgError, "bignum cannot treat base %d", base);
    }

    t = rb_big_clone(x);
    ds = BDIGITS(t);
    ss = rb_str_new(0, j);
    s = RSTRING(ss)->ptr;

    s[0] = RBIGNUM(x)->sign ? '+' : '-';
    while (i && j) {
	long k = i;
	unsigned long num = 0;
	while (k--) {
	    num = BIGUP(num) + ds[k];
	    ds[k] = (USHORT)(num / hbase);
	    num %= hbase;
	}
	if (ds[i-1] == 0) i--;
	k = 4;
	while (k--) {
	    c = (char)(num % base);
	    s[--j] = hexmap[(int)c];
	    num /= base;
	    if (i == 0 && num == 0) break;
	}
    }
    while (s[j] == '0') j++;
    RSTRING(ss)->len -= RBIGNUM(x)->sign?j:j-1;
    memmove(RBIGNUM(x)->sign?s:s+1, s+j, RSTRING(ss)->len);
    s[RSTRING(ss)->len] = '\0';

    return ss;
}

static VALUE
rb_big_to_s(x)
    VALUE x;
{
    return rb_big2str(x, 10);
}

static unsigned long
big2ulong(x, type)
    VALUE x;
    char *type;
{
    unsigned long num;
    long len = RBIGNUM(x)->len;
    USHORT *ds;

    if (len > sizeof(long)/sizeof(USHORT))
	rb_raise(rb_eRangeError, "bignum too big to convert into `%s'", type);
    ds = BDIGITS(x);
    num = 0;
    while (len--) {
	num = BIGUP(num);
	num += ds[len];
    }
    return num;
}

unsigned long
rb_big2ulong(x)
    VALUE x;
{
    unsigned long num = big2ulong(x, "unsigned long");

    if (!RBIGNUM(x)->sign) return -num;
    return num;
}

long
rb_big2long(x)
    VALUE x;
{
    unsigned long num = big2ulong(x, "int");

    if ((long)num < 0) {
	rb_raise(rb_eRangeError, "bignum too big to convert into `int'");
    }
    if (!RBIGNUM(x)->sign) return -(long)num;
    return num;
}

static VALUE
rb_big_to_i(x)
    VALUE x;
{
    return bignorm(x);
}

VALUE
rb_dbl2big(d)
    double d;
{
    unsigned long i = 0;
    long c;
    USHORT *digits;
    VALUE z;
    double u = (d < 0)?-d:d;

    if (isinf(d)) {
	rb_raise(rb_eFloatDomainError, d < 0 ? "-Infinity" : "Infinity");
    }
    if (isnan(d)) {
	rb_raise(rb_eFloatDomainError, "NaN");
    }

    while (!POSFIXABLE(u) || 0 != (long)u) {
	u /= (double)(BIGRAD);
	i++;
    }
    z = bignew(i, d>=0);
    digits = BDIGITS(z);
    while (i--) {
	u *= BIGRAD;
	c = (long)u;
	u -= c;
	digits[i] = (USHORT)c;
    }

    return bignorm(z);
}

double
rb_big2dbl(x)
    VALUE x;
{
    double d = 0.0;
    long i = RBIGNUM(x)->len;
    USHORT *ds = BDIGITS(x);

    while (i--) {
	d = ds[i] + BIGRAD*d;
    }
    if (!RBIGNUM(x)->sign) d = -d;
    return d;
}

static VALUE
rb_big_to_f(x)
    VALUE x;
{
    return rb_float_new(rb_big2dbl(x));
}

static VALUE
rb_big_cmp(x, y)
    VALUE x, y;
{
    long xlen = RBIGNUM(x)->len;

    switch (TYPE(y)) {
      case T_FIXNUM:
	y = rb_int2big(FIX2LONG(y));
	break;

      case T_BIGNUM:
	break;

      default:
	return rb_num_coerce_bin(x, y);
    }

    if (RBIGNUM(x)->sign > RBIGNUM(y)->sign) return INT2FIX(1);
    if (RBIGNUM(x)->sign < RBIGNUM(y)->sign) return INT2FIX(-1);
    if (xlen < RBIGNUM(y)->len)
	return (RBIGNUM(x)->sign) ? INT2FIX(-1) : INT2FIX(1);
    if (xlen > RBIGNUM(y)->len)
	return (RBIGNUM(x)->sign) ? INT2FIX(1) : INT2FIX(-1);

    while(xlen-- && (BDIGITS(x)[xlen]==BDIGITS(y)[xlen]));
    if (-1 == xlen) return INT2FIX(0);
    return (BDIGITS(x)[xlen] > BDIGITS(y)[xlen]) ?
	(RBIGNUM(x)->sign ? INT2FIX(1) : INT2FIX(-1)) :
	    (RBIGNUM(x)->sign ? INT2FIX(-1) : INT2FIX(1));
}

static VALUE
rb_big_eq(x, y)
    VALUE x, y;
{
    if (rb_big_cmp(x, y) == INT2FIX(0)) return Qtrue;
    return Qfalse;
}

static VALUE
rb_big_uminus(x)
    VALUE x;
{
    VALUE z = rb_big_clone(x);

    RBIGNUM(z)->sign = !RBIGNUM(x)->sign;

    return bignorm(z);
}

static VALUE
rb_big_neg(x)
    VALUE x;
{
    VALUE z = rb_big_clone(x);
    long i = RBIGNUM(x)->len;
    USHORT *ds = BDIGITS(z);

    if (!RBIGNUM(x)->sign) rb_big_2comp(z);
    while (i--) ds[i] = ~ds[i];
    if (RBIGNUM(x)->sign) rb_big_2comp(z);
    RBIGNUM(z)->sign = !RBIGNUM(z)->sign;

    return bignorm(z);
}

static VALUE
bigsub(x, y)
    VALUE x, y;
{
    VALUE z = 0;
    USHORT *zds;
    long num;
    long i;

    i = RBIGNUM(x)->len;
    /* if x is larger than y, swap */
    if (RBIGNUM(x)->len < RBIGNUM(y)->len) {
	z = x; x = y; y = z;	/* swap x y */
    }
    else if (RBIGNUM(x)->len == RBIGNUM(y)->len) {
	while (i > 0) {
	    i--;
	    if (BDIGITS(x)[i] > BDIGITS(y)[i]) {
		break;
	    }
	    if (BDIGITS(x)[i] < BDIGITS(y)[i]) {
		z = x; x = y; y = z;	/* swap x y */
		break;
	    }
	}
    }

    z = bignew(RBIGNUM(x)->len, (z == 0)?1:0);
    zds = BDIGITS(z);

    for (i = 0, num = 0; i < RBIGNUM(y)->len; i++) { 
	num += (long)BDIGITS(x)[i] - BDIGITS(y)[i];
	zds[i] = BIGLO(num);
	num = BIGDN(num);
    } 
    while (num && i < RBIGNUM(x)->len) {
	num += BDIGITS(x)[i];
	zds[i++] = BIGLO(num);
	num = BIGDN(num);
    }
    while (i < RBIGNUM(x)->len) {
	zds[i] = BDIGITS(x)[i];
	i++;
    }
    
    return bignorm(z);
}

static VALUE
bigadd(x, y, sign)
    VALUE x, y;
    char sign;
{
    VALUE z;
    long num;
    long i, len;

    sign = (sign == RBIGNUM(y)->sign);
    if (RBIGNUM(x)->sign != sign) {
	if (sign) return bigsub(y, x);
	return bigsub(x, y);
    }

    if (RBIGNUM(x)->len > RBIGNUM(y)->len) {
	len = RBIGNUM(x)->len + 1;
        z = x; x = y; y = z;
    }
    else {
	len = RBIGNUM(y)->len + 1;
    }
    z = bignew(len, sign);

    len = RBIGNUM(x)->len;
    for (i = 0, num = 0; i < len; i++) {
	num += BDIGITS(x)[i] + BDIGITS(y)[i];
	BDIGITS(z)[i] = BIGLO(num);
	num = BIGDN(num);
    }
    len = RBIGNUM(y)->len;
    while (num && i < len) {
	num += BDIGITS(y)[i];
	BDIGITS(z)[i++] = BIGLO(num);
	num = BIGDN(num);
    }
    while (i < len) {
	BDIGITS(z)[i] = BDIGITS(y)[i];
	i++;
    }
    BDIGITS(z)[i] = (USHORT)num;

    return bignorm(z);
}

VALUE
rb_big_plus(x, y)
    VALUE x, y;
{
    switch (TYPE(y)) {
      case T_FIXNUM:
	y = rb_int2big(FIX2LONG(y));
	/* fall through */
      case T_BIGNUM:
	return bigadd(x, y, 1);

      case T_FLOAT:
	return rb_float_new(rb_big2dbl(x) + RFLOAT(y)->value);

      default:
	return rb_num_coerce_bin(x, y);
    }
}

VALUE
rb_big_minus(x, y)
    VALUE x, y;
{
    switch (TYPE(y)) {
      case T_FIXNUM:
	y = rb_int2big(FIX2LONG(y));
	/* fall through */
      case T_BIGNUM:
	return bigadd(x, y, 0);

      case T_FLOAT:
	return rb_float_new(rb_big2dbl(x) - RFLOAT(y)->value);

      default:
	return rb_num_coerce_bin(x, y);
    }
}

VALUE
rb_big_mul(x, y)
    VALUE x, y;
{
    long i, j;
    unsigned long n = 0;
    VALUE z;
    USHORT *zds;

    if (FIXNUM_P(x)) x = rb_int2big(FIX2LONG(x));
    switch (TYPE(y)) {
      case T_FIXNUM:
	y = rb_int2big(FIX2LONG(y));
	break;

      case T_BIGNUM:
	break;

      case T_FLOAT:
	return rb_float_new(rb_big2dbl(x) * RFLOAT(y)->value);

      default:
	return rb_num_coerce_bin(x, y);
    }

    j = RBIGNUM(x)->len + RBIGNUM(y)->len + 1;
    z = bignew(j, RBIGNUM(x)->sign==RBIGNUM(y)->sign);
    zds = BDIGITS(z);
    while (j--) zds[j] = 0;
    for (i = 0; i < RBIGNUM(x)->len; i++) {
	unsigned long dd = BDIGITS(x)[i]; 
	if (dd == 0) continue;
	n = 0;
	for (j = 0; j < RBIGNUM(y)->len; j++) {
	    int ee = n + dd * BDIGITS(y)[j];
	    n = zds[i + j] + ee;
	    if (ee) zds[i + j] = BIGLO(n);
	    n = BIGDN(n);
	}
	if (n) {
	    zds[i + j] = n;
	}
    }

    return bignorm(z);
}

static void
bigdivmod(x, y, div, mod, modulo)
    VALUE x, y;
    VALUE *div, *mod;
    int modulo;
{
    long nx = RBIGNUM(x)->len, ny = RBIGNUM(y)->len;
    long i, j;
    VALUE yy, z;
    USHORT *xds, *yds, *zds, *tds;
    unsigned long t2;
    long num;
    USHORT dd, q;

    yds = BDIGITS(y);
    if (ny == 0 && yds[0] == 0) rb_num_zerodiv();
    if (nx < ny	|| nx == ny && BDIGITS(x)[nx - 1] < BDIGITS(y)[ny - 1]) {
	if (div) *div = INT2FIX(0);
	if (mod) *mod = bignorm(x);
	return;
    }
    xds = BDIGITS(x);
    if (ny == 1) {
	dd = yds[0];
	z = rb_big_clone(x);
	zds = BDIGITS(z);
	t2 = 0; i = nx;
	while (i--) {
	    t2 = BIGUP(t2) + zds[i];
	    zds[i] = (USHORT)(t2 / dd);
	    t2 %= dd;
	}
	RBIGNUM(z)->sign = RBIGNUM(x)->sign==RBIGNUM(y)->sign;
	if (div) *div = bignorm(z);
	if (mod) {
	    if (!RBIGNUM(y)->sign) t2 = -(long)t2;
	    *mod = INT2NUM(t2);
	}
	return;
    }
    z = bignew(nx==ny?nx+2:nx+1, RBIGNUM(x)->sign==RBIGNUM(y)->sign);
    zds = BDIGITS(z);
    if (nx==ny) zds[nx+1] = 0;
    while (!yds[ny-1]) ny--;
    if ((dd = BIGRAD/(int)(yds[ny-1]+1)) != 1) {
	yy = rb_big_clone(y);
	tds = BDIGITS(yy);
	j = 0;
	num = 0;
	while (j<ny) {
	    num += (long)yds[j]*dd;
	    tds[j++] = BIGLO(num);
	    num = BIGDN(num);
	}
	yds = tds;
	j = 0;
	num = 0;
	while (j<nx) {
	    num += (long)xds[j]*dd;
	    zds[j++] = BIGLO(num);
	    num = BIGDN(num);
	}
	zds[j] = (USHORT)num;
    }
    else {
	zds[nx] = 0;
	j = nx;
	while (j--) zds[j] = xds[j];
    }
    j = nx==ny?nx+1:nx;
    do {
	if (zds[j] ==  yds[ny-1]) q = BIGRAD-1;
	else q = (USHORT)((BIGUP(zds[j]) + zds[j-1])/yds[ny-1]);
	if (q) {
	    i = 0; num = 0; t2 = 0;
	    do {			/* multiply and subtract */
		int ee;
		t2 += (long)yds[i] * q;
		ee = num - BIGLO(t2);
		num = zds[j - ny + i] + ee;
		if (ee) zds[j - ny + i] = BIGLO(num);
		num = BIGDN(num);
		t2 = BIGDN(t2);
	    } while (++i < ny);
	    num += zds[j - ny + i] - t2; /* borrow from high digit; don't update */
	    while (num) {		/* "add back" required */
		i = 0; num = 0; q--;
		do {
		    int ee = num + yds[i];
		    num = (long) zds[j - ny + i] + ee;
		    if (ee) zds[j - ny + i] = BIGLO(num);
		    num = BIGDN(num);
		} while (++i < ny);
		num--;
	    }
	}
	zds[j] = q;
    } while (--j >= ny);
    if (div) {			/* move quotient down in z */
	*div = rb_big_clone(z);
	zds = BDIGITS(*div);
	j = (nx==ny ? nx+2 : nx+1) - ny;
	for (i = 0;i < j;i++) zds[i] = zds[i+ny];
	RBIGNUM(*div)->len = i;
	*div = bignorm(*div);
    }
    if (mod) {			/* just normalize remainder */
	*mod = rb_big_clone(z);
	if (dd) {
	    zds = BDIGITS(*mod);
	    t2 = 0; i = ny;
	    while(i--) {
		t2 = BIGUP(t2) + zds[i];
		zds[i] = (USHORT)(t2 / dd);
		t2 %= dd;
	    }
	}
	RBIGNUM(*mod)->len = ny;
	RBIGNUM(*mod)->sign = RBIGNUM(x)->sign;
	if (modulo && RBIGNUM(x)->sign != RBIGNUM(y)->sign) {
	    long len = ny;
	    zds = BDIGITS(*mod);
	    while (len-- && !zds[len]);
	    if (len > 0) {
		*mod = bigadd(*mod, y, 1);
		return;
	    }
	}
	*mod = bignorm(*mod);
    }
}

static VALUE
rb_big_div(x, y)
    VALUE x, y;
{
    VALUE z;

    switch (TYPE(y)) {
      case T_FIXNUM:
	y = rb_int2big(FIX2LONG(y));
	break;

      case T_BIGNUM:
	break;

      case T_FLOAT:
	return rb_float_new(rb_big2dbl(x) / RFLOAT(y)->value);

      default:
	return rb_num_coerce_bin(x, y);
    }
    bigdivmod(x, y, &z, 0, 0);

    return z;
}


static VALUE
rb_big_modulo(x, y, modulo)
    VALUE x, y;
    int modulo;
{
    VALUE z;

    switch (TYPE(y)) {
      case T_FIXNUM:
	y = rb_int2big(FIX2LONG(y));
	break;

      case T_BIGNUM:
	break;

      case T_FLOAT:
	y = rb_dbl2big(RFLOAT(y)->value);
	break;

      default:
	return rb_num_coerce_bin(x, y);
    }
    bigdivmod(x, y, 0, &z, modulo);

    return z;
}

static VALUE
rb_big_mod(x, y)
    VALUE x, y;
{
    return rb_big_modulo(x, y, 1);
}

static VALUE
rb_big_remainder(x, y)
    VALUE x, y;
{
    return rb_big_modulo(x, y, 0);
}

VALUE
rb_big_divmod(x, y)
    VALUE x, y;
{
    VALUE div, mod;

    switch (TYPE(y)) {
      case T_FIXNUM:
	y = rb_int2big(FIX2LONG(y));
	break;

      case T_FLOAT:
	y = rb_dbl2big(RFLOAT(y)->value);
	break;

      case T_BIGNUM:
	break;

      default:
	return rb_num_coerce_bin(x, y);
    }
    bigdivmod(x, y, &div, &mod, 1);

    return rb_assoc_new(div, mod);;
}

VALUE
rb_big_pow(x, y)
    VALUE x, y;
{
    double d;
    long yy;
    
    if (y == INT2FIX(0)) return INT2FIX(1);
    switch (TYPE(y)) {
      case T_FLOAT:
	d = RFLOAT(y)->value;
	break;

      case T_BIGNUM:
	rb_warn("in a**b, b may be too big");
	d = rb_big2dbl(y);
	break;

      case T_FIXNUM:
	yy = NUM2LONG(y);
	if (yy > 0) {
	    VALUE z;

	    z = x;
	    for (;;) {
		yy = yy - 1;
		if (yy == 0) break;
		while (yy % 2 == 0) {
		    yy = yy / 2;
		    x = rb_big_mul(x, x);
		}
		z = rb_big_mul(z, x);
	    }
	    if (!FIXNUM_P(z)) z = bignorm(z);
	    return z;
	}
	d = (double)yy;
	break;

      default:
	return rb_num_coerce_bin(x, y);
    }
    return rb_float_new(pow(rb_big2dbl(x), d));
}

VALUE
rb_big_and(x, y)
    VALUE x, y;
{
    VALUE z;
    USHORT *ds1, *ds2, *zds;
    long i, l1, l2;
    char sign;

    if (FIXNUM_P(y)) {
	y = rb_int2big(FIX2LONG(y));
    }
    else {
	Check_Type(y, T_BIGNUM);
    }

    if (!RBIGNUM(y)->sign) {
	y = rb_big_clone(y);
	rb_big_2comp(y);
    }
    if (!RBIGNUM(x)->sign) {
	x = rb_big_clone(x);
	rb_big_2comp(x);
    }
    if (RBIGNUM(x)->len > RBIGNUM(y)->len) {
	l1 = RBIGNUM(y)->len;
	l2 = RBIGNUM(x)->len;
	ds1 = BDIGITS(y);
	ds2 = BDIGITS(x);
	sign = RBIGNUM(y)->sign;
    }
    else {
	l1 = RBIGNUM(x)->len;
	l2 = RBIGNUM(y)->len;
	ds1 = BDIGITS(x);
	ds2 = BDIGITS(y);
	sign = RBIGNUM(x)->sign;
    }
    z = bignew(l2, RBIGNUM(x)->sign || RBIGNUM(y)->sign);
    zds = BDIGITS(z);

    for (i=0; i<l1; i++) {
	zds[i] = ds1[i] & ds2[i];
    }
    for (; i<l2; i++) {
	zds[i] = sign?0:ds2[i];
    }
    if (!RBIGNUM(z)->sign) rb_big_2comp(z);
    return bignorm(z);
}

VALUE
rb_big_or(x, y)
    VALUE x, y;
{
    VALUE z;
    USHORT *ds1, *ds2, *zds;
    unsigned long i, l1, l2;
    char sign;

    if (FIXNUM_P(y)) {
	y = rb_int2big(FIX2LONG(y));
    }
    else {
	Check_Type(y, T_BIGNUM);
    }

    if (!RBIGNUM(y)->sign) {
	y = rb_big_clone(y);
	rb_big_2comp(y);
    }
    if (!RBIGNUM(x)->sign) {
	x = rb_big_clone(x);
	rb_big_2comp(x);
    }
    if (RBIGNUM(x)->len > RBIGNUM(y)->len) {
	l1 = RBIGNUM(y)->len;
	l2 = RBIGNUM(x)->len;
	ds1 = BDIGITS(y);
	ds2 = BDIGITS(x);
	sign = RBIGNUM(y)->sign;
    }
    else {
	l1 = RBIGNUM(x)->len;
	l2 = RBIGNUM(y)->len;
	ds1 = BDIGITS(x);
	ds2 = BDIGITS(y);
	sign = RBIGNUM(x)->sign;
    }
    z = bignew(l2, RBIGNUM(x)->sign && RBIGNUM(y)->sign);
    zds = BDIGITS(z);

    for (i=0; i<l1; i++) {
	zds[i] = ds1[i] | ds2[i];
    }
    for (; i<l2; i++) {
	zds[i] = sign?ds2[i]:(BIGRAD-1);
    }
    if (!RBIGNUM(z)->sign) rb_big_2comp(z);

    return bignorm(z);
}

VALUE
rb_big_xor(x, y)
    VALUE x, y;
{
    VALUE z;
    USHORT *ds1, *ds2, *zds;
    unsigned int i, l1, l2;
    char sign;

    if (FIXNUM_P(y)) {
	y = rb_int2big(FIX2LONG(y));
    }
    else {
	Check_Type(y, T_BIGNUM);
    }

    if (!RBIGNUM(y)->sign) {
	y = rb_big_clone(y);
	rb_big_2comp(y);
    }
    if (!RBIGNUM(x)->sign) {
	x = rb_big_clone(x);
	rb_big_2comp(x);
    }
    if (RBIGNUM(x)->len > RBIGNUM(y)->len) {
	l1 = RBIGNUM(y)->len;
	l2 = RBIGNUM(x)->len;
	ds1 = BDIGITS(y);
	ds2 = BDIGITS(x);
	sign = RBIGNUM(y)->sign;
    }
    else {
	l1 = RBIGNUM(x)->len;
	l2 = RBIGNUM(y)->len;
	ds1 = BDIGITS(x);
	ds2 = BDIGITS(y);
	sign = RBIGNUM(x)->sign;
    }
    RBIGNUM(x)->sign = RBIGNUM(x)->sign?1:0;
    RBIGNUM(y)->sign = RBIGNUM(y)->sign?1:0;
    z = bignew(l2, !(RBIGNUM(x)->sign ^ RBIGNUM(y)->sign));
    zds = BDIGITS(z);

    for (i=0; i<l1; i++) {
	zds[i] = ds1[i] ^ ds2[i];
    }
    for (; i<l2; i++) {
	zds[i] = sign?ds2[i]:~ds2[i];
    }
    if (!RBIGNUM(z)->sign) rb_big_2comp(z);

    return bignorm(z);
}

static VALUE rb_big_rshift _((VALUE,VALUE));

VALUE
rb_big_lshift(x, y)
    VALUE x, y;
{
    USHORT *xds, *zds;
    int shift = NUM2INT(y);
    int s1 = shift/BITSPERDIG;
    int s2 = shift%BITSPERDIG;
    VALUE z;
    unsigned long num = 0;
    long len, i;

    if (shift < 0) return rb_big_rshift(x, INT2FIX(-shift));
    xds = BDIGITS(x);
    len = RBIGNUM(x)->len;
    z = bignew(len+s1+1, RBIGNUM(x)->sign);
    zds = BDIGITS(z);
    for (i=0; i<s1; i++) {
	*zds++ = 0;
    }
    for (i=0; i<len; i++) {
	num = num | *xds++<<s2;
	*zds++ = BIGLO(num);
	num = BIGDN(num);
    }
    *zds = BIGLO(num);
    return bignorm(z);
}

static VALUE
rb_big_rshift(x, y)
    VALUE x, y;
{
    USHORT *xds, *zds;
    int shift = NUM2INT(y);
    int s1 = shift/BITSPERDIG;
    int s2 = shift%BITSPERDIG;
    VALUE z;
    unsigned long num = 0;
    long i = RBIGNUM(x)->len;
    long j;

    if (shift < 0) return rb_big_lshift(x, INT2FIX(-shift));
    if (s1 > RBIGNUM(x)->len) {
	if (RBIGNUM(x)->sign)
	    return INT2FIX(0);
	else
	    return INT2FIX(-1);
    }
    xds = BDIGITS(x);
    i = RBIGNUM(x)->len; j = i - s1;
    z = bignew(j, RBIGNUM(x)->sign);
    zds = BDIGITS(z);
    while (i--, j--) {
	num = (num | xds[i]) >> s2;
	zds[j] = BIGLO(num);
	num = BIGUP(xds[i]);
    }
    return bignorm(z);
}

static VALUE
rb_big_aref(x, y)
    VALUE x, y;
{
    USHORT *xds;
    int shift = NUM2INT(y);
    int s1, s2;

    if (shift < 0) return INT2FIX(0);
    s1 = shift/BITSPERDIG;
    s2 = shift%BITSPERDIG;

    if (!RBIGNUM(x)->sign) {
	if (s1 >= RBIGNUM(x)->len) return INT2FIX(1);
	x = rb_big_clone(x);
	rb_big_2comp(x);
    }
    else {
	if (s1 >= RBIGNUM(x)->len) return INT2FIX(0);
    }
    xds = BDIGITS(x);
    if (xds[s1] & (1<<s2))
	return INT2FIX(1);
    return INT2FIX(0);
}

static VALUE
rb_big_hash(x)
    VALUE x;
{
    long i, len;
    int key;
    USHORT *digits;

    key = 0; digits = BDIGITS(x);
    for (i=0,len=RBIGNUM(x)->len; i<RBIGNUM(x)->len; i++) {
	key ^= *digits++;
    }
    return INT2FIX(key);
}

static VALUE
rb_big_coerce(x, y)
    VALUE x, y;
{
    if (FIXNUM_P(y)) {
	return rb_assoc_new(rb_int2big(FIX2LONG(y)), x);
    }
    else {
	rb_raise(rb_eTypeError, "Can't coerce %s to Bignum",
		 rb_class2name(CLASS_OF(y)));
    }
    /* not reached */
    return Qnil;
}

static VALUE
rb_big_abs(x)
    VALUE x;
{
    if (!RBIGNUM(x)->sign) {
	x = rb_big_clone(x);
	RBIGNUM(x)->sign = 1;
    }
    return x;
}

/* !!!warnig!!!!
   this is not really a random number!!
*/

VALUE
rb_big_rand(max, rand)
    VALUE max;
    double rand;
{
    VALUE v;
    long len;

    len = RBIGNUM(max)->len;
    v = bignew(len,1);
    while (len--) {
	BDIGITS(v)[len] = ((USHORT)~0) * rand;
    }

    return rb_big_mod((VALUE)v, max);
}

static VALUE
rb_big_size(big)
    VALUE big;
{
    return INT2FIX(RBIGNUM(big)->len*sizeof(USHORT));
}

static VALUE
rb_big_zero_p(big)
    VALUE big;
{
    return Qfalse;
}

void
Init_Bignum()
{
    rb_cBignum = rb_define_class("Bignum", rb_cInteger);

    rb_undef_method(CLASS_OF(rb_cBignum), "new");

    rb_define_method(rb_cBignum, "to_s", rb_big_to_s, 0);
    rb_define_method(rb_cBignum, "coerce", rb_big_coerce, 1);
    rb_define_method(rb_cBignum, "-@", rb_big_uminus, 0);
    rb_define_method(rb_cBignum, "+", rb_big_plus, 1);
    rb_define_method(rb_cBignum, "-", rb_big_minus, 1);
    rb_define_method(rb_cBignum, "*", rb_big_mul, 1);
    rb_define_method(rb_cBignum, "/", rb_big_div, 1);
    rb_define_method(rb_cBignum, "%", rb_big_mod, 1);
    rb_define_method(rb_cBignum, "divmod", rb_big_divmod, 1);
    rb_define_method(rb_cBignum, "remainder", rb_big_remainder, 1);
    rb_define_method(rb_cBignum, "**", rb_big_pow, 1);
    rb_define_method(rb_cBignum, "&", rb_big_and, 1);
    rb_define_method(rb_cBignum, "|", rb_big_or, 1);
    rb_define_method(rb_cBignum, "^", rb_big_xor, 1);
    rb_define_method(rb_cBignum, "~", rb_big_neg, 0);
    rb_define_method(rb_cBignum, "<<", rb_big_lshift, 1);
    rb_define_method(rb_cBignum, ">>", rb_big_rshift, 1);
    rb_define_method(rb_cBignum, "[]", rb_big_aref, 1);

    rb_define_method(rb_cBignum, "<=>", rb_big_cmp, 1);
    rb_define_method(rb_cBignum, "==", rb_big_eq, 1);
    rb_define_method(rb_cBignum, "===", rb_big_eq, 1);
    rb_define_method(rb_cBignum, "eql?", rb_big_eq, 1);
    rb_define_method(rb_cBignum, "hash", rb_big_hash, 0);
    rb_define_method(rb_cBignum, "to_i", rb_big_to_i, 0);
    rb_define_method(rb_cBignum, "to_f", rb_big_to_f, 0);
    rb_define_method(rb_cBignum, "abs", rb_big_abs, 0);
    rb_define_method(rb_cBignum, "size", rb_big_size, 0);
    rb_define_method(rb_cBignum, "zero?", rb_big_zero_p, 0);
}
