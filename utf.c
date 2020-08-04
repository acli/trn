/* This file written 2020 by Ambrose Li */
/* utf.c - Functions for handling Unicode sort of properly
 *
 * vi: set sw=4 ts=8 ai sm noet :
 */
/* This software is copyrighted as detailed in the LICENSE file. */

#include <stdio.h>

#include "EXTERN.h"
#include "common.h"
#include "utf.h"
#include "INTERN.h"

/* OK - valid second and subsequent bytes in UTF-8 */
#define OK(s) ((*(s) & 0xC0) == 0x80)
#define U(c) (((Uchar)(c)) & 0xFF)

/* LEAD - decode leading byte in UTF-8 at (char *)s, bitmask mask, shift width bits
 * NEXT - decode second and subsequent bytes with byte value (char)s_i, shift width bits
 */
#define LEAD(s, mask, bits) ((*s & mask) << bits)
#define NEXT(s_i, bits) (((s_i) & 0x3F) << bits)

#define IS_UTF8(cs)		((cs) & 0x8000)
#define IS_SINGLE_BYTE(cs)	((cs) & 0x4000)
#define IS_DOUBLE_BYTE(cs)	((cs) & 0x2000)

#define IS_ISO_8859_X(cs)	(((cs) & 0x4010) == 0x4010)
#define IS_WINDOWS_125X(cs)	(((cs) & 0x4020) == 0x4020)

struct charset_desc {
    char *name;
    int id;
};

struct charset_desc charset_descs[] = {
    { "ascii", CHARSET_ASCII },
    { "us-ascii", CHARSET_ASCII },
    { "utf-8", CHARSET_UTF8 },
    { "iso8859-1", CHARSET_ISO8859_1 },
    { "iso8859-15", CHARSET_ISO8859_15 },
    { "iso-8859-1", CHARSET_ISO8859_1 },
    { "iso-8859-15", CHARSET_ISO8859_15 },
    { "windows-1252", CHARSET_WINDOWS_1252 },
    { NULL, CHARSET_UNKNOWN }
};

static int in_charset = CHARSET_UTF8;
static int out_charset = CHARSET_UTF8;

int
find_charset(s)
const char *s;
{
    int it = CHARSET_UNKNOWN;
    if (s) {
	int i;
	for (i = 0; ; i += 1) {
	    char *name = charset_descs[i].name;
	    int j;
	if (name == NULL) break;
	    for (j = 0; ; j++) {
	if (tolower(s[j]) != name[j]) break;
		if (s[j] == 0 && name[j] == 0)
		    it = charset_descs[i].id;
	if (s[j] == 0 || name[j] == 0) break;
	    }
	if (it != CHARSET_UNKNOWN) break;
	}
    }
    return it;
}

int
utf_init(f, t)
const char *f;
const char *t;
{
    int i = find_charset(f);
    int o = find_charset(t);
    if (i != CHARSET_UNKNOWN)
	in_charset = i;
    if (o != CHARSET_UNKNOWN)
	out_charset = o;
    return i;
}

int
byte_length_at(s)
const char *s;
{
    int it = s != NULL; /* correct for ASCII */
    if (!it) {
	;
    } else if (IS_UTF8(in_charset)) {
	size_t n = strlen(s);
	if (n > 0 && (*s & 0x80) == 0) {
	    ;
	} else if (n > 1 && (*s & 0xE0) == 0xC0 && OK(s + 1)) {
	    it = 2;
	} else if (n > 2 && (*s & 0xF0) == 0xE0 && OK(s + 1) && OK(s + 2)) {
	    it = 3;
	} else if (n > 3 && (*s & 0xF8) == 0xF0 && OK(s + 1) && OK(s + 2) && OK(s + 3)) {
	    it = 4;
	} else if (n > 4 && (*s & 0xFC) == 0xF8 && OK(s + 1) && OK(s + 2) && OK(s + 3) && OK(s + 4)) {
	    it = 5;
	} else if (n > 5 && (*s & 0xFE) == 0xFC && OK(s + 1) && OK(s + 2) && OK(s + 3) && OK(s + 4) && OK(s + 5)) {
	    it = 6;
	} else {
	    /* FIXME - invalid UTF-8 */
	}
    } else if (IS_SINGLE_BYTE(in_charset)) {
	;
    } else if (IS_DOUBLE_BYTE(in_charset)) {
	if (*s & 0x80)
	    it = 2;
    }
    return it;
}

