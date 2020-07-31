/* ngsrch.h
 */
/* This software is copyrighted as detailed in the LICENSE file. */


#ifdef NGSEARCH
#define NGS_ABORT 0
#define NGS_FOUND 1
#define NGS_INTR 2
#define NGS_NOTFOUND 3
#define NGS_ERROR 4
#define NGS_DONE 5

EXT bool ng_doempty INIT(FALSE);	/* search empty newsgroups? */
#endif

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

void ngsrch_init _((void));
#ifdef NGSEARCH
int ng_search _((char*,int));
bool ng_wanted _((NGDATA*));
#endif
char* ng_comp _((COMPEX*,char*,bool_int,bool_int));
