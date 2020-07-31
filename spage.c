/* This file Copyright 1992 by Clifford A. Adams */
/* spage.c
 *
 */

#include "EXTERN.h"
#include "common.h"
#ifdef SCAN
#include "list.h"
#include "hash.h"
#include "cache.h"
#include "final.h"	/* assert() */
#include "ngdata.h"
#include "rt-util.h"	/* spinner */
#include "scan.h"
#include "sdisp.h"	/* for display dimension sizes */
#include "smisc.h"
#include "sorder.h"
#ifdef SCAN_ART
#include "scanart.h"
#include "samain.h"
#include "sathread.h"	/* sa_mode_fold */
#endif
#include "term.h"
#include "INTERN.h"
#include "spage.h"

/* returns TRUE if sucessful */
bool
s_fillpage_backward(end)
long end;		/* entry number to be last on page */
{
    int min_page_ents;	/* current minimum */
    int i,j;
    long a;		/* misc entry number */
    int page_lines;	/* lines available on page */
    int line_on;	/* line # currently on: 0=line after top status bar */

/* Debug */
#if 0
    printf("entry: s_fillpage_backward(%d)\n",end) FLUSH;
#endif

    page_lines = scr_height - s_top_lines - s_bot_lines;
    min_page_ents = MAX_PAGE_SIZE-1;
    s_bot_ent = -1;	/* none yet */
    line_on = 0;

    /* whatever happens, the entry display will need a full refresh... */
    s_ref_status = s_ref_desc = 0;	/* refresh from top entry */

    if (end < 1)		/* start from end */
	end = s_last();
    if (end == 0)		/* no entries */
	return FALSE;
    if (s_eligible(end))
	a = end;
    else
	a = s_prev_elig(end);
    if (!a)	/* no eligible entries */
	return FALSE;
    /* at this point we *know* that there is at least one eligible */

/* what if the "first" one for the page has a description too long? */
/* later make it shorten the descript. */

/* CONSIDER: make setspin conditional on context? */
    setspin(SPIN_BACKGROUND);	/* turn on spin on cache misses */
    /* uncertain what next comment means now */
    /* later do sheer paranoia check for min_page_ents */
    while ((line_on+s_ent_lines(a)) <= page_lines) {
	page_ents[min_page_ents].entnum = a;
	i = s_ent_lines(a);
	page_ents[min_page_ents].lines = i;
	min_page_ents--;
	s_bot_ent += 1;
	line_on = line_on+i;
	a = s_prev_elig(a);
	if (!a)		/* no more eligible */
	    break;	/* get out of loop and finish up... */
    }
/* what if none on page? (desc. too long) Fix later */
    setspin(SPIN_POP);	/* turn off spin on cache misses */
    /* replace the entries at the front of the page_ents array */
    /* also set start_line entries */
    j = 0;
    line_on = 0;
    for (i = min_page_ents+1; i < MAX_PAGE_SIZE; i++) {
	page_ents[j].entnum = page_ents[i].entnum;
	page_ents[j].pageflags = (char)0;
	page_ents[j].lines = page_ents[i].lines;
	page_ents[j].start_line = line_on;
	line_on = line_on + page_ents[j].lines;
	j++;
    }
    /* set new s_top_ent */
    s_top_ent = page_ents[0].entnum;
    /* Now, suppose that the pointer position is off the page.  That would
     * be bad, so lets make sure it doesn't happen.
     */
    if (s_ptr_page_line > s_bot_ent)
	s_ptr_page_line = s_bot_ent;
#ifdef SCAN_ART
    if (s_cur_type != S_ART)
	return TRUE;
    /* temporary fix.  Under some conditions ineligible entries will
     * not be found until they are in the page.  In this case just
     * refill the page.
     */
    for (i = 0; i <= s_bot_ent; i++)
	if (is_unavailable(sa_ents[page_ents[i].entnum].artnum))
	    break;
    if (i <= s_bot_ent)
	return s_fillpage_backward(end);
/* next time the unavail won't be chosen */
#endif
    return TRUE;	/* we have a page... */
}