/* NOTE: correctness is not guaranteed; this is only a rough generalization */
int
visual_width_at(s)
const char *s;
{
    CODE_POINT c = code_point_at(s);
    int it = 1;
    if (c == INVALID_CODE_POINT) {
	it = 0;
    } else if (IS_SINGLE_BYTE(in_charset)) {
	it = 1;
    } else if (IS_DOUBLE_BYTE(in_charset)) {
	it = (c & 0x80)? 2: 1;
    } else if ((c >= 0x00300 && c <= 0x0036F)	/* combining diacritics */
	    || (c >= 0x01AB0 && c <= 0x01AFF)
	    || (c >= 0x01DC0 && c <= 0x01DFF)
	    || (c == 0x0200B && c <= 0x0200F)	/* zwsp, zwnj, zwj, lrm, rlm */
	    || (c == 0x0202A && c <= 0x0202E)	/* lre, rle, pdf, lro, rlo */
	    || (c == 0x02060 && c <= 0x02064)) { /* wj,..., invisible plus */
	it = 0;
    } else if ((c >= 0x02E80 && c <= 0x04DBF)	/* CJK misc, kana, hangul */
	    || (c >= 0x04E00 && c <= 0x09FFF)	/* CJK ideographs */
	    || (c >= 0x0FE30 && c <= 0x0FE4F)	/* CJK compatibility forms */
	    || (c >= 0x0FF00 && c <= 0x0FF60)	/* CJK fullwidth forms */
	    || (c >= 0x0FFE0 && c <= 0x0FFE6)
	    || (c >= 0x20000 && c <= 0x2FA1F)) { /* more CJK ideographs */
	it = 2;
    }
    return it;
}

int
visual_length_of(s)
const char *s;
{
    int it = 0;
    if (s) {
	for (; *s; ) {
	    int w = byte_length_at(s);
	    int v = visual_width_at(s);
	    it += v;
	    s += w;
	}
    }
    return it;
}

CODE_POINT
code_point_at(s)
const char *s;
{
    CODE_POINT it;
    if (s != NULL) {
	if (IS_UTF8(in_charset)) {
	    size_t n = strlen(s);
	    if (n > 0 && (*s & 0x80) == 0) {
		it = *s;
	    } else if (n > 1 && (*s & 0xE0) == 0xC0 && OK(s + 1)) {
		it = LEAD(s, 0x1F, 6) | NEXT(s[1], 0);
	    } else if (n > 2 && (*s & 0xF0) == 0xE0 && OK(s + 1) && OK(s + 2)) {
		it = LEAD(s, 0x0F, 12) | NEXT(s[1], 6) | NEXT(s[2], 0);
	    } else if (n > 3 && (*s & 0xF8) == 0xF0 && OK(s + 1) && OK(s + 2) && OK(s + 3)) {
		it = LEAD(s, 0x07, 18) | NEXT(s[1], 12) | NEXT(s[2], 6) | NEXT(s[3], 0);
	    } else if (n > 4 && (*s & 0xFC) == 0xF8 && OK(s + 1) && OK(s + 2) && OK(s + 3) && OK(s + 4)) {
		it = LEAD(s, 0x03, 24) | NEXT(s[1], 18) | NEXT(s[2], 12) | NEXT(s[3], 6) | NEXT(s[4], 0);
	    } else if (n > 5 && (*s & 0xFE) == 0xFC && OK(s + 1) && OK(s + 2) && OK(s + 3) && OK(s + 4) && OK(s + 5)) {
		it = LEAD(s, 0x01, 30) | NEXT(s[1], 24) | NEXT(s[2], 18) | NEXT(s[3], 12) | NEXT(s[4], 6) | NEXT(s[5], 0);
	    } else {
		it = INVALID_CODE_POINT;
	    }
	} else if (in_charset == CHARSET_ASCII) {
	    it = *s & 0x7F;
	} else if (in_charset == CHARSET_ISO8859_1) {
	    it = *s & 0xFF; /* I hate signed/unsigned conversions */
	} else if (in_charset == CHARSET_ISO8859_15) {
	    it = *s & 0xFF;
	    if (it == '\244')
		it = 0x20AC;
	} else if (in_charset == CHARSET_WINDOWS_1252) {
	    it = *s & 0xFF;
	    if (it > 0x7F && it < 0xA0) {
		/*FIXME*/
	    }
	}
    } else {
	it = INVALID_CODE_POINT;
    }
    return it;
}

