/* ngsrch.c
 */
/* This software is copyrighted as detailed in the LICENSE file. */


#include "EXTERN.h"
#include "common.h"
#include "list.h"
#include "ngdata.h"
#include "hash.h"
#include "nntpclient.h"
#include "datasrc.h"
#include "addng.h"
#include "nntp.h"
#include "ngstuff.h"
#include "rcstuff.h"
#include "final.h"
#include "search.h"
#include "trn.h"
#include "util.h"
#include "util2.h"
#include "term.h"
#include "rcln.h"
#include "cache.h"
#include "rt-select.h"
#include "rt-util.h"
#include "INTERN.h"
#include "ngsrch.h"

#ifdef NGSEARCH
COMPEX ngcompex;
#endif

void
ngsrch_init()
{
#ifdef NGSEARCH
    init_compex(&ngcompex);
#endif
}

#ifdef NGSEARCH
int
ng_search(patbuf,get_cmd)
char* patbuf;				/* if patbuf != buf, get_cmd must */
int get_cmd;				/*   be set to FALSE!!! */
{
    register char cmdchr = *patbuf;	/* what kind of search? */
    register char* s;
    char* pattern;			/* unparsed pattern */
    char* cmdlst = NULL;		/* list of commands to do */
    int ret = NGS_NOTFOUND;		/* assume no commands */
    bool backward = cmdchr == '?';	/* direction of search */
    bool output_level = (!use_threads && gmode != 's');
    NGDATA* ng_start = ngptr;

    int_count = 0;
    if (get_cmd && buf == patbuf)
	if (!finish_command(FALSE))	/* get rest of command */
	    return NGS_ABORT;

    perform_status_init(newsgroup_toread);
    s = cpytill(buf,patbuf+1,cmdchr);	/* ok to cpy buf+1 to buf */
    for (pattern = buf; *pattern == ' '; pattern++) ;
    if (*pattern)
	ng_doempty = FALSE;

    if (*s) {				/* modifiers or commands? */
	while (*++s) {
	    switch (*s) {
	    case 'r':
		ng_doempty = TRUE;
		break;
	    default:
		goto loop_break;
	    }
	}
      loop_break:;
    }
    while (isspace(*s) || *s == ':')
	s++;
    if (*s)
	cmdlst = savestr(s);
    else if (gmode == 's')
	cmdlst = savestr("+");
    if (cmdlst)
	ret = NGS_DONE;
    if ((s = ng_comp(&ngcompex,pattern,TRUE,TRUE)) != NULL) {
					/* compile regular expression */
	errormsg(s);
	ret = NGS_ERROR;
	goto exit;
    }
    if (!cmdlst) {
	fputs("\nSearching...",stdout);	/* give them something to read */
	fflush(stdout);
    }

    if (first_addgroup) {
	ADDGROUP *gp = first_addgroup;
	do {
	    if (execute(&ngcompex,gp->name) != NULL) {
		if (!cmdlst)
		    return NGS_FOUND;
		if (addgrp_perform(gp,cmdlst,output_level && page_line==1)<0) {
		    free(cmdlst);
		    return NGS_INTR;
		}
	    }
	    if (!output_level && page_line == 1)
		perform_status(newsgroup_toread, 50);
	} while ((gp = gp->next) != NULL);
	goto exit;
    }

    if (backward) {
	if (!ngptr)
	    ng_start = ngptr = last_ng;
	else if (!cmdlst) {
	    if (ngptr == first_ng)	/* skip current newsgroup */
		ngptr = last_ng;
	    else
		ngptr = ngptr->prev;
	}
    }
    else {
	if (!ngptr)
	    ng_start = ngptr = first_ng;
	else if (!cmdlst) {
	    if (ngptr == last_ng)	/* skip current newsgroup */
		ngptr = first_ng;
	    else
		ngptr = ngptr->next;
	}
    }

    if (!ngptr)
	return NGS_NOTFOUND;

    do {
	if (int_count) {
	    int_count = 0;
	    ret = NGS_INTR;
	    break;
	}

	if (ngptr->toread >= TR_NONE && ng_wanted(ngptr)) {
	    if (ngptr->toread == TR_NONE)
		set_toread(ngptr, ST_LAX);
	    if (ng_doempty || ((ngptr->toread > TR_NONE) ^ sel_rereading)) {
		if (!cmdlst)
		    return NGS_FOUND;
		set_ng(ngptr);
		if (ng_perform(cmdlst,output_level && page_line == 1) < 0) {
		    free(cmdlst);
		    return NGS_INTR;
		}
	    }
	    if (output_level && !cmdlst) {
		printf("\n[0 unread in %s -- skipping]",ngptr->rcline);
		fflush(stdout);
	    }
	}
	if (!output_level && page_line == 1)
	    perform_status(newsgroup_toread, 50);
    } while ((ngptr = (backward? (ngptr->prev? ngptr->prev : last_ng)
			       : (ngptr->next? ngptr->next : first_ng)))
		!= ng_start);
exit:
    if (cmdlst)
	free(cmdlst);
    return ret;
}
#endif /* NGSEARCH */

#ifdef NGSEARCH
bool
ng_wanted(np)
NGDATA* np;
{
    return execute(&ngcompex,np->rcline) != NULL;
}
#endif /* NGSEARCH */

char*
ng_comp(compex,pattern,RE,fold)
COMPEX* compex;
char* pattern;
bool_int RE;
bool_int fold;
{
    char ng_pattern[128];
    register char* s = pattern;
    register char* d = ng_pattern;

    if (!*s) {
	if (compile(compex, "", RE, fold))
	    return "No previous search pattern";
	return NULL;			/* reuse old pattern */
    }
    for (; *s; s++) {
	if (*s == '.') {
	    *d++ = '\\';
	    *d++ = *s;
	}
	else if (*s == '?') {
	    *d++ = '.';
	}
	else if (*s == '*') {
	    *d++ = '.';
	    *d++ = *s;
	}
#if OLD_RN_WAY
	else if (strnEQ(s,"all",3)) {
	    *d++ = '.';
	    *d++ = '*';
	    s += 2;
	}
#endif
	else
	    *d++ = *s;
    }
    *d = '\0';
    return compile(compex,ng_pattern,RE,fold);
}
