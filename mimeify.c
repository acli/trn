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
#include "mimeify-int.h"
#include "INTERN.h"


int
main(argc,argv)
int argc;
char* argv[];
{
    int st = 0;
    if (argc != 2) {
	fprintf(stderr, "Usage: mimeify FILE\n\
Create an equivalent MIME-compliant article from the specified FILE.\n\
The result is sent to standard output.\n\
");
	st = 1;
    } else {
	mimeify_status_t summary;
	FILE *input = fopen(argv[1], "r");
	if (input == NULL) {
	    perror(argv[1]);
	    st = 1;
	} else if (mimeify_scan_input(input, &summary)) {
	    mimeify_copy_input(input, stdout, &summary);
	    fclose(input);
	}
    }
    return st;
}
