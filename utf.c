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

#define XXXXXX INVALID_CODE_POINT

CODE_POINT cp1252_himap[128] = {
    0x20ac, XXXXXX, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021,	0x02c6, 0x2030, 0x0160, 0x2039, 0x0152, XXXXXX, 0x017d, XXXXXX,
    XXXXXX, 0x2018, 0x2019, 0x201c, 0x201d, 0x00b7, 0x2013, 0x2014,	0x02dc, 0x2122, 0x0161, 0x203a, 0x0153, XXXXXX, 0x017e, 0x0178,
    0x00a0, 0x00a1, 0x00a2, 0x00a3, 0x00a4, 0x00a5, 0x00a6, 0x00a7,	0x00a8, 0x00a9, 0x00aa, 0x00ab, 0x00ac, XXXXXX, 0x00ae, 0x00af,
    0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x00b4, 0x00b5, 0x00b6, 0x00b7,	0x00b8, 0x00b9, 0x00ba, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0x00bf,
    0x00c0, 0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5, 0x00c6, 0x00c7,	0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd, 0x00ce, 0x00cf,
    0x00d0, 0x00d1, 0x00d2, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x00d7,	0x00d8, 0x00d9, 0x00da, 0x00db, 0x00dc, 0x00dd, 0x00de, 0x00df,
    0x00e0, 0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x00e7,	0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef,
    0x00f0, 0x00f1, 0x00f2, 0x00f3, 0x00f4, 0x00f5, 0x00f6, 0x00f7,	0x00f8, 0x00f9, 0x00fa, 0x00fb, 0x00fc, 0x00fd, 0x00fe, 0x00ff,
};

CODE_POINT iso8859_1_himap[128] = {
    XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX,	XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX,
    XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX,	XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX,
    0x00a0, 0x00a1, 0x00a2, 0x00a3, 0x00a4, 0x00a5, 0x00a6, 0x00a7,	0x00a8, 0x00a9, 0x00aa, 0x00ab, 0x00ac, XXXXXX, 0x00ae, 0x00af,
    0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x00b4, 0x00b5, 0x00b6, 0x00b7,	0x00b8, 0x00b9, 0x00ba, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0x00bf,
    0x00c0, 0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5, 0x00c6, 0x00c7,	0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd, 0x00ce, 0x00cf,
    0x00d0, 0x00d1, 0x00d2, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x00d7,	0x00d8, 0x00d9, 0x00da, 0x00db, 0x00dc, 0x00dd, 0x00de, 0x00df,
    0x00e0, 0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x00e7,	0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef,
    0x00f0, 0x00f1, 0x00f2, 0x00f3, 0x00f4, 0x00f5, 0x00f6, 0x00f7,	0x00f8, 0x00f9, 0x00fa, 0x00fb, 0x00fc, 0x00fd, 0x00fe, 0x00ff,
};

CODE_POINT iso8859_15_himap[128] = {
    XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX,	XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX,
    XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX,	XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX, XXXXXX,
    0x00a0, 0x00a1, 0x00a2, 0x00a3, 0x20ac, 0x00a5, 0x0160, 0x00a7,	0x0161, 0x00a9, 0x00aa, 0x00ab, 0x00ac, XXXXXX, 0x00ae, 0x00af,
    0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x017d, 0x00b5, 0x00b6, 0x00b7,	0x017e, 0x00b9, 0x00ba, 0x00bb, 0x0152, 0x0153, 0x0178, 0x00bf,
    0x00c0, 0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5, 0x00c6, 0x00c7,	0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd, 0x00ce, 0x00cf,
    0x00d0, 0x00d1, 0x00d2, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x00d7,	0x00d8, 0x00d9, 0x00da, 0x00db, 0x00dc, 0x00dd, 0x00de, 0x00df,
    0x00e0, 0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x00e7,	0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef,
    0x00f0, 0x00f1, 0x00f2, 0x00f3, 0x00f4, 0x00f5, 0x00f6, 0x00f7,	0x00f8, 0x00f9, 0x00fa, 0x00fb, 0x00fc, 0x00fd, 0x00fe, 0x00ff,
};

typedef struct charset_desc {
    const char *name;
    int id;
    CODE_POINT* himap;
} charset_desc_t;

charset_desc_t charset_descs[] = {
    /* Tags defined in utf.h go first; these are short labels for charsubst.c */
    { TAG_ASCII, CHARSET_ASCII, NULL },
    { "us-ascii", CHARSET_ASCII, NULL },
    { "ascii", CHARSET_ASCII, NULL },

    { TAG_UTF8, CHARSET_UTF8, NULL },
    { "utf-8", CHARSET_UTF8, NULL },
    { "utf8", CHARSET_UTF8, NULL },

    { TAG_ISO8859_1, CHARSET_ISO8859_1, iso8859_1_himap },
    { "iso8859-1", CHARSET_ISO8859_1, iso8859_1_himap },
    { "iso-8859-1", CHARSET_ISO8859_1, iso8859_1_himap },

    { TAG_ISO8859_15, CHARSET_ISO8859_15, iso8859_15_himap },
    { "iso8859-15", CHARSET_ISO8859_15, iso8859_15_himap },
    { "iso-8859-15", CHARSET_ISO8859_15, iso8859_15_himap },

    { TAG_WINDOWS_1252, CHARSET_WINDOWS_1252, cp1252_himap },
    { "cp1252", CHARSET_WINDOWS_1252, cp1252_himap },
    { "windows-1252", CHARSET_WINDOWS_1252, cp1252_himap },

    { NULL, CHARSET_UNKNOWN }
};

