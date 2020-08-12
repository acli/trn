/* This file written 2020 by Ambrose Li */
/* mimeify.c - Convert article into MIME format
 *
 * vi: set sw=4 ts=8 ai sm noet :
 */
/* This software is copyrighted as detailed in the LICENSE file. */

#include <stdio.h>

#include "EXTERN.h"
#include "common.h"
#include "utf.h"
#include "INTERN.h"


int
main(argc,argv)
int argc;
char* argv[];
{
    int st = 0;
    if (argc < 2) {
	st = 1;
    } else {
	;
    }
    return st;
}