/* fills the page array */
/* returns TRUE on success */
bool
s_fillpage_forward(start)
long start;			/* entry to start filling with */
{
    int i;
    long a;
    int page_lines;	/* lines available on page */
    int line_on;	/* line # currently on (0: line after top status bar */

/* Debug */
#if 0
    printf("entry: s_fillpage_forward(%d)\n",start) FLUSH;
#endif

    page_lines = scr_height - s_top_lines - s_bot_lines;
    s_bot_ent = -1;
    line_on = 0;

    /* whatever happens, the entry zone will need a full refresh... */
    s_ref_status = s_ref_desc = 0;

    if (start < 0)	/* fill from top */
	start = s_first();
    if (start == 0)	/* no entries */
	return FALSE;

    if (s_eligible(start))
	a = start;
    else
	a = s_next_elig(start);
    if (!a)	/* no eligible entries */
	return FALSE;
    /* at this point we *know* that there is at least one eligible */

/* what if the first entry for the page has a description too long? */
/* later make it shorten the descript. */

    setspin(SPIN_BACKGROUND);	/* turn on spin on cache misses */
/* ?  later do paranoia check for s_bot_ent */
    while ((line_on+s_ent_lines(a)) <= page_lines) {
	s_bot_ent += 1;
	page_ents[s_bot_ent].entnum = a;
	page_ents[s_bot_ent].start_line = line_on;
	i = s_ent_lines(a);
	page_ents[s_bot_ent].lines = i;
	page_ents[s_bot_ent].pageflags = (char)0;
	line_on = line_on+i;
	a = s_next_elig(a);
	if (!a)		/* no more eligible */
	    break;	/* get out of loop and finish up... */
    }
    setspin(SPIN_POP);	/* turn off spin on cache misses */
    /* Now, suppose that the pointer position is off the page.  That would
     * be bad, so lets make sure it doesn't happen.
     */
    /* set new s_top_ent */
    s_top_ent = page_ents[0].entnum;
    if (s_ptr_page_line > s_bot_ent)
	s_ptr_page_line = s_bot_ent;
#ifdef SCAN_ART
    if (s_cur_type != S_ART)
	return TRUE;
    /* temporary fix.  Under some conditions ineligible entries will
     * not be found until they are in the page.  In this case just
     * refill the page.
     */
    for (i = 0; i <= s_bot_ent; i++)
	if (is_unavailable(sa_ents[page_ents[i].entnum].artnum))
	    break;
    if (i <= s_bot_ent)
	return s_fillpage_forward(start);
    /* next time the unavail won't be chosen */
#endif
    return TRUE;	/* we have a page... */
}

/* Given possible changes to which entries should be on the page,
 * fills the page with more/less entries.  Tries to minimize refreshing
 * (the last entry on the page will be refreshed whether it needs it
 *  or not.)
 */
/* returns TRUE on success */
bool
s_refillpage()
{
    int i,j;
    long a;
    int page_lines;	/* lines available on page */
    int line_on;	/* line # currently on: 0=line after top status bar */

/* Debug */
#if 0
    printf("entry: s_refillpage\n") FLUSH;
#endif

    page_lines = scr_height - s_top_lines - s_bot_lines;
    /* if the top entry is not the s_top_ent,
     *    or the top entry is not eligible,
     *    or the top entry is not on the first line,
     *    or the top entry has a different # of lines now,
     * just refill the whole page.
     */
    if (s_bot_ent < 1 || s_top_ent < 1
     ||	s_top_ent != page_ents[0].entnum || !s_eligible(page_ents[0].entnum)
     || page_ents[0].start_line != 0
     || page_ents[0].lines != s_ent_lines(page_ents[0].entnum))
	return s_fillpage_forward(s_top_ent);

    i = 1;
    /* CAA misc note: I used to have
     * a = page_ents[1];
     * ...at this point.  This caused a truly difficult to track bug...
     * (briefly, occasionally the entry in page_ents[1] would be
     *  *before* (by display order) the entry in page_ents[0].  In
     *  this case the start_line entry of page_ents[0] could be overwritten
     *  causing big problems...)
     */
    a = s_next_elig(page_ents[0].entnum);

    /* similar to the tests in the last loop... */
    while (i <= s_bot_ent && s_eligible(page_ents[i].entnum)
      && page_ents[i].entnum == a
      && page_ents[i].lines == s_ent_lines(page_ents[i].entnum)) {
	i++;
	a = s_next_elig(a);
    }
    j = i-1;	/* j is the last "good" entry */

    s_bot_ent = j;
    line_on = page_ents[j].start_line + s_ent_lines(page_ents[j].entnum);
    a = s_next_elig(page_ents[j].entnum);

    setspin(SPIN_BACKGROUND);
    while (a && line_on+s_ent_lines(a) <= page_lines) {
	i = s_ent_lines(a);
	s_bot_ent += 1;
	page_ents[s_bot_ent].entnum = a;
	page_ents[s_bot_ent].lines = i;
	page_ents[s_bot_ent].start_line = line_on;
	page_ents[s_bot_ent].pageflags = (char)0;
	line_on = line_on+i;
	a = s_next_elig(a);
    }
    setspin(SPIN_POP);

    /* there are fairly good reasons to refresh the last good entry, such
     * as clearing the rest of the screen...
     */
    s_ref_status = s_ref_desc = j;

    /* Now, suppose that the pointer position is off the page.  That would
     * be bad, so lets make sure it doesn't happen.
     */
    if (s_ptr_page_line > s_bot_ent)
	s_ptr_page_line = s_bot_ent;
#ifdef SCAN_ART
    if (s_cur_type != S_ART)
	return TRUE;
    /* temporary fix.  Under some conditions ineligible entries will
     * not be found until they are in the page.  In this case just
     * refill the page.
     */
    for (i = 0; i <= s_bot_ent; i++)
	if (is_unavailable(sa_ents[page_ents[i].entnum].artnum))
	    break;
    if (i <= s_bot_ent)
	return s_refillpage();	/* next time the unavail won't be chosen */
#endif
    return TRUE;	/* we have a page... */
}

