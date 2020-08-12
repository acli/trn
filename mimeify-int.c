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

int
mimeify_scan_input(input, statptr)
FILE * input;
mimeify_status_t *statptr;
{
    int st = 1;
    if (input == NULL)
	;
    else {
	bool has8bit = 0;
	int maxwidth = 0;
	int width;
	bool in_header = TRUE;
	for (width = 0;;) {
	    int c = fgetc(input);
	if (c == EOF) break;
	    if (c == '\n') {
		if (width > maxwidth)
		    maxwidth = width;
		width = 0;
	    } else
		width += 1;
	    if (c & 0200)
		has8bit = TRUE;
	}
	if (statptr != NULL) {
	    statptr->has8bit = has8bit;
	    statptr->maxwidth = maxwidth;
	}
	st = 0;
    }
    return st;
}

