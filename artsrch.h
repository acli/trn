/* artsrch.h
 */
/* This software is copyrighted as detailed in the LICENSE file. */


#ifndef NBRA
#include "search.h"
#endif

#ifdef ARTSEARCH

#define SRCH_ABORT 0
#define SRCH_INTR 1
#define SRCH_FOUND 2
#define SRCH_NOTFOUND 3
#define SRCH_DONE 4
#define SRCH_SUBJDONE 5
#define SRCH_ERROR 6
#endif

EXT char* lastpat INIT(nullstr);	/* last search pattern */
#ifdef ARTSEARCH
EXT COMPEX sub_compex;		/* last compiled subject search */
EXT COMPEX art_compex;		/* last compiled normal search */
EXT COMPEX* bra_compex INIT(&(art_compex)); /* current compex with brackets */

#define ARTSCOPE_SUBJECT	0
#define ARTSCOPE_FROM		1
#define ARTSCOPE_ONEHDR		2
#define ARTSCOPE_HEAD		3
#define ARTSCOPE_BODY_NOSIG	4
#define ARTSCOPE_BODY		5
#define ARTSCOPE_ARTICLE	6

EXT char scopestr[] INIT("sfHhbBa");
EXT int art_howmuch;		/* search scope */
EXT int art_srchhdr;		/* specific header number to search */
EXT bool art_doread;		/* search read articles? */
#endif

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

void artsrch_init _((void));
#ifdef ARTSEARCH
int art_search _((char*,int,int));
bool wanted _((COMPEX*,ART_NUM,char_int));
#endif
