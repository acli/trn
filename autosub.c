/* autosub.c
 */
/* This software is copyrighted as detailed in the LICENSE file. */


#include "EXTERN.h"
#include "common.h"
#include "search.h"
#include "list.h"
#include "ngdata.h"
#include "ngsrch.h"
#include "env.h"
#include "util.h"
#include "util2.h"
#include "final.h"
#include "INTERN.h"
#include "autosub.h"

/* Consider the newsgroup specified, and return:	*/
/* : if we should autosubscribe to it			*/
/* ! if we should autounsubscribe to it			*/
/* \0 if we should ask the user.			*/
int
auto_subscribe(name)
char* name;
{
    char* s;

    if((s = getval("AUTOSUBSCRIBE", (char*)NULL)) && matchlist(s, name))
	return ':';
    if((s = getval("AUTOUNSUBSCRIBE", (char*)NULL)) && matchlist(s, name))
	return '!';
    return 0;
}

bool
matchlist(patlist, s)
char* patlist;
char* s;
{
    COMPEX ilcompex;
    char* p;
    char* err;
    bool result;
    bool tmpresult;

    result = FALSE;
    init_compex(&ilcompex);
    while(patlist && *patlist) {
	if (*patlist == '!') {
	    patlist++;
	    tmpresult = FALSE;
	} else
	    tmpresult = TRUE;

	if ((p = index(patlist, ',')) != NULL)
	    *p = '\0';
        /* compile regular expression */
	err = ng_comp(&ilcompex,patlist,TRUE,TRUE);
	if (p)
	    *p++ = ',';

	if (err != NULL) {
	    printf("\n%s\n", err) FLUSH;
	    finalize(1);
	}
	
	if (execute(&ilcompex,s) != NULL)
	    result = tmpresult;
	patlist = p;
    }
    free_compex(&ilcompex);
    return result;
}
