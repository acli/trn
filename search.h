/* search.h
 */
/* This software is copyrighted as detailed in the LICENSE file. */


#ifndef NBRA
#define	NBRA	10		/* the maximum number of meta-brackets in an
				   RE -- \( \) */
#define NALTS	10		/* the maximum number of \|'s */
 
struct compex {
    char* expbuf;		/* The compiled search string */
    int eblen;			/* Length of above buffer */
    char* alternatives[NALTS+1];/* The list of \| seperated alternatives */
    char* braslist[NBRA];	/* RE meta-bracket start list */
    char* braelist[NBRA];	/* RE meta-bracket end list */
    char* brastr;		/* saved match string after execute() */
    char nbra;			/* The number of meta-brackets int the most
				   recenlty compiled RE */
    bool do_folding;		/* fold upper and lower case? */
};
#endif

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

void search_init _((void));
void init_compex _((COMPEX*));
void free_compex _((COMPEX*));
char* getbracket _((COMPEX*,int));
void case_fold _((int));
char* compile _((COMPEX*,char*,int,int));
char* grow_eb _((COMPEX*,char*,char**));
char* execute _((COMPEX*,char*));
bool advance _((COMPEX*,char*,char*));
bool backref _((COMPEX*,int,char*));
bool cclass _((char*,int,int));
