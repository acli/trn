/* ngdata.c
 */
/* This software is copyrighted as detailed in the LICENSE file. */


#include "EXTERN.h"
#include "common.h"
#include "list.h"
#include "trn.h"
#include "hash.h"
#include "cache.h"
#include "bits.h"
#include "head.h"
#include "rthread.h"
#include "rt-select.h"
#include "ng.h"
#include "intrp.h"
#include "kfile.h"
#include "final.h"
#include "term.h"
#include "env.h"
#include "util.h"
#include "util2.h"
#include "ndir.h"
#ifdef SCORE
#include "score.h"
#endif
#ifdef SCAN_ART
#include "scan.h"
#include "scanart.h"
#endif
#include "INTERN.h"
#include "ngdata.h"
#include "ngdata.ih"
#include "EXTERN.h"
#include "nntpclient.h"
#include "datasrc.h"
#include "nntp.h"
#include "rcstuff.h"
#include "rcln.h"

void
ngdata_init()
{
    ;
}

/* set current newsgroup */

void
set_ng(np)
NGDATA* np;
{
    ngptr = np;
    if (ngptr)
	set_ngname(ngptr->rcline);
}

int
access_ng()
{
#ifdef SUPPORT_NNTP
    ART_NUM old_first = ngptr->abs1st;

    if (datasrc->flags & DF_REMOTE) {
	int ret = nntp_group(ngname,ngptr);
	if (ret == -2)
	    return -2;
	if (ret <= 0) {
	    ngptr->toread = TR_BOGUS;
	    return 0;
	}
	if ((lastart = getngsize(ngptr)) < 0) /* Impossible... */
	    return 0;
	absfirst = ngptr->abs1st;
	if (absfirst > old_first)
	    checkexpired(ngptr,absfirst);
    }
    else
#endif
    {
	if (eaccess(ngdir,5)) {		/* directory read protected? */
	    if (eaccess(ngdir,0)) {
# ifdef VERBOSE
		IF(verbose)
		    printf("\nNewsgroup %s does not have a spool directory!\n",
			   ngname) FLUSH;
		ELSE
# endif
# ifdef TERSE
		    printf("\nNo spool for %s!\n",ngname) FLUSH;
# endif
		termdown(2);
	    } else {
# ifdef VERBOSE
		IF(verbose)
		    printf("\nNewsgroup %s is not currently accessible.\n",
			   ngname) FLUSH;
		ELSE
# endif
# ifdef TERSE
		    printf("\n%s not readable.\n",ngname) FLUSH;
# endif
		termdown(2);
	    }
	    /* make this newsgroup temporarily invisible */
	    ngptr->toread = TR_NONE;
	    return 0;
	}

	/* chdir to newsgroup subdirectory */

	if (chdir(ngdir)) {
	    printf(nocd,ngdir) FLUSH;
	    return 0;
	}
	if ((lastart = getngsize(ngptr)) < 0) /* Impossible... */
	    return 0;
	absfirst = ngptr->abs1st;
    }

    dmcount = 0;
    missing_count = 0;
    in_ng = TRUE;			/* tell the world we are here */

    build_cache();
    return 1;
}

void
chdir_newsdir()
{
    if (chdir(datasrc->spool_dir) || (
#ifdef SUPPORT_NNTP
			 !(datasrc->flags & DF_REMOTE) &&
#endif
					    chdir(ngdir))) {
	printf(nocd,ngdir) FLUSH;
	sig_catcher(0);
    }
}

void
grow_ng(newlast)
ART_NUM newlast;
{
    ART_NUM tmpfirst;

    forcegrow = FALSE;
    if (newlast > lastart) {
	ART_NUM tmpart = art;
	ngptr->toread += (ART_UNREAD)(newlast-lastart);
	tmpfirst = lastart+1;
#ifdef SCAN_ART
	/* Increase the size of article scan arrays. */
	sa_grow(lastart,newlast);
#endif
	do {
	    lastart++;
	    article_ptr(lastart)->flags |= AF_EXISTS|AF_UNREAD;
	} while (lastart < newlast);
	article_list->high = lastart;
	thread_grow();
#ifdef SCORE
	/* Score all new articles now just in case they weren't done above. */
	sc_fill_scorelist(tmpfirst,newlast);
#endif
#ifdef KILLFILES
#ifdef VERBOSE
	IF(verbose)
	    sprintf(buf,
		"%ld more article%s arrived -- processing memorized commands...\n\n",
		(long)(lastart - tmpfirst + 1),
		(lastart > tmpfirst ? "s have" : " has" ) );
	ELSE			/* my, my, how clever we are */
#endif
#ifdef TERSE
	    strcpy(buf, "More news -- auto-processing...\n\n");
#endif
	termdown(2);
	if (kf_state & KFS_NORMAL_LINES) {
	    bool forcelast_save = forcelast;
	    ARTICLE* artp_save = artp;
	    kill_unwanted(tmpfirst,buf,TRUE);
	    artp = artp_save;
	    forcelast = forcelast_save;
	}
#endif
	art = tmpart;
    }
}

static int
ngorder_number(npp1, npp2)
register NGDATA** npp1;
register NGDATA** npp2;
{
    return (int)((*npp1)->num - (*npp2)->num) * sel_direction;
}