struct gstate {
    int in;
    int out;
    CODE_POINT* himap_in;
    CODE_POINT* himap_out;
};

struct gstate gs = { CHARSET_UTF8, CHARSET_UTF8, NULL };


int
find_charset(s)
const char *s;
{
    int it = CHARSET_UNKNOWN;
    if (s) {
	int i;
	for (i = 0; ; i += 1) {
	    const char *name = charset_descs[i].name;
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

charset_desc_t *
find_charset_desc(id)
int id;
{
    const char *it = NULL;
    int i;
    for (i = 0; ; i += 1) {
	charset_desc_t *node = &charset_descs[i];
    if (node->name == NULL) break;
	if (id == node->id)
	    it = node;
    if (it != NULL) break;
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
    if (i != CHARSET_UNKNOWN) {
	gs.in = i;
	charset_desc_t *node = find_charset_desc(i);
	if (node)
	    gs.himap_in = node->himap;
    }
    if (o != CHARSET_UNKNOWN) {
	gs.out = o;
	charset_desc_t *node = find_charset_desc(o);
	if (node)
	    gs.himap_out = node->himap;
    }
    return i;
}

const char *
input_charset_name()
{
    return find_charset_desc(gs.in)->name;
}

const char *
output_charset_name()
{
    return find_charset_desc(gs.out)->name;
}

int
byte_length_at(s)
const char *s;
{
    int it = s != NULL; /* correct for ASCII */
    if (!it) {
	;
    } else if (IS_UTF8(gs.in)) {
	if ((*s & 0x80) == 0) {
	    ;
	} else if ((*s & 0xE0) == 0xC0 && OK(s + 1)) {
	    it = 2;
	} else if ((*s & 0xF0) == 0xE0 && OK(s + 1) && OK(s + 2)) {
	    it = 3;
	} else if ((*s & 0xF8) == 0xF0 && OK(s + 1) && OK(s + 2) && OK(s + 3)) {
	    it = 4;
	} else if ((*s & 0xFC) == 0xF8 && OK(s + 1) && OK(s + 2) && OK(s + 3) && OK(s + 4)) {
	    it = 5;
	} else if ((*s & 0xFE) == 0xFC && OK(s + 1) && OK(s + 2) && OK(s + 3) && OK(s + 4) && OK(s + 5)) {
	    it = 6;
	} else {
	    /* FIXME - invalid UTF-8 */
	}
    } else if (IS_SINGLE_BYTE(gs.in)) {
	;
    } else if (IS_DOUBLE_BYTE(gs.in)) {
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
    } else if (IS_SINGLE_BYTE(gs.in)) {
	it = 1;
    } else if (IS_DOUBLE_BYTE(gs.in)) {
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

int
visual_length_between(s1, s2)
const char *s1;
const char *s2;
{
    int it = 0;
    if (s1 && s2) {
	if (s1 > s2) {
	    const char *t = s1;
	    s1 = s2;
	    s2 = t;
	}
	for (; *s1 && s1 < s2; ) {
	    int w = byte_length_at(s1);
	    int v = visual_width_at(s1);
	    it += v;
	    s1 += w;
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
	if (IS_UTF8(gs.in)) {
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
	} else if (gs.in == CHARSET_ASCII) {
	    it = *s & 0x7F;
	} else if (gs.himap_in != NULL) {
	    it = *s & 0xFF; /* I hate signed/unsigned conversions */
	    if (it & 0x80)
		it = gs.himap_in[it & 0x7F];
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
    else if (IS_UTF8(gs.in)) {
	it = insert_utf8_at(s, c);
    } else if (c <= 0x7F) {
	s[0] = (char)c;
	it = 1;
    } else if (gs.himap_in != NULL) {
	it = insert_utf8_at(s, gs.himap_in[c & 0x7F]);
    }
    return it;
}

bool
at_norm_char(s)
const char *s;
{
    int it = s != NULL;
    if (it) {
	if (gs.in == CHARSET_UTF8) {
	    CODE_POINT c = code_point_at(s);
	    it = c >= 0x20 && !(c >= 0x7F && c < 0xA0) && c != 0x2028 && c != 0x2029;
	}
	else if (U(*s) < 0x80)
	    it = (U(*s) >= ' ' && U(*s) < 0x7F);
	else if (gs.himap_in != NULL)
	    it = gs.himap_in[U(*s) & 0x7F] != INVALID_CODE_POINT;
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
	if (gs.in == CHARSET_UTF8 || (*s >= ' ' && *s < 0x7F)) {
	    int w = byte_length_at(s);
	    int i;
	    it = visual_width_at(s);
	    if (outputok)
		for (i = 0; i < w; i += 1) {
		    putchar(*s);
		    s++;
		}
	    *strptr = s;
	} else if (gs.himap_in) {
	    char buf[7];
	    int w = insert_utf8_at(buf, gs.himap_in[U(*s) & 0x7F]);
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
