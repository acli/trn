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

#define OK(s) ((*(s) & 0xC0) == 0x80)
#define U(c) ((Uchar)(c))

#define LEAD(s, mask, bits) ((*s & mask) << bits)
#define NEXT(s_i, bits) (((s_i) & 0x3F) << bits)

int
byte_length_at(s)
const char *s;
{
    int it = s != NULL; /* correct for ASCII */
    if (it) {
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
    }
    return it;
}

/* NOTE: correctness is not guaranteed; this is only a rough generalization */
int
visual_width_at(s)
const char *s;
{
    unsigned long c = code_point_at(s);
    int it = 1;
    if (c == INVALID_CODE_POINT) {
	it = 0;
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

unsigned long
code_point_at(s)
const char *s;
{
    int it;
    if (s != NULL) {
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
    } else {
	it = INVALID_CODE_POINT;
    }
    return it;
}

bool
at_norm_char(s)
const char *s;
{
    int it = s != NULL;
    if (it) {
	switch (byte_length_at(s)) {
	case 1:
	    it = (U(*s) >= ' ' && U(*s) < '\177'); /* correct for ASCII */
	}
    }
    return it;
}

int
put_char_adv(sp)
char **sp;
{
    int it;
    if (sp == NULL) {
	it = 0;
    } else {
	char *s = *sp;
	int w = byte_length_at(s);
	int i;
	it = visual_width_at(s);
	for (i = 0; i < w; i += 1) {
	    putchar(*s);
	    s++;
	}
	*sp = s;
    }
    return it;
}
