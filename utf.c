/* This file written 2020 by Ambrose Li */
/* utf.c - Functions for handling Unicode sort of properly
 *
 * vi: set sw=4 ts=8 ai sm noet :
 */
/* This software is copyrighted as detailed in the LICENSE file. */


#include "EXTERN.h"
#include "common.h"
#include "utf.h"
#include "INTERN.h"

#define OK(s) ((*(s) & 0xC0) == 0x80)
#define U(c) ((Uchar)(c))

int
byte_length_at(s)
const char *s;
{
    int it = 1; /* correct for ASCII */
    size_t n = strlen(s);
    if (1) {
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

bool
at_norm_char(s)
const char *s;
{
    int it = 1;
    switch (byte_length_at(s)) {
    case 1:
	it = (U(*s) >= ' ' && U(*s) < '\177'); /* correct for ASCII */
    }
    return it;
}

int
put_char_adv(sp)
char **sp;
{
    char *s = *sp;
    int w = byte_length_at(s);
    int i;
    for (i = 0; i < w; i += 1) {
	putchar(*s);
	s++;
    }
    *sp = s;
    return 1; /* This is technically wrong because wide characters exist */
}
