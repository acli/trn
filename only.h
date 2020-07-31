/* only.h
 */
/* This software is copyrighted as detailed in the LICENSE file. */


#ifndef NBRA
#include "search.h"
#endif

EXT char* ngtodo[MAXNGTODO];		/* restrictions in effect */
EXT COMPEX* compextodo[MAXNGTODO];	/* restrictions in compiled form */

EXT int maxngtodo INIT(0);		/*  0 => no restrictions */
					/* >0 => # of entries in ngtodo */

EXT char empty_only_char INIT('o');

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

void only_init _((void));
void setngtodo _((char*));
bool inlist _((char*));
void end_only _((void));
void push_only _((void));
void pop_only _((void));
