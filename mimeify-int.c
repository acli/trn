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
	st = 0;
    }
    return st;
}

