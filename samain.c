/* This file Copyright 1992 by Clifford A. Adams */
/* samain.c
 *
 * main working routines  (may change later)
 */

#include "EXTERN.h"
#include "common.h"
#ifdef SCAN_ART
#include "list.h"
#include "hash.h"
#include "cache.h"
#include "ngdata.h"
#include "bits.h"
#include "head.h" /* for fetching misc header lines... */
#include "util.h"
#ifdef SCORE
#include "score.h"
#endif
#include "scan.h"
#include "scmd.h"
#include "sdisp.h"
#include "smisc.h"	/* needed? */
#include "sorder.h"
#include "spage.h"
#include "scanart.h"
#include "samisc.h"
#include "sacmd.h"
#include "sadesc.h"
#include "sadisp.h"
#include "sathread.h"
#include "INTERN.h"
#include "samain.h"


void
sa_init()
{
    sa_init_context();
    if (lastart == 0 || absfirst > lastart)
	return;		/* no articles */
    if (s_initscreen())		/* If not able to init screen...*/
	return;				/* ...most likely dumb terminal */
    sa_initmode();			/*      mode differences */
    sa_init_threads();
    sa_mode_read_elig = FALSE;
    if (firstart > lastart)		/* none unread */
	sa_mode_read_elig = TRUE;	/* unread+read in some situations */
    if (!sa_initarts())			/* init article array(s) */
	return;				/* ... no articles */
#ifdef SCORE
#ifdef PENDING
    if (sa_mode_read_elig) {
	sc_fill_read = TRUE;
	sc_fill_max = absfirst - 1;
    }
#endif
#endif
    s_save_context();
    sa_initialized = TRUE;		/* all went well... */
}

void
sa_init_ents()
{
    sa_num_ents = sa_ents_alloc = 0;
    sa_ents = (SA_ENTRYDATA*)NULL;
}

void
sa_clean_ents()
{
    free(sa_ents);
}

/* returns entry number that was added */
long
sa_add_ent(artnum)
ART_NUM artnum;		/* article number to be added */
{
    long cur;

    sa_num_ents++;
    if (sa_num_ents > sa_ents_alloc) {
	sa_ents_alloc += 100;
	if (sa_ents_alloc == 100) {	/* newly allocated */
	    /* don't use number 0, just allocate it and skip it */
	    sa_num_ents = 2;
	    sa_ents = (SA_ENTRYDATA*)safemalloc(sa_ents_alloc
					* sizeof (SA_ENTRYDATA));
        } else {
	    sa_ents = (SA_ENTRYDATA*)saferealloc((char*)sa_ents,
			sa_ents_alloc * sizeof (SA_ENTRYDATA));
	}
    }
    cur = sa_num_ents-1;
    sa_ents[cur].artnum = artnum;
    sa_ents[cur].subj_thread_num = 0;
    sa_ents[cur].sa_flags = (char)0;
    s_order_add(cur);
    return cur;
}

void
sa_cleanmain()
{
    sa_clean_ents();

    sa_mode_zoom = FALSE;	/* doesn't survive across groups */
    /* remove the now-unused scan-context */
    s_delete_context(sa_scan_context);
    sa_context_init = FALSE;
    sa_scan_context = -1;
    /* no longer "in" article scan mode */
    sa_mode_read_elig = FALSE;	/* the default */
    sa_in = FALSE;
}

void
sa_growarts(oldlast,last)
long oldlast,last;
{
    int i;

    for (i = oldlast+1; i <= last; i++)
	(void)sa_add_ent(i);
}

/* Initialize the scan-context to enter article scan mode. */
void
sa_init_context()
{
    if (sa_context_init)
	return;		/* already initialized */
    if (sa_scan_context == -1)
	sa_scan_context = s_new_context(S_ART);
    s_change_context(sa_scan_context);
}

bool
sa_initarts()
{
    int a;

    sa_init_ents();
    /* add all available articles to entry list */
    for (a = article_first(absfirst); a <= lastart; a = article_next(a)) {
	if (article_exists(a))
	    (void)sa_add_ent(a);
    }
    sa_order_read = sa_mode_read_elig;
    return TRUE;
}

/* note: initscreen must be called before (for scr_width) */
void
sa_initmode()
{
    /* set up screen sizes */
    sa_set_screen();

    sa_mode_zoom = 0;			/* reset zoom */
}

int
sa_mainloop()
{
    int i;

#ifdef SCORE
/* Eventually, strn will need a variable in score.[ch] set when the
 * scoring initialization *failed*, so that strn could try to
 * initialize the scoring again or decide to disallow score operations.)
 */
    /* If strn is in score mode but scoring is not initialized,
     * then try to initialize.
     * If that fails then strn will just use arrival ordering.
     */
    if (!sc_initialized && sa_mode_order == 2) {
	sc_delay = FALSE;	/* yes, actually score... */
	sc_init(TRUE);		/* wait for articles to score */
	if (!sc_initialized)
	    sa_mode_order = 1;	/* arrival order */
    }
#endif
    /* redraw it *all* */
    s_ref_all = TRUE;
    if (s_top_ent < 1)
	s_top_ent = s_first();
    i = s_fillpage();
    if (i == -1 || i == 0) {
	/* for now just quit if no page could be filled. */
	return SA_QUIT;
    }
    i = s_cmdloop();
    if (i == SA_READ) {
	i = SA_NORM;
    }
    if (i > 0) {
	sa_art = sa_ents[i].artnum;
	return SA_NORM;
    }
    /* something else (quit, return, etc...) */
    return i;
}

/* do something useful until a key is pressed. */
void
sa_lookahead()
{
#ifdef PENDING
#ifdef SCORE
    sc_lookahead(TRUE,FALSE);		/* do resorting now... */
#else /* !SCORE */
/* consider looking forward from the last article on the page... */
    ;				/* so the function isn't empty */
#endif /* SCORE */
#else /* !PENDING */
    ;				/* so the function isn't empty */
#endif
}

/* Returns first marked entry number, or 0 if no articles are marked. */
long
sa_readmarked_elig()
{
    long e;

    e = s_first();
    if (!e)
	return 0L;
    for ( ; e; e = s_next(e)) {
	if (!sa_basic_elig(e))
	    continue;
	if (sa_marked(e))
	    return e;
    }
    /* This is possible since the marked articles might not be eligible. */
    return 0;
}
#endif /* SCAN */