int
insert_utf8_at(s, c)
char *s;
CODE_POINT c;
{
    int it;
    /* FIXME - should we check if s has enough space? */
    if (s == NULL)
	it = 0;
    else if (c <= 0x0000007F) {
	s[0] = (char)c;
	it = 1;
    }
    else if (c <= 0x000007FF) {
	s[0] = ((char)(c >>  6) & 0x1F) | 0xC0;
	s[1] = ((char) c        & 0x3F) | 0x80;
	it = 2;
    }
    else if (c <= 0x0000FFFF) {
	s[0] = ((char)(c >> 12) & 0x1F) | 0xE0;
	s[1] = ((char)(c >>  6) & 0x3F) | 0x80;
	s[2] = ((char) c        & 0x3F) | 0x80;
	it = 3;
    }
    else if (c <= 0x001FFFFF) {
	s[0] = ((char)(c >> 18) & 0x1F) | 0xF0;
	s[1] = ((char)(c >> 12) & 0x3F) | 0x80;
	s[2] = ((char)(c >>  6) & 0x3F) | 0x80;
	s[3] = ((char) c        & 0x3F) | 0x80;
	it = 4;
    }
    else if (c <= 0x03FFFFFF) {
	s[0] = ((char)(c >> 24) & 0x1F) | 0xF8;
	s[1] = ((char)(c >> 18) & 0x3F) | 0x80;
	s[2] = ((char)(c >> 12) & 0x3F) | 0x80;
	s[3] = ((char)(c >>  6) & 0x3F) | 0x80;
	s[4] = ((char) c        & 0x3F) | 0x80;
	it = 5;
    }
    else if (c <= 0x7FFFFFFF) {
	s[0] = ((char)(c >> 30) & 0x1F) | 0xFC;
	s[1] = ((char)(c >> 24) & 0x3F) | 0x80;
	s[2] = ((char)(c >> 18) & 0x3F) | 0x80;
	s[3] = ((char)(c >> 12) & 0x3F) | 0x80;
	s[3] = ((char)(c >>  6) & 0x3F) | 0x80;
	s[4] = ((char) c        & 0x3F) | 0x80;
	it = 6;
    }
    return it;
}

int
insert_unicode_at(s, c)
char *s;
CODE_POINT c;
{
    int it;
    /* FIXME - should we check if s has enough space? */
    if (s == NULL)
	it = 0;
    else if (IS_UTF8(out_charset)) {
	it = insert_utf8_at(s, c);
    } else if (c <= 0x7F
	    || (out_charset == CHARSET_ISO8859_1 && c <= 0xFF)
	    || (out_charset == CHARSET_ISO8859_15 && c <= 0xFF && c != 0xA4)
	    || (out_charset == CHARSET_WINDOWS_1252 && (c < 0x7F || (c >= 0xA0 && c <= 0xFF)))) {
	s[0] = (char)c;
	it = 1;
    }
    return it;
}

bool
at_norm_char(s)
const char *s;
{
    int it = s != NULL;
    if (it) {
	if (in_charset == CHARSET_UTF8) {
	    CODE_POINT c = code_point_at(s);
	    it = c >= 0x20 && !(c >= 0x7F && c < 0xA0) && c != 0x2028 && c != 0x2029;
	}
	else if (in_charset == CHARSET_ASCII)
	    it = (U(*s) >= ' ' && U(*s) < 0x7F);
	else if (IS_ISO_8859_X(in_charset))
	    it = ((U(*s) >= ' ' && U(*s) < 0x7F) || (U(*s) >= 0xA0 && U(*s) <= 0xFF));
	else if (IS_WINDOWS_125X(in_charset))
	    it = (U(*s) >= ' ' && U(*s) <= 0xFF);
    }
    return it;
}

int
put_char_adv(strptr, outputok)
char **strptr;
bool_int outputok;
{
    int it;
    if (strptr == NULL) {
	it = 0;
    } else {
	char *s = *strptr;
	if (in_charset == CHARSET_UTF8 || (*s >= ' ' && *s < 0x7F)) {
	    int w = byte_length_at(s);
	    int i;
	    it = visual_width_at(s);
	    if (outputok)
		for (i = 0; i < w; i += 1) {
		    putchar(*s);
		    s++;
		}
	    *strptr = s;
	} else if (IS_ISO_8859_X(in_charset) || IS_WINDOWS_125X(in_charset)) { /* FIXME */
	    char buf[7];
	    int w = insert_utf8_at(buf, U(*s));
	    int i;
	    for (i = 0; i < w; i += 1)
		putchar(buf[i]);
	    it = 1;
	    s++;
	    *strptr = s;
	}
    }
    return it;
}

char *
create_utf8_copy(s)
char *s;
{
    char *it = s;
    if (s != NULL) {
	int slen;
	int tlen;
	int i;
	char buf[7];

	/* Precalculate size of required space */
	for (slen = tlen = 0; s[slen]; ) {
	    int sw = byte_length_at(s+slen);
	    int tw = insert_utf8_at(buf, code_point_at(s+slen));
	    slen += sw;
	    tlen += tw;
	}

	/* Create the actual copy */
	it = malloc(tlen + 1);
	if (it) {
	    char *bufptr = it;
	    for (i = 0; s[i]; ) {
		int sw = byte_length_at(s+i);
		bufptr += insert_utf8_at(bufptr, code_point_at(s+i));
		i += sw;
	    }
	    *bufptr = '\0';
	}
    }
    return it;
}