/* fills a page from current position.
 *   if that fails, backs up for a page.
 *     if that fails, downgrade eligibility requirements and try again
 * returns positive # on normal fill,
 *         negative # on special condition
 *         0 on failure
 */
int
s_fillpage()
{
    int i;

    s_refill = FALSE;	/* we don't need one now */
    if (s_top_ent < 1)	/* set top to first entry */
	s_top_ent = s_first();
    if (s_top_ent == 0)	/* no entries */
	return 0;	/* failure */
    if (!s_refillpage())	/* try for efficient refill */
	if (!s_fillpage_backward(s_top_ent)) {
	    /* downgrade eligibility standards */
	    switch (s_cur_type) {
#ifdef SCAN_ART
	      case S_ART:		/* article context */
		if (sa_mode_zoom) {		/* we were zoomed in */
		    s_ref_top = TRUE;	/* for "FOLD" display */
		    sa_mode_zoom = FALSE;	/* zoom out */
		    if (sa_unzoomrefold)
			sa_mode_fold = TRUE;
		    (void)s_go_top_ents();	/* go to top (ents and page) */
		    return s_fillpage();
		}
		return -1;		/* there just aren't entries! */
#endif
	      default:
		return -1;		/* there just aren't entries! */
	    } /* switch */
	} /* if */
    /* temporary fix.  Under some conditions ineligible entries will
     * not be found until they are in the page.  In this case just
     * refill the page.
     */
    for (i = 0; i <= s_bot_ent; i++)
	if (!s_eligible(page_ents[i].entnum))
	    return s_fillpage();  /* ineligible won't be chosen again */
#ifdef SCAN_ART
    if (s_cur_type != S_ART)
	return 1;
    /* be extra cautious about the article scan pages */
    for (i = 0; i <= s_bot_ent; i++)
	if (is_unavailable(sa_ents[page_ents[i].entnum].artnum))
	    break;
    if (i <= s_bot_ent)
	return s_fillpage();	/* next time the unavail won't be chosen */
#endif
    return 1;	/* I guess everything worked :-) */
}

void
s_cleanpage()
{
}

void
s_go_top_page()
{
    s_ptr_page_line = 0;
}

void
s_go_bot_page()
{
    s_ptr_page_line = s_bot_ent;
}

bool
s_go_top_ents()
{
    s_top_ent = s_first();
    if (!s_top_ent)
	printf("s_go_top_ents(): no first entry\n") FLUSH;
    assert(s_top_ent);	/* be nicer later */
    if (!s_eligible(s_top_ent))	/* this may save a redraw...*/
	s_top_ent = s_next_elig(s_top_ent);
    if (!s_top_ent) {	/* none eligible */
	/* just go to the top of all the entries */
	if (s_cur_type == S_ART)
	    s_top_ent = absfirst;
	else
	    s_top_ent = 1;	/* not very nice coding */
    }
    s_refill = TRUE;
    s_go_top_page();
    return TRUE;	/* successful */
}

bool
s_go_bot_ents()
{
    bool flag;

    flag = s_fillpage_backward(s_last());	/* fill backwards */
    if (!flag)
	return FALSE;
    s_go_bot_page();
    return TRUE;
}

void
s_go_next_page()
{
    long a;
    bool flag;

    a = s_next_elig(page_ents[s_bot_ent].entnum);
    if (!a)
	return;		/* no next page (we shouldn't have been called) */
    /* the fill-page will set the refresh for the screen */
    flag = s_fillpage_forward(a);
    assert(flag);		/* I *must* be able to fill a page */
    s_ptr_page_line = 0;	/* top of page */
}

void
s_go_prev_page()
{
    long a;
    bool flag;

    a = s_prev_elig(page_ents[0].entnum);
    if (!a)
	return;		/* no prev. page (we shouldn't have been called) */
    /* the fill-page will set the refresh for the screen */
    flag = s_fillpage_backward(a);	/* fill backward */
    assert(flag);		/* be nicer later... */
    /* take care of partially filled previous pages */
    flag = s_refillpage();
    assert(flag);		/* be nicer later... */
    s_ref_status = s_ref_desc = 0;	/* refresh from top */
    s_ptr_page_line = 0;	/* top of page */
}
#endif /* SCAN */
