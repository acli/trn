/* This file Copyright 1992 by Clifford A. Adams */
/* samisc.c
 *
 * lower-level routines
 */

#include "EXTERN.h"
#include "common.h"
#ifdef SCAN_ART
#include "list.h"
#include "hash.h"
#include "cache.h"
#include "artio.h"		/* openart */
#include "bits.h"
#include "final.h"		/* assert's sig_catcher() */
#include "term.h"		/* for "backspace" */
#include "util.h"
#include "scanart.h"
#include "samain.h"
#include "sathread.h"
#include "sorder.h"
#include "trn.h"
#include "ngdata.h"		/* several */
#include "rcstuff.h"
#include "ng.h"			/* for "art" */
#include "head.h"		/* fetchsubj */
#include "rthread.h"
#include "rt-select.h"		/* sel_mode */
#ifdef SCORE
#include "score.h"
#endif
#include "INTERN.h"
#include "samisc.h"

#ifdef UNDEF	/* use function for breakpoint debugging */
int
check_article(a)
long a;
{
    if (a < absfirst || a > lastart) {
	printf("\nArticle %d out of range\n",a) FLUSH;
	return FALSE;
    }
    return TRUE;
}
#else
/* note that argument is used twice. */
#define check_article(a) ((a) >= absfirst && (a) <= lastart)
#endif

/* ignoring "Fold" (or later recursive) mode(s), is this article eligible? */
bool
sa_basic_elig(a)
long a;
{
    ART_NUM artnum;

    artnum = sa_ents[a].artnum;
    assert(check_article(artnum));

    /* "run the gauntlet" style (:-) */
    if (!sa_mode_read_elig && was_read(artnum))
	return FALSE;
    if (sa_mode_zoom && !sa_selected1(a))
	return FALSE;
#ifdef SCORE
    if (sa_mode_order == 2)	/* score order */
	if (!SCORED(artnum))
	    return FALSE;
#endif
    /* now just check availability */
    if (is_unavailable(artnum)) {
	if (!was_read(artnum))
	    oneless_artnum(artnum);
	return FALSE;		/* don't try positively unavailable */
    }
    /* consider later positively checking availability */
    return TRUE;	/* passed all tests */
}

bool
sa_eligible(a)
long a;
{

    assert(check_article(sa_ents[a].artnum));
    if (!sa_basic_elig(a))
	return FALSE;		/* must always be basic-eligible */
    if (!sa_mode_fold)
	return TRUE;		/* just use basic-eligible */
    else {
	if (sa_subj_thread_prev(a))
	    return FALSE;	/* there was an earlier match */
	return TRUE;		/* no prior matches */
    }
}

/* given an article number, return the entry number for that article */
/* (There is no easy mapping between entry numbers and article numbers.) */
long
sa_artnum_to_ent(artnum)
ART_NUM artnum;
{
    long i;
    for (i = 1; i < sa_num_ents; i++)
	if (sa_ents[i].artnum == artnum)
	    return i;
    /* this had better not happen (complain?) */
    return -1;
}

/* select1 the articles picked in the TRN thread selector */
void
sa_selthreads()
{
    register SUBJECT *sp;
    register ARTICLE *ap;
    bool want_unread;
#if 0
    /* this does not work now, but maybe it will help debugging? */
    int subj_mask = (sel_mode == SM_THREAD? (SF_THREAD|SF_VISIT) : SF_VISIT);
#endif
    int subj_mask = SF_VISIT;

    long art;
    long i;

    want_unread = !sa_mode_read_elig;

    /* clear any old selections */
    for (i = 1; i < sa_num_ents; i++)
	sa_ents[i].sa_flags =
	    (sa_ents[i].sa_flags & 0xfd);

    /* Loop through all (selected) articles. */
    for (sp = first_subject; sp; sp = sp->next) {
	if ((sp->flags & subj_mask) == subj_mask) {
	    for (ap = first_art(sp); ap; ap = next_art(ap)) {
		art = article_num(ap);
		if ((ap->flags & AF_SEL)
		 && (!(ap->flags & AF_UNREAD) ^ want_unread)) {
		    /* this was a trn-thread selected article */
		    sa_select1(sa_artnum_to_ent(art));
#ifdef SCORE
    /* if scoring, make sure that this article is scored... */
		    if (sa_mode_order == 2)	/* score order */
			sc_score_art(art,FALSE);
#endif
		    }
		}/* for all articles */
	    }/* if selected */
	}/* for all threads */
    s_sort();
}

int
sa_number_arts()
{
    int total;
    long i;
    ART_NUM a;

    if (sa_mode_read_elig)
	i = absfirst;
    else
	i = firstart;
    total = 0;
    for (i = 1; i < sa_num_ents; i++) {
	a = sa_ents[i].artnum;
	if (is_unavailable(a))
	    continue;
	if (!article_unread(a) && !sa_mode_read_elig)
	    continue;
	total++;
    }
    return total;
}

/* needed for a couple functions which act within the
 * scope of an article.
 */
void
sa_go_art(a)
long a;
{
    art = a;
    (void)article_find(art);
    if (openart != art)
        artopen(art,(ART_POS)0);
}

int
sa_compare(a,b)
long a,b;		/* the entry numbers to compare */
{
    long i,j;
    
#ifdef SCORE
    if (sa_mode_order == 2) {	/* score order */
	/* do not score the articles here--move the articles to
	 * the end of the list if unscored.
	 */
	if (!SCORED(sa_ents[a].artnum)) {	/* a unscored */
	    if (!SCORED(sa_ents[b].artnum)) {	/* a+b unscored */
	        if (a < b)		/* keep ordering consistent */
		    return -1;
		return 1;
	    }
	    return 1;		/* move unscored (a) to end */
	}
	if (!SCORED(sa_ents[b].artnum))	/* only b unscored */
	    return -1;		/* move unscored (b) to end */

	i = sc_score_art(sa_ents[a].artnum,TRUE);
	j = sc_score_art(sa_ents[b].artnum,TRUE);
	if (i < j)
	    return 1;
	if (i > j)
	    return -1;
	/* i == j */
	if (score_newfirst) {
	    if (a < b)
		return 1;
	    return -1;
	} else {
	    if (a < b)
		return -1;
	    return 1;
	}
    }
#endif
    if (a < b)
	return -1;
    return 1;
}
#endif /* SCAN */
