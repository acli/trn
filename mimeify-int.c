/* This file written 2020 by Ambrose Li */
/* mimeify-int.c - mimify internals
 *
 * vi: set sw=4 ts=8 ai sm noet :
 */
/* This software is copyrighted as detailed in the LICENSE file. */

#include <stdio.h>

#include "EXTERN.h"
#include "common.h"
#include "utf.h"
#include "mimeify-int.h"
#include "INTERN.h"

#define STATE_HEADER_INITIAL		'H'
#define STATE_HEADER_MEDIAL		'h'
#define STATE_HEADER_CONTINUATION	'c'
#define STATE_BODY			'B'

int
mimeify_scan_input(input, output, statptr)
FILE * input;
FILE * output;
mimeify_status_t *statptr;
{
    int st = 1;
    if (input == NULL)
	;
    else {
	bool has8bit = 0;
	int maxbytelen = 0;
	int bytelen;
	int state = STATE_HEADER_INITIAL;
	for (bytelen = 0;;) {
	    int c = fgetc(input);
	if (c == EOF) break;
	    if (state == STATE_HEADER_INITIAL) {
		if (c == '\n')
		    state = STATE_BODY;
		else if (c == ' ' || c == '\t')
		    state = STATE_HEADER_CONTINUATION;
		else {
		    state = STATE_HEADER_MEDIAL;
		    bytelen += 1;
		}
	    } else if (state == STATE_HEADER_MEDIAL) {
		if (c == '\n')
		    state = STATE_HEADER_INITIAL;
		else
		    bytelen += 1;
	    } else if (state == STATE_HEADER_CONTINUATION) {
		if (c == '\n')
		    state = STATE_HEADER_INITIAL; /* this is probably not actually valid */
		else if (c == ' ' || c == '\t')
		    ;
		else {
		    state = STATE_HEADER_MEDIAL;
		    bytelen += 2;
		}
	    } else if (state == STATE_BODY) {
		if (c == '\n')
		    bytelen = 0;
		else
		    bytelen += 1;
	    }
	    if (bytelen > maxbytelen)
		maxbytelen = bytelen;
	    if (c & 0200)
		has8bit = TRUE;
	}
	if (statptr != NULL) {
	    statptr->has8bit = has8bit;
	    statptr->maxbytelen = maxbytelen;
	}
	st = 0;
    }
    return st;
}