static int
ngorder_groupname(npp1, npp2)
register NGDATA** npp1;
register NGDATA** npp2;
{
    return strcaseCMP((*npp1)->rcline, (*npp2)->rcline) * sel_direction;
}

static int
ngorder_count(npp1, npp2)
register NGDATA** npp1;
register NGDATA** npp2;
{
    int eq;
    if ((eq = (int)((*npp1)->toread - (*npp2)->toread)) != 0)
	return eq * sel_direction;
    return (int)((*npp1)->num - (*npp2)->num);
}

/* Sort the newsgroups into the chosen order.
*/
void
sort_newsgroups()
{
    register NGDATA* np;
    register int i;
    NGDATA** lp;
    NGDATA** ng_list;
    int (*sort_procedure)();

    /* If we don't have at least two newsgroups, we're done! */
    if (!first_ng || !first_ng->next)
	return;

    switch (sel_sort) {
      case SS_NATURAL:
      default:
	sort_procedure = ngorder_number;
	break;
      case SS_STRING:
	sort_procedure = ngorder_groupname;
	break;
      case SS_COUNT:
	sort_procedure = ngorder_count;
	break;
    }

    ng_list = (NGDATA**)safemalloc(newsgroup_cnt * sizeof (NGDATA*));
    for (lp = ng_list, np = first_ng; np; np = np->next)
	*lp++ = np;
    assert(lp - ng_list == newsgroup_cnt);

    qsort(ng_list, newsgroup_cnt, sizeof (NGDATA*), sort_procedure);

    first_ng = np = ng_list[0];
    np->prev = NULL;
    for (i = newsgroup_cnt, lp = ng_list; --i; lp++) {
	lp[0]->next = lp[1];
	lp[1]->prev = lp[0];
    }
    last_ng = lp[0];
    last_ng->next = NULL;
    free((char*)ng_list);
}

void
ng_skip()
{
#ifdef SUPPORT_NNTP
    if (datasrc->flags & DF_REMOTE) {
	ART_NUM artnum;

	clear();
# ifdef VERBOSE
	IF(verbose)
	    fputs("Skipping unavailable article\n",stdout);
	ELSE
# endif /* VERBOSE */
# ifdef TERSE
	    fputs("Skipping\n",stdout);
# endif /* TERSE */
	termdown(1);
	if (novice_delays) {
	    pad(just_a_sec/3);
	    sleep(1);
	}
	art = article_next(art);
	artp = article_ptr(art);
	do {
	    /* tries to grab PREFETCH_SIZE XHDRS, flagging missing articles */
	    (void) fetchsubj(art, FALSE);
	    artnum = art+PREFETCH_SIZE-1;
	    if (artnum > lastart)
		artnum = lastart;
	    while (art <= artnum) {
		if (artp->flags & AF_EXISTS)
		    return;
		art = article_next(art);
		artp = article_ptr(art);
	    }
	} while (art <= lastart);
    }
    else
#endif
    {
	if (errno != ENOENT) {	/* has it not been deleted? */
	    clear();
# ifdef VERBOSE
	    IF(verbose)
		printf("\n(Article %ld exists but is unreadable.)\n",(long)art)
			FLUSH;
	    ELSE
# endif
# ifdef TERSE
		printf("\n(%ld unreadable.)\n",(long)art) FLUSH;
# endif
	    termdown(2);
	    if (novice_delays) {
		pad(just_a_sec);
		sleep(2);
	    }
	}
	inc_art(selected_only,FALSE);	/* try next article */
    }
}

/* find the maximum article number of a newsgroup */

ART_NUM
getngsize(gp)
register NGDATA* gp;
{
    register int len;
    register char* nam;
    char tmpbuf[LBUFLEN];
    long last, first;
    char ch;

    nam = gp->rcline;
    len = gp->numoffset - 1;

    if (!find_actgrp(gp->rc->datasrc,tmpbuf,nam,len,gp->ngmax)) {
	if (gp->subscribechar == ':') {
	    gp->subscribechar = NEGCHAR;
	    gp->rc->flags |= RF_RCCHANGED;
	    newsgroup_toread--;
	}
	return TR_BOGUS;
    }

#ifdef ANCIENT_NEWS
    sscanf(tmpbuf+len+1, "%ld %c", &last, &ch);
    first = 1;
#else
    sscanf(tmpbuf+len+1, "%ld %ld %c", &last, &first, &ch);
#endif
    if (!gp->abs1st)
	gp->abs1st = (ART_NUM)first;
    if (!in_ng) {
	if (redirected) {
	    if (redirected != nullstr)
		free(redirected);
	    redirected = NULL;
	}
	switch (ch) {
	case 'n':
	    moderated = getval("NOPOSTRING"," (no posting)");
	    break;
	case 'm':
	    moderated = getval("MODSTRING", " (moderated)");
	    break;
	case 'x':
	    redirected = nullstr;
	    moderated = " (DISABLED)";
	    break;
	case '=':
	    len = strlen(tmpbuf);
	    if (tmpbuf[len-1] == '\n')
		tmpbuf[len-1] = '\0';
	    redirected = savestr(rindex(tmpbuf, '=') + 1);
	    moderated = " (REDIRECTED)";
	    break;
	default:
	    moderated = nullstr;
	    break;
	}
    }
    if (last <= gp->ngmax)
	return gp->ngmax;
    return gp->ngmax = (ART_NUM)last;
}
