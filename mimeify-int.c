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
	int maxbytelen = 0;
	int bytelen;
	bool in_header = TRUE;
	for (bytelen = 0;;) {
	    int c = fgetc(input);
	if (c == EOF) break;
	    if (c == '\n') {
		if (bytelen > maxbytelen)
		    maxbytelen = bytelen;
		bytelen = 0;
	    } else
		bytelen += 1;
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

