/* only.c
 */
/* This software is copyrighted as detailed in the LICENSE file. */


#include "EXTERN.h"
#include "common.h"
#include "search.h"
#include "util.h"
#include "util2.h"
#include "final.h"
#include "list.h"
#include "term.h"
#include "ngdata.h"
#include "ngsrch.h"
#include "INTERN.h"
#include "only.h"

static int save_maxngtodo = 0;

void
only_init()
{
    ;
}

void
setngtodo(pat)
char* pat;
{
    char* s;
    register int i = maxngtodo + save_maxngtodo;

    if (!*pat)
	return;
    if (i < MAXNGTODO) {
	ngtodo[i] = savestr(pat);
#ifndef lint
	compextodo[i] = (COMPEX*)safemalloc(sizeof(COMPEX));
#endif
	init_compex(compextodo[i]);
	compile(compextodo[i],pat,TRUE,TRUE);
	if ((s = ng_comp(compextodo[i],pat,TRUE,TRUE)) != NULL) {
	    printf("\n%s\n",s) FLUSH;
	    finalize(1);
	}
	maxngtodo++;
    }
}

/* if command line list is non-null, is this newsgroup wanted? */

bool
inlist(ngnam)
char* ngnam;
{
    register int i;

    if (maxngtodo == 0)
	return TRUE;
    for (i = save_maxngtodo; i < maxngtodo + save_maxngtodo; i++) {
	if (execute(compextodo[i],ngnam))
	    return TRUE;
    }
    return FALSE;
}

void
end_only()
{
    if (maxngtodo) {			/* did they specify newsgroup(s) */
	int i;

#ifdef VERBOSE
	IF(verbose)
	    sprintf(msg, "Restriction %s%s removed.",ngtodo[0],
		    maxngtodo > 1 ? ", etc." : nullstr);
	ELSE
#endif
#ifdef TERSE
	    sprintf(msg, "Exiting \"only\".");
#endif
	for (i = save_maxngtodo; i < maxngtodo + save_maxngtodo; i++) {
	    free(ngtodo[i]);
	    free_compex(compextodo[i]);
#ifndef lint
	    free((char*)compextodo[i]);
#endif
	}
	maxngtodo = 0;
	ng_min_toread = 1;
    }
}

void
push_only()
{
    save_maxngtodo = maxngtodo;
    maxngtodo = 0;
}

void
pop_only()
{
    ART_UNREAD save_ng_min_toread = ng_min_toread;

    end_only();

    maxngtodo = save_maxngtodo;
    save_maxngtodo = 0;

    ng_min_toread = save_ng_min_toread;
}
