/* rthread.c
*/
/* This software is copyrighted as detailed in the LICENSE file. */


#include "EXTERN.h"
#include "common.h"
#include "list.h"
#include "intrp.h"
#include "trn.h"
#include "hash.h"
#include "cache.h"
#include "bits.h"
#include "ngdata.h"
#include "nntpclient.h"
#include "datasrc.h"
#include "nntp.h"
#include "ng.h"
#include "rcln.h"
#include "search.h"
#include "artstate.h"
#include "rcstuff.h"
#include "final.h"
#include "kfile.h"
#include "head.h"
#include "util.h"
#include "util2.h"
#include "rt-mt.h"
#include "rt-ov.h"
#ifdef SCORE
#include "score.h"
#endif
#include "rt-page.h"
#include "rt-process.h"
#include "rt-select.h"
#include "rt-util.h"
#include "rt-wumpus.h"
#include "INTERN.h"
#include "rthread.h"
#include "rthread.ih"

void
thread_init()
{
}

/* Generate the thread data we need for this group.  We must call
** thread_close() before calling this again.
*/
void
thread_open()
{
    bool find_existing = !first_subject;
    time_t save_ov_opened;
    ARTICLE* ap;

    if (!msgid_hash)
	msgid_hash = hashcreate(1999, msgid_cmp); /*TODO: pick a better size */
    if (ThreadedGroup) {
	/* Parse input and use msgid_hash for quick article lookups. */
	/* If cached but not threaded articles exist, set up to thread them. */
	if (first_subject) {
	    first_cached = firstart;
	    last_cached = firstart - 1;
	    parsed_art = 0;
	}
    }

    if (sel_mode == SM_ARTICLE)
	set_selector(sel_mode, sel_artsort);
    else
	set_selector(sel_threadmode, sel_threadsort);

    if ((datasrc->flags & DF_TRY_THREAD) && !first_subject) {
	if (mt_data() < 0)
	    return;
    }
    if ((datasrc->flags & DF_TRY_OVERVIEW) && !cached_all_in_range) {
	if (thread_always) {
	    spin_todo = spin_estimate = lastart - absfirst + 1;
	    (void) ov_data(absfirst, lastart, FALSE);
	    if (datasrc->ov_opened && find_existing && datasrc->over_dir == NULL) {
		mark_missing_articles();
		rc_to_bits();
		find_existing = FALSE;
	    }
	}
	else {
	    spin_todo = lastart - firstart + 1;
	    spin_estimate = ngptr->toread;
	    if (firstart > lastart) {
		/* If no unread articles, see if ov. exists as fast as possible */
		(void) ov_data(absfirst, absfirst, FALSE);
		cached_all_in_range = FALSE;
	    } else
		(void) ov_data(firstart, lastart, FALSE);
	}
    }
    if (find_existing) {
	find_existing_articles();
	mark_missing_articles();
	rc_to_bits();
    }

    for (ap = article_ptr(article_first(firstart));
	 ap && article_num(ap) <= last_cached;
	 ap = article_nextp(ap))
    {
	if ((ap->flags & (AF_EXISTS|AF_CACHED)) == AF_EXISTS)
	    (void) parseheader(article_num(ap));
    }

    if (last_cached > lastart) {
	ngptr->toread += (ART_UNREAD)(last_cached-lastart);
	/* ensure getngsize() knows the new maximum */
	ngptr->ngmax = lastart = last_cached;
    }
    article_list->high = lastart;

    for (ap = article_ptr(article_first(absfirst));
	 ap && article_num(ap) <= lastart;
	 ap = article_nextp(ap))
    {
	if (ap->flags & AF_CACHED)
	    check_poster(ap);
    }

    save_ov_opened = datasrc->ov_opened;
    datasrc->ov_opened = 0; /* avoid trying to call ov_data twice for high arts */
    thread_grow();	    /* thread any new articles not yet in the database */
    datasrc->ov_opened = save_ov_opened;
    added_articles = 0;
    sel_page_sp = 0;
    sel_page_app = 0;
}

/* Update the group's thread info.
*/
void
thread_grow()
{
    added_articles += lastart - last_cached;
    if (added_articles && thread_always)
	cache_range(last_cached + 1, lastart);
    count_subjects(CS_NORM);
    if (artptr_list)
	sort_articles();
    else
	sort_subjects();
}

void
thread_close()
{
    curr_artp = artp = NULL;
    init_tree();			/* free any tree lines */

    update_thread_kfile();
    if (msgid_hash)
	hashwalk(msgid_hash, cleanup_msgid_hash, 0);
    sel_page_sp = 0;
    sel_page_app = 0;
    sel_last_ap = 0;
    sel_last_sp = 0;
    selected_only = FALSE;
    sel_exclusive = 0;
    ov_close();
}

static int
cleanup_msgid_hash(keylen, data, extra)
int keylen;
HASHDATUM* data;
int extra;
{
    register ARTICLE* ap = (ARTICLE*)data->dat_ptr;
    int ret = -1;

    if (ap) {
	if (data->dat_len)
	    return 0;
	if ((kf_state & KFS_GLOBAL_THREADFILE) && ap->autofl) {
	    data->dat_ptr = ap->msgid;
	    data->dat_len = ap->autofl;
	    ret = 0;
	    ap->msgid = NULL;
	}
	if (ap->flags & AF_TMPMEM) {
	    clear_article(ap);
	    free((char*)ap);
	}
    }
    return ret;
}

void
top_article()
{
    art = lastart+1;
    artp = NULL;
    inc_art(selected_only, FALSE);
    if (art > lastart && last_cached < lastart)
	art = firstart;
}

ARTICLE*
first_art(sp)
register SUBJECT* sp;
{
    register ARTICLE* ap = (ThreadedGroup? sp->thread : sp->articles);
    if (ap && !(ap->flags & AF_EXISTS)) {
	oneless(ap);
	ap = next_art(ap);
    }
    return ap;
}

ARTICLE*
last_art(sp)
register SUBJECT* sp;
{
    register ARTICLE* ap;

    if (!ThreadedGroup) {
	ap = sp->articles;
	while (ap->subj_next)
	    ap = ap->subj_next;
	return ap;
    }

    ap = sp->thread;
    if (ap) {
	for (;;) {
	    if (ap->sibling)
		ap = ap->sibling;
	    else if (ap->child1)
		ap = ap->child1;
	    else
		break;
	}
	if (!(ap->flags & AF_EXISTS)) {
	    oneless(ap);
	    ap = prev_art(ap);
	}
    }
    return ap;
}

/* Bump art/artp to the next article, wrapping from thread to thread.
** If sel_flag is TRUE, only stops at selected articles.
** If rereading is FALSE, only stops at unread articles.
*/
void
inc_art(sel_flag, rereading)
bool_int sel_flag, rereading;
{
    register ARTICLE* ap = artp;
    int subj_mask = (rereading? 0 : SF_VISIT);

    /* Use the explicit article-order if it exists */
    if (artptr_list) {
	ARTICLE** limit = artptr_list + artptr_list_size;
	if (!ap)
	    artptr = artptr_list-1;
	else if (!artptr || *artptr != ap) {
	    for (artptr = artptr_list; artptr < limit; artptr++) {
		if (*artptr == ap)
		    break;
	    }
	}
	do {
	    if (++artptr >= limit)
		break;
	    ap = *artptr;
	} while ((!rereading && !(ap->flags & AF_UNREAD))
	      || (sel_flag && !(ap->flags & AF_SEL)));
	if (artptr < limit) {
	    artp = *artptr;
	    art = article_num(artp);
	} else {
	    artp = NULL;
	    art = lastart+1;
	    artptr = artptr_list;
	}
	return;
    }

    /* Use subject- or thread-order when possible */
    if (ThreadedGroup || srchahead) {
	register SUBJECT* sp;
	if (ap)
	    sp = ap->subj;
	else
	    sp = next_subj((SUBJECT*)NULL, subj_mask);
	if (!sp)
	    goto num_inc;
	do {
	    if (ap)
		ap = next_art(ap);
	    else
		ap = first_art(sp);
	    while (!ap) {
		sp = next_subj(sp, subj_mask);
		if (!sp)
		    break;
		ap = first_art(sp);
	    }
	} while (ap && ((!rereading && !(ap->flags & AF_UNREAD))
		     || (sel_flag && !(ap->flags & AF_SEL))));
	if ((artp = ap) != NULL)
	    art = article_num(ap);
	else {
	    if (art <= last_cached)
		art = last_cached+1;
	    else
		art++;
	    if (art <= lastart)
		artp = article_ptr(art);
	    else
		art = lastart+1;
	}
	return;
    }

    /* Otherwise, just increment through the art numbers */
  num_inc:
    if (!ap)
      art = firstart-1;
    for (;;) {
	art = article_next(art);
	if (art > lastart) {
	    art = lastart+1;
	    ap = NULL;
	    break;
	}
	ap = article_ptr(art);
	if (!(ap->flags & AF_EXISTS))
	    oneless(ap);
	else if ((rereading || (ap->flags & AF_UNREAD))
	      && (!sel_flag || (ap->flags & AF_SEL)))
	    break;
    } 
    artp = ap;
}

/* Bump art/artp to the previous article, wrapping from thread to thread.
** If sel_flag is TRUE, only stops at selected articles.
** If rereading is FALSE, only stops at unread articles.
*/
void
dec_art(sel_flag, rereading)
bool_int sel_flag, rereading;
{
    register ARTICLE* ap = artp;
    int subj_mask = (rereading? 0 : SF_VISIT);

    /* Use the explicit article-order if it exists */
    if (artptr_list) {
	ARTICLE** limit = artptr_list + artptr_list_size;
	if (!ap)
	    artptr = limit;
	else if (!artptr || *artptr != ap) {
	    for (artptr = artptr_list; artptr < limit; artptr++) {
		if (*artptr == ap)
		    break;
	    }
	}
	do {
	    if (artptr == artptr_list)
		break;
	    ap = *--artptr;
	} while ((!rereading && !(ap->flags & AF_UNREAD))
	      || (sel_flag && !(ap->flags & AF_SEL)));
	artp = *artptr;
	art = article_num(artp);
	return;
    }

    /* Use subject- or thread-order when possible */
    if (ThreadedGroup || srchahead) {
	register SUBJECT* sp;
	if (ap)
	    sp = ap->subj;
	else
	    sp = prev_subj((SUBJECT*)NULL, subj_mask);
	if (!sp)
	    goto num_dec;
	do {
	    if (ap)
		ap = prev_art(ap);
	    else
		ap = last_art(sp);
	    while (!ap) {
		sp = prev_subj(sp, subj_mask);
		if (!sp)
		    break;
		ap = last_art(sp);
	    }
	} while (ap && ((!rereading && !(ap->flags & AF_UNREAD))
		     || (sel_flag && !(ap->flags & AF_SEL))));
	if ((artp = ap) != NULL)
	    art = article_num(ap);
	else
	    art = absfirst-1;
	return;
    }

    /* Otherwise, just decrement through the art numbers */
  num_dec:
    for (;;) {
	art = article_prev(art);
	if (art < absfirst) {
	    art = absfirst-1;
	    ap = NULL;
	    break;
	}
	ap = article_ptr(art);
	if (!(ap->flags & AF_EXISTS))
	    oneless(ap);
	else if ((rereading || (ap->flags & AF_UNREAD))
	      && (!sel_flag || (ap->flags & AF_SEL)))
	    break;
    }
    artp = ap;
}

/* Bump the param to the next article in depth-first order.
*/
ARTICLE*
bump_art(ap)
register ARTICLE* ap;
{
    if (ap->child1)
	return ap->child1;
    while (!ap->sibling) {
	if (!(ap = ap->parent))
	    return NULL;
    }
    return ap->sibling;
}

/* Bump the param to the next REAL article.  Uses subject order in a
** non-threaded group; honors the breadth_first flag in a threaded one.
*/
ARTICLE*
next_art(ap)
register ARTICLE* ap;
{
try_again:
    if (!ThreadedGroup) {
	ap = ap->subj_next;
	goto done;
    }
    if (breadth_first) {
	if (ap->sibling) {
	    ap = ap->sibling;
	    goto done;
	}
	if (ap->parent)
	    ap = ap->parent->child1;
	else
	    ap = ap->subj->thread;
    }
    do {
	if (ap->child1) {
	    ap = ap->child1;
	    goto done;
	}
	while (!ap->sibling) {
	    if (!(ap = ap->parent))
		return NULL;
	}
	ap = ap->sibling;
    } while (breadth_first);
done:
    if (ap && !(ap->flags & AF_EXISTS)) {
	oneless(ap);
	goto try_again;
    }
    return ap;
}

/* Bump the param to the previous REAL article.  Uses subject order in a
** non-threaded group.
*/
ARTICLE*
prev_art(ap)
register ARTICLE* ap;
{
    register ARTICLE* initial_ap;

try_again:
    initial_ap = ap;
    if (!ThreadedGroup) {
	if ((ap = ap->subj->articles) == initial_ap)
	    ap = NULL;
	else
	    while (ap->subj_next != initial_ap)
		ap = ap->subj_next;
	goto done;
    }
    ap = (ap->parent ? ap->parent->child1 : ap->subj->thread);
    if (ap == initial_ap) {
	ap = ap->parent;
	goto done;
    }
    while (ap->sibling != initial_ap)
	ap = ap->sibling;
    while (ap->child1) {
	ap = ap->child1;
	while (ap->sibling)
	    ap = ap->sibling;
    }
done:
    if (ap && !(ap->flags & AF_EXISTS)) {
	oneless(ap);
	goto try_again;
    }
    return ap;
}

/* Find the next art/artp with the same subject as this one.  Returns
** FALSE if no such article exists.
*/
bool
next_art_with_subj()
{
    register ARTICLE* ap = artp;

    if (!ap)
	return FALSE;

    do {
	ap = ap->subj_next;
	if (!ap) {
	    if (!art)
		art = firstart;
	    return FALSE;
	}
    } while (!ALLBITS(ap->flags, AF_EXISTS | AF_UNREAD)
	  || (selected_only && !(ap->flags & AF_SEL)));
    artp = ap;
    art = article_num(ap);
#ifdef ARTSEARCH
    srchahead = -1;
#endif
    return TRUE;
}

/* Find the previous art/artp with the same subject as this one.  Returns
** FALSE if no such article exists.
*/
bool
prev_art_with_subj()
{
    register ARTICLE* ap = artp;
    register ARTICLE* ap2;

    if (!ap)
	return FALSE;

    do {
	ap2 = ap->subj->articles;
	if (ap2 == ap)
	    ap = NULL;
	else {
	    while (ap2 && ap2->subj_next != ap)
		ap2 = ap2->subj_next;
	    ap = ap2;
	}
	if (!ap) {
	    if (!art)
		art = lastart;
	    return FALSE;
	}
    } while (!(ap->flags & AF_EXISTS)
	  || (selected_only && !(ap->flags & AF_SEL)));
    artp = ap;
    art = article_num(ap);
    return TRUE;
}

SUBJECT*
next_subj(sp, subj_mask)
register SUBJECT* sp;
int subj_mask;
{
    if (!sp)
	sp = first_subject;
    else if (sel_mode == SM_THREAD) {
	ARTICLE* ap = sp->thread;
	do {
	    sp = sp->next;
	} while (sp && sp->thread == ap);
    }
    else
	sp = sp->next;

    while (sp && (sp->flags & subj_mask) != subj_mask) {
	sp = sp->next;
    }
    return sp;
}

SUBJECT*
prev_subj(sp, subj_mask)
register SUBJECT* sp;
int subj_mask;
{
    if (!sp)
	sp = last_subject;
    else if (sel_mode == SM_THREAD) {
	ARTICLE* ap = sp->thread;
	do {
	    sp = sp->prev;
	} while (sp && sp->thread == ap);
    }
    else
	sp = sp->prev;

    while (sp && (sp->flags & subj_mask) != subj_mask) {
	sp = sp->prev;
    }
    return sp;
}

/* Select a single article.
*/
void
select_article(ap, auto_flags)
register ARTICLE* ap;
int auto_flags;
{
    int desired_flags = (sel_rereading? AF_EXISTS : (AF_EXISTS|AF_UNREAD));
#ifdef VERBOSE
    bool echo = (auto_flags & ALSO_ECHO) != 0;
#endif
    auto_flags &= AUTO_SELS;
    if ((ap->flags & (AF_EXISTS|AF_UNREAD)) == desired_flags) {
	if (!(ap->flags & sel_mask)) {
	    selected_count++;
#ifdef VERBOSE
	    IF(verbose && echo && gmode != 's')
		fputs("\tSelected",stdout);
#endif
	}
	ap->flags = (ap->flags & ~AF_DEL) | sel_mask;
    }
    if (auto_flags)
	change_auto_flags(ap, auto_flags);
    if (ap->subj) {
	if (!(ap->subj->flags & sel_mask))
	    selected_subj_cnt++;
	ap->subj->flags = (ap->subj->flags&~SF_DEL) | sel_mask | SF_VISIT;
    }
    selected_only = (selected_only || selected_count != 0);
}

/* Select this article's subject.
*/
void
select_arts_subject(ap, auto_flags)
register ARTICLE* ap;
int auto_flags;
{
    if (ap->subj && ap->subj->articles)
	select_subject(ap->subj, auto_flags);
    else
	select_article(ap, auto_flags);
}

/* Select all the articles in a subject.
*/
void
select_subject(subj, auto_flags)
SUBJECT* subj;
int auto_flags;
{
    register ARTICLE* ap;
    int desired_flags = (sel_rereading? AF_EXISTS : (AF_EXISTS|AF_UNREAD));
    int old_count = selected_count;

    auto_flags &= AUTO_SELS;
    for (ap = subj->articles; ap; ap = ap->subj_next) {
	if ((ap->flags & (AF_EXISTS|AF_UNREAD|sel_mask)) == desired_flags) {
	    ap->flags |= sel_mask;
	    selected_count++;
	}
	if (auto_flags)
	    change_auto_flags(ap, auto_flags);
    }
    if (selected_count > old_count) {
	if (!(subj->flags & sel_mask))
	    selected_subj_cnt++;
	subj->flags = (subj->flags & ~SF_DEL)
		    | sel_mask | SF_VISIT | SF_WASSELECTED;
	selected_only = TRUE;
    } else
	subj->flags |= SF_WASSELECTED;
}

/* Select this article's thread.
*/
void
select_arts_thread(ap, auto_flags)
register ARTICLE* ap;
int auto_flags;
{
    if (ap->subj && ap->subj->thread)
	select_thread(ap->subj->thread, auto_flags);
    else
	select_arts_subject(ap, auto_flags);
}

/* Select all the articles in a thread.
*/
void
select_thread(thread, auto_flags)
register ARTICLE* thread;
int auto_flags;
{
    register SUBJECT* sp;

    sp = thread->subj;
    do {
	select_subject(sp, auto_flags);
	sp = sp->thread_link;
    } while (sp != thread->subj);
}

/* Select the subthread attached to this article.
*/
void
select_subthread(ap, auto_flags)
register ARTICLE* ap;
int auto_flags;
{
    register ARTICLE* limit;
    SUBJECT* subj;
    int desired_flags = (sel_rereading? AF_EXISTS : (AF_EXISTS|AF_UNREAD));
    int old_count = selected_count;

    if (!ap)
	return;
    subj = ap->subj;
    for (limit = ap; limit; limit = limit->parent) {
	if (limit->sibling) {
	    limit = limit->sibling;
	    break;
	}
    }

    auto_flags &= AUTO_SELS;
    for (; ap != limit; ap = bump_art(ap)) {
	if ((ap->flags & (AF_EXISTS|AF_UNREAD|sel_mask)) == desired_flags) {
	    ap->flags |= sel_mask;
	    selected_count++;
	}
	if (auto_flags)
	    change_auto_flags(ap, auto_flags);
    }
    if (subj && selected_count > old_count) {
	if (!(subj->flags & sel_mask))
	    selected_subj_cnt++;
	subj->flags = (subj->flags & ~SF_DEL) | sel_mask | SF_VISIT;
	selected_only = TRUE;
    }
}

/* Deselect a single article.
*/
void
deselect_article(ap, auto_flags)
register ARTICLE* ap;
int auto_flags;
{
#ifdef VERBOSE
    bool echo = (auto_flags & ALSO_ECHO) != 0;
#endif
    auto_flags &= AUTO_SELS;
    if (ap->flags & sel_mask) {
	ap->flags &= ~sel_mask;
	if (!selected_count--)
	    selected_count = 0;
#ifdef VERBOSE
	IF(verbose && echo && gmode != 's')
	    fputs("\tDeselected",stdout);
#endif
    }
    if (sel_rereading && sel_mode == SM_ARTICLE)
	ap->flags |= AF_DEL;
}

/* Deselect this article's subject.
*/
void
deselect_arts_subject(ap)
register ARTICLE* ap;
{
    if (ap->subj && ap->subj->articles)
	deselect_subject(ap->subj);
    else
	deselect_article(ap, 0);
}

/* Deselect all the articles in a subject.
*/
void
deselect_subject(subj)
SUBJECT* subj;
{
    register ARTICLE* ap;

    for (ap = subj->articles; ap; ap = ap->subj_next) {
	if (ap->flags & sel_mask) {
	    ap->flags &= ~sel_mask;
	    if (!selected_count--)
		selected_count = 0;
	}
    }
    if (subj->flags & sel_mask) {
	subj->flags &= ~sel_mask;
	selected_subj_cnt--;
    }
    subj->flags &= ~(SF_VISIT | SF_WASSELECTED);
    if (sel_rereading)
	subj->flags |= SF_DEL;
    else
	subj->flags &= ~SF_DEL;
}

/* Deselect this article's thread.
*/
void
deselect_arts_thread(ap)
register ARTICLE* ap;
{
    if (ap->subj && ap->subj->thread)
	deselect_thread(ap->subj->thread);
    else
	deselect_arts_subject(ap);
}

/* Deselect all the articles in a thread.
*/
void
deselect_thread(thread)
register ARTICLE* thread;
{
    register SUBJECT* sp;

    sp = thread->subj;
    do {
	deselect_subject(sp);
	sp = sp->thread_link;
    } while (sp != thread->subj);
}

/* Deselect everything.
*/
void
deselect_all()
{
    register SUBJECT* sp;

    for (sp = first_subject; sp; sp = sp->next)
	deselect_subject(sp);
    selected_count = selected_subj_cnt = 0;
    sel_page_sp = 0;
    sel_page_app = 0;
    sel_last_ap = 0;
    sel_last_sp = 0;
    selected_only = FALSE;
}

/* Kill all unread articles attached to this article's subject.
*/
void
kill_arts_subject(ap, auto_flags)
register ARTICLE* ap;
int auto_flags;
{
    if (ap->subj && ap->subj->articles)
	kill_subject(ap->subj, auto_flags);
    else {
	if (auto_flags & SET_TORETURN)
	    delay_unmark(ap);
	set_read(ap);
	auto_flags &= AUTO_KILLS;
	if (auto_flags)
	    change_auto_flags(ap, auto_flags);
    }
}

/* Kill all unread articles attached to the given subject.
*/
void
kill_subject(subj, auto_flags)
SUBJECT* subj;
int auto_flags;
{
    register ARTICLE* ap;
    register int killmask = (auto_flags & AFFECT_ALL)? 0 : sel_mask;
    char toreturn = (auto_flags & SET_TORETURN) != 0;

    auto_flags &= AUTO_KILLS;
    for (ap = subj->articles; ap; ap = ap->subj_next) {
	if (toreturn)
	    delay_unmark(ap);
	if ((ap->flags & (AF_UNREAD|killmask)) == AF_UNREAD)
	    set_read(ap);
	if (auto_flags)
	    change_auto_flags(ap, auto_flags);
    }
    subj->flags &= ~(SF_VISIT | SF_WASSELECTED);
}

/* Kill all unread articles attached to this article's thread.
*/
void
kill_arts_thread(ap, auto_flags)
register ARTICLE* ap;
int auto_flags;
{
    if (ap->subj && ap->subj->thread)
	kill_thread(ap->subj->thread, auto_flags);
    else
	kill_arts_subject(ap, auto_flags);
}

/* Kill all unread articles attached to the given thread.
*/
void
kill_thread(thread, auto_flags)
register ARTICLE* thread;
int auto_flags;
{
    register SUBJECT* sp;

    sp = thread->subj;
    do {
	kill_subject(sp, auto_flags);
	sp = sp->thread_link;
    } while (sp != thread->subj);
}

/* Kill the subthread attached to this article.
*/
void
kill_subthread(ap, auto_flags)
register ARTICLE* ap;
int auto_flags;
{
    register ARTICLE* limit;
    char toreturn = (auto_flags & SET_TORETURN) != 0;

    if (!ap)
	return;
    for (limit = ap; limit; limit = limit->parent) {
	if (limit->sibling) {
	    limit = limit->sibling;
	    break;
	}
    }

    auto_flags &= AUTO_KILLS;
    for (; ap != limit; ap = bump_art(ap)) {
	if (toreturn)
	    delay_unmark(ap);
	if (ALLBITS(ap->flags, AF_EXISTS | AF_UNREAD))
	    set_read(ap);
	if (auto_flags)
	    change_auto_flags(ap, auto_flags);
    }
}

/* Unkill all the articles attached to the given subject.
*/
void
unkill_subject(subj)
SUBJECT* subj;
{
    register ARTICLE* ap;
    int save_sel_count = selected_count;

    for (ap = subj->articles; ap; ap = ap->subj_next) {
	if (sel_rereading) {
	    if (ALLBITS(ap->flags, AF_DELSEL | AF_EXISTS)) {
		if (!(ap->flags & AF_UNREAD))
		    ngptr->toread++;
		if (selected_only && !(ap->flags & AF_SEL))
		    selected_count++;
		ap->flags = (ap->flags & ~AF_DELSEL) | AF_SEL|AF_UNREAD;
	    } else
		ap->flags &= ~(AF_DEL|AF_DELSEL);
	} else {
	    if ((ap->flags & (AF_EXISTS|AF_UNREAD)) == AF_EXISTS)
		onemore(ap);
	    if (selected_only && (ap->flags & (AF_SEL|AF_UNREAD)) == AF_UNREAD) {
		ap->flags = (ap->flags & ~AF_DEL) | AF_SEL;
		selected_count++;
	    }
	}
    }
    if (selected_count != save_sel_count
     && selected_only && !(subj->flags & SF_SEL)) {
	subj->flags |= SF_SEL | SF_VISIT | SF_WASSELECTED;
	selected_subj_cnt++;
    }
    subj->flags &= ~(SF_DEL|SF_DELSEL);
}

/* Unkill all the articles attached to the given thread.
*/
void
unkill_thread(thread)
register ARTICLE* thread;
{
    register SUBJECT* sp;

    sp = thread->subj;
    do {
	unkill_subject(sp);
	sp = sp->thread_link;
    } while (sp != thread->subj);
}

/* Unkill the subthread attached to this article.
*/
void
unkill_subthread(ap)
register ARTICLE* ap;
{
    register ARTICLE* limit;
    register SUBJECT* sp;

    if (!ap)
	return;
    for (limit = ap; limit; limit = limit->parent) {
	if (limit->sibling) {
	    limit = limit->sibling;
	    break;
	}
    }

    sp = ap->subj;
    for (; ap != limit; ap = bump_art(ap)) {
	if ((ap->flags & (AF_EXISTS|AF_UNREAD)) == AF_EXISTS)
	    onemore(ap);
	if (selected_only && !(ap->flags & AF_SEL)) {
	    ap->flags |= AF_SEL;
	    selected_count++;
	}
    }
    if (!(sp->flags & sel_mask))
	selected_subj_cnt++;
    sp->flags = (sp->flags & ~SF_DEL) | SF_SEL | SF_VISIT;
    selected_only = (selected_only || selected_count != 0);
}

/* Clear the auto flags in all unread articles attached to the given subject.
*/
void
clear_subject(subj)
SUBJECT* subj;
{
    register ARTICLE* ap;

    for (ap = subj->articles; ap; ap = ap->subj_next)
	clear_auto_flags(ap);
}

/* Clear the auto flags in all unread articles attached to the given thread.
*/
void
clear_thread(thread)
register ARTICLE* thread;
{
    register SUBJECT* sp;

    sp = thread->subj;
    do {
	clear_subject(sp);
	sp = sp->thread_link;
    } while (sp != thread->subj);
}

/* Clear the auto flags in the subthread attached to this article.
*/
void
clear_subthread(ap)
register ARTICLE* ap;
{
    register ARTICLE* limit;

    if (!ap)
	return;
    for (limit = ap; limit; limit = limit->parent) {
	if (limit->sibling) {
	    limit = limit->sibling;
	    break;
	}
    }

    for (; ap != limit; ap = bump_art(ap))
	clear_auto_flags(ap);
}

ARTICLE*
subj_art(sp)
SUBJECT* sp;
{
    register ARTICLE* ap = NULL;
    int art_mask = (selected_only? (AF_SEL|AF_UNREAD) : AF_UNREAD);
    bool TG_save = ThreadedGroup;

    ThreadedGroup = (sel_mode == SM_THREAD);
    ap = first_art(sp);
    while (ap && (ap->flags & art_mask) != art_mask)
	ap = next_art(ap);
    if (!ap) {
	reread = TRUE;
	ap = first_art(sp);
	if (selected_only) {
	    while (ap && !(ap->flags & AF_SEL))
		ap = next_art(ap);
	    if (!ap)
		ap = first_art(sp);
	}
    }
    ThreadedGroup = TG_save;
    return ap;
}

/* Find the next thread (first if art > lastart).  If articles are selected,
** only choose from threads with selected articles.
*/
void
visit_next_thread()
{
    register SUBJECT* sp;
    register ARTICLE* ap = artp;

    sp = (ap? ap->subj : NULL);
    while ((sp = next_subj(sp, SF_VISIT)) != NULL) {
	if ((ap = subj_art(sp)) != NULL) {
	    art = article_num(ap);
	    artp = ap;
	    return;
	}
	reread = FALSE;
    }
    artp = NULL;
    art = lastart+1;
    forcelast = TRUE;
}

/* Find previous thread (or last if artp == NULL).  If articles are selected,
** only choose from threads with selected articles.
*/
void
visit_prev_thread()
{
    register SUBJECT* sp;
    register ARTICLE* ap = artp;

    sp = (ap? ap->subj : NULL);
    while ((sp = prev_subj(sp, SF_VISIT)) != NULL) {
	if ((ap = subj_art(sp)) != NULL) {
	    art = article_num(ap);
	    artp = ap;
	    return;
	}
	reread = FALSE;
    }
    artp = NULL;
    art = lastart+1;
    forcelast = TRUE;
}

/* Find artp's parent or oldest ancestor.  Returns FALSE if no such
** article.  Sets art and artp otherwise.
*/
bool
find_parent(keep_going)
bool_int keep_going;
{
    register ARTICLE* ap = artp;

    if (!ap->parent)
	return FALSE;

    do {
	ap = ap->parent;
    } while (keep_going && ap->parent);

    artp = ap;
    art = article_num(ap);
    return TRUE;
}

/* Find artp's first child or youngest decendent.  Returns FALSE if no
** such article.  Sets art and artp otherwise.
*/
bool
find_leaf(keep_going)
bool_int keep_going;
{
    register ARTICLE* ap = artp;

    if (!ap->child1)
	return FALSE;

    do {
	ap = ap->child1;
    } while (keep_going && ap->child1);

    artp = ap;
    art = article_num(ap);
    return TRUE;
}

/* Find the next "sibling" of artp, including cousins that are the
** same distance down the thread as we are.  Returns FALSE if no such
** article.  Sets art and artp otherwise.
*/
bool
find_next_sib()
{
    ARTICLE* ta;
    ARTICLE* tb;
    int ascent;

    ascent = 0;
    ta = artp;
    for (;;) {
	while (ta->sibling) {
	    ta = ta->sibling;
	    if ((tb = first_sib(ta, ascent)) != NULL) {
		artp = tb;
		art = article_num(tb);
		return TRUE;
	    }
	}
	if (!(ta = ta->parent))
	    break;
	ascent++;
    }
    return FALSE;
}

/* A recursive routine to find the first node at the proper depth.  This
** article is at depth 0.
*/
static ARTICLE*
first_sib(ta, depth)
ARTICLE* ta;
int depth;
{
    ARTICLE* tb;

    if (!depth)
	return ta;

    for (;;) {
	if (ta->child1 && (tb = first_sib(ta->child1, depth-1)))
	    return tb;

	if (!ta->sibling)
	    return NULL;

	ta = ta->sibling;
    }
}

/* Find the previous "sibling" of artp, including cousins that are
** the same distance down the thread as we are.  Returns FALSE if no
** such article.  Sets art and artp otherwise.
*/
bool
find_prev_sib()
{
    ARTICLE* ta;
    ARTICLE* tb;
    int ascent;

    ascent = 0;
    ta = artp;
    for (;;) {
	tb = ta;
	if (ta->parent)
	    ta = ta->parent->child1;
	else
	    ta = ta->subj->thread;
	if ((tb = last_sib(ta, ascent, tb)) != NULL) {
	    artp = tb;
	    art = article_num(tb);
	    return TRUE;
	}
	if (!(ta = ta->parent))
	    break;
	ascent++;
    }
    return FALSE;
}

/* A recursive routine to find the last node at the proper depth.  This
** article is at depth 0.
*/
static ARTICLE*
last_sib(ta, depth, limit)
ARTICLE* ta;
int depth;
ARTICLE* limit;
{
    ARTICLE* tb;
    ARTICLE* tc;

    if (ta == limit)
	return NULL;

    if (ta->sibling) {
	tc = ta->sibling;
	if (tc != limit && (tb = last_sib(tc,depth,limit)))
	    return tb;
    }
    if (!depth)
	return ta;
    if (ta->child1)
	return last_sib(ta->child1, depth-1, limit);
    return NULL;
}

/* Get each subject's article count; count total articles and selected
** articles (use sel_rereading to determine whether to count read or
** unread articles); deselect any subjects we find that are empty if
** CS_UNSELECT or CS_UNSEL_STORE is specified.  If mode is CS_RESELECT
** is specified, the selections from the last CS_UNSEL_STORE are
** reselected.
*/
void
count_subjects(cmode)
int cmode;
{
    register int count, sel_count;
    register ARTICLE* ap;
    register SUBJECT* sp;
    int desired_flags = (sel_rereading? AF_EXISTS : (AF_EXISTS|AF_UNREAD));
    time_t subjdate;

    obj_count = selected_count = selected_subj_cnt = 0;
    if (last_cached >= lastart)
	firstart = lastart+1;

    switch (cmode) {
    case CS_RETAIN:
	break;
    case CS_UNSEL_STORE:
	for (sp = first_subject; sp; sp = sp->next) {
	    if (sp->flags & SF_VISIT)
		sp->flags = (sp->flags & ~SF_VISIT) | SF_OLDVISIT;
	    else
		sp->flags &= ~SF_OLDVISIT;
	}
	break;
    case CS_RESELECT:
	for (sp = first_subject; sp; sp = sp->next) {
	    if (sp->flags & SF_OLDVISIT)
		sp->flags |= SF_VISIT;
	    else
		sp->flags &= ~SF_VISIT;
	}
	break;
    default:
	for (sp = first_subject; sp; sp = sp->next)
	    sp->flags &= ~SF_VISIT;
    }

    for (sp = first_subject; sp; sp = sp->next) {
	subjdate = 0;
	count = sel_count = 0;
	for (ap = sp->articles; ap; ap = ap->subj_next) {
	    if ((ap->flags & (AF_EXISTS|AF_UNREAD)) == desired_flags) {
		count++;
		if (ap->flags & sel_mask)
		    sel_count++;
		if (!subjdate)
		    subjdate = ap->date;
		if (article_num(ap) < firstart)
		    firstart = article_num(ap);
	    }
	}
	sp->misc = count;
	if (subjdate)
	    sp->date = subjdate;
	else if (!sp->date && sp->articles)
	    sp->date = sp->articles->date;
	obj_count += count;
	if (cmode == CS_RESELECT) {
	    if (sp->flags & SF_VISIT) {
		sp->flags = (sp->flags & ~(SF_SEL|SF_DEL)) | sel_mask;
		selected_count += sel_count;
		selected_subj_cnt++;
	    } else
		sp->flags &= ~sel_mask;
	} else {
	    if (sel_count
	     && (cmode >= CS_UNSEL_STORE || (sp->flags & sel_mask))) {
		sp->flags = (sp->flags & ~(SF_SEL|SF_DEL)) | sel_mask;
		selected_count += sel_count;
		selected_subj_cnt++;
	    } else if (cmode >= CS_UNSELECT)
		sp->flags &= ~sel_mask;
	    else if (sp->flags & sel_mask) {
		sp->flags &= ~SF_DEL;
		selected_subj_cnt++;
	    }
	    if (count && (!selected_only || (sp->flags & sel_mask))) {
		sp->flags |= SF_VISIT;
	    }
	}
    }
    if (cmode != CS_RETAIN && cmode != CS_RESELECT
     && !obj_count && !selected_only) {
	for (sp = first_subject; sp; sp = sp->next)
	    sp->flags |= SF_VISIT;
    }
}

static int
subjorder_subject(spp1, spp2)
register SUBJECT** spp1;
register SUBJECT** spp2;
{
    return strcaseCMP((*spp1)->str+4, (*spp2)->str+4) * sel_direction;
}

static int
subjorder_date(spp1, spp2)
register SUBJECT** spp1;
register SUBJECT** spp2;
{
    time_t eq = (*spp1)->date - (*spp2)->date;
    return eq? eq > 0? sel_direction : -sel_direction : 0;
}

static int
subjorder_count(spp1, spp2)
register SUBJECT** spp1;
register SUBJECT** spp2;
{
    short eq = (*spp1)->misc - (*spp2)->misc;
    return eq? eq > 0? sel_direction : -sel_direction : subjorder_date(spp1,spp2);
}

static int
subjorder_lines(spp1, spp2)
register SUBJECT** spp1;
register SUBJECT** spp2;
{
    long eq, l1, l2;
    l1 = (*spp1)->articles? (*spp1)->articles->lines : 0;
    l2 = (*spp2)->articles? (*spp2)->articles->lines : 0;
    eq = l1 - l2;
    return eq? eq > 0? sel_direction : -sel_direction : subjorder_date(spp1,spp2);
}

#ifdef SCORE

/* for now, highest eligible article score */
static int
subject_score_high(sp)
SUBJECT* sp;
{
    ARTICLE* ap;
    int desired_flags = (sel_rereading? AF_EXISTS : (AF_EXISTS|AF_UNREAD));
    int hiscore = 0;
    int hiscore_found = 0;
    int sc;

    /* find highest score of desired articles */
    for (ap = sp->articles; ap; ap = ap->subj_next) {
	if ((ap->flags & (AF_EXISTS|AF_UNREAD)) == desired_flags) {
	    sc = sc_score_art(article_num(ap),FALSE);
	    if ((!hiscore_found) || (sc>hiscore)) {
		hiscore_found = 1;
		hiscore = sc;
	    }
	}
    }
    return hiscore;
}

/* later make a subject-thread score routine */
static int
subjorder_score(spp1, spp2)
register SUBJECT** spp1;
register SUBJECT** spp2;
{
    int sc1, sc2;
    sc1 = subject_score_high(*spp1);
    sc2 = subject_score_high(*spp2);

    if (sc1 != sc2)
	return (sc2 - sc1) * sel_direction;
    return subjorder_date(spp1, spp2);
}
#endif

static int
threadorder_subject(spp1, spp2)
SUBJECT** spp1;
SUBJECT** spp2;
{
    register ARTICLE* t1 = (*spp1)->thread;
    register ARTICLE* t2 = (*spp2)->thread;
    if (t1 != t2 && t1 && t2)
	return strcaseCMP(t1->subj->str+4, t2->subj->str+4) * sel_direction;
    return subjorder_date(spp1, spp2);
}

static int
threadorder_date(spp1, spp2)
SUBJECT** spp1;
SUBJECT** spp2;
{
    register ARTICLE* t1 = (*spp1)->thread;
    register ARTICLE* t2 = (*spp2)->thread;
    if (t1 != t2 && t1 && t2) {
	register SUBJECT* sp1;
	register SUBJECT* sp2;
	long eq;
	if (!(sp1 = t1->subj)->misc)
	    for (sp1=sp1->thread_link; sp1 != t1->subj; sp1=sp1->thread_link)
		if (sp1->misc)
		    break;
	if (!(sp2 = t2->subj)->misc)
	    for (sp2=sp2->thread_link; sp2 != t2->subj; sp2=sp2->thread_link)
		if (sp2->misc)
		    break;
	if (!(eq = sp1->date - sp2->date))
	    return strcaseCMP(sp1->str+4, sp2->str+4);
	return eq > 0? sel_direction : -sel_direction;
    }
    return subjorder_date(spp1, spp2);
}

static int
threadorder_count(spp1, spp2)
SUBJECT** spp1;
SUBJECT** spp2;
{
    register int size1 = (*spp1)->misc;
    register int size2 = (*spp2)->misc;
    if ((*spp1)->thread != (*spp2)->thread) {
	register SUBJECT* sp;
	for (sp = (*spp1)->thread_link; sp != *spp1; sp = sp->thread_link)
	    size1 += sp->misc;
	for (sp = (*spp2)->thread_link; sp != *spp2; sp = sp->thread_link)
	    size2 += sp->misc;
    }
    if (size1 != size2)
	return (size1 - size2) * sel_direction;
    return threadorder_date(spp1, spp2);
}

static int
threadorder_lines(spp1, spp2)
SUBJECT** spp1;
SUBJECT** spp2;
{
    register ARTICLE* t1 = (*spp1)->thread;
    register ARTICLE* t2 = (*spp2)->thread;
    if (t1 != t2 && t1 && t2) {
	register SUBJECT* sp1;
	register SUBJECT* sp2;
	long eq, l1, l2;
	if (!(sp1 = t1->subj)->misc) {
	    for (sp1=sp1->thread_link; sp1 != t1->subj; sp1=sp1->thread_link) {
		if (sp1->misc)
		    break;
	    }
	}
	if (!(sp2 = t2->subj)->misc) {
	    for (sp2=sp2->thread_link; sp2 != t2->subj; sp2=sp2->thread_link) {
		if (sp2->misc)
		    break;
	    }
	}
	l1 = sp1->articles? sp1->articles->lines : 0;
	l2 = sp2->articles? sp2->articles->lines : 0;
	if ((eq = l1 - l2) != 0)
	    return eq > 0? sel_direction : -sel_direction;
	eq = sp1->date - sp2->date;
	return eq? eq > 0? sel_direction : -sel_direction : 0;
    }
    return subjorder_date(spp1, spp2);
}

#ifdef SCORE

static int
thread_score_high(tp)
SUBJECT* tp;
{ 
    SUBJECT* sp;
    int hiscore = 0;
    int hiscore_found = 0;
    int sc;

    for (sp = tp->thread_link; ; sp = sp->thread_link) {
	sc = subject_score_high(sp);
	if ((!hiscore_found) || (sc>hiscore)) {
	    hiscore_found = 1;
	    hiscore = sc;
	}
	/* break *after* doing the last item */
	if (tp == sp)
	    break;
    }
    return hiscore;
}

static int
threadorder_score(spp1, spp2)
SUBJECT** spp1;
SUBJECT** spp2;
{
    int sc1, sc2;

    sc1 = sc2 = 0;

    if ((*spp1)->thread != (*spp2)->thread) {
	sc1 = thread_score_high(*spp1);
	sc2 = thread_score_high(*spp2);
    }
    if (sc1 != sc2)
	return (sc2 - sc1) * sel_direction;
    return threadorder_date(spp1, spp2);
}
#endif

/* Sort the subjects according to the chosen order.
*/
void
sort_subjects()
{
    register SUBJECT* sp;
    register int i;
    SUBJECT** lp;
    SUBJECT** subj_list;
    int (*sort_procedure)();

    /* If we don't have at least two subjects, we're done! */
    if (!first_subject || !first_subject->next)
	return;

    switch (sel_sort) {
      case SS_DATE:
      default:
	sort_procedure = (sel_mode == SM_THREAD?
			  threadorder_date : subjorder_date);
	break;
      case SS_STRING:
	sort_procedure = (sel_mode == SM_THREAD?
			  threadorder_subject : subjorder_subject);
	break;
      case SS_COUNT:
	sort_procedure = (sel_mode == SM_THREAD?
			  threadorder_count : subjorder_count);
	break;
      case SS_LINES:
	sort_procedure = (sel_mode == SM_THREAD?
			  threadorder_lines : subjorder_lines);
	break;
#ifdef SCORE
      /* if SCORE is undefined, use the default above */
      case SS_SCORE:
	sort_procedure = (sel_mode == SM_THREAD?
			  threadorder_score : subjorder_score);
	break;
#endif
    }

    subj_list = (SUBJECT**)safemalloc(subject_count * sizeof (SUBJECT*));
    for (lp = subj_list, sp = first_subject; sp; sp = sp->next)
	*lp++ = sp;
    assert(lp - subj_list == subject_count);

    qsort(subj_list, subject_count, sizeof (SUBJECT*), sort_procedure);

    first_subject = sp = subj_list[0];
    sp->prev = NULL;
    for (i = subject_count, lp = subj_list; --i; lp++) {
	lp[0]->next = lp[1];
	lp[1]->prev = lp[0];
	if (sel_mode == SM_THREAD) {
	    if (lp[0]->thread && lp[0]->thread == lp[1]->thread)
		lp[0]->thread_link = lp[1];
	    else {
		lp[0]->thread_link = sp;
		sp = lp[1];
	    }
	}
    }
    last_subject = lp[0];
    last_subject->next = NULL;
    if (sel_mode == SM_THREAD)
	last_subject->thread_link = sp;
    free((char*)subj_list);
}

static int
artorder_date(art1, art2)
register ARTICLE** art1;
register ARTICLE** art2;
{
    long eq = (*art1)->date - (*art2)->date;
    return eq? eq > 0? sel_direction : -sel_direction : 0;
}

static int
artorder_subject(art1, art2)
register ARTICLE** art1;
register ARTICLE** art2;
{
    if ((*art1)->subj == (*art2)->subj)
	return artorder_date(art1, art2);
    return strcaseCMP((*art1)->subj->str + 4, (*art2)->subj->str + 4)
	* sel_direction;
}

static int
artorder_author(art1, art2)
register ARTICLE** art1;
register ARTICLE** art2;
{
    int eq = strcaseCMP((*art1)->from, (*art2)->from);
    return eq? eq * sel_direction : artorder_date(art1, art2);
}

static int
artorder_number(art1, art2)
register ARTICLE** art1;
register ARTICLE** art2;
{
    ART_NUM eq = article_num(*art1) - article_num(*art2);
    return eq > 0? sel_direction : -sel_direction;
}

static int
artorder_groups(art1, art2)
register ARTICLE** art1;
register ARTICLE** art2;
{
    long eq;
#ifdef DEBUG
    assert((*art1)->subj != NULL);
    assert((*art2)->subj != NULL);
#endif
    if ((*art1)->subj == (*art2)->subj)
	return artorder_date(art1, art2);
    eq = (*art1)->subj->date - (*art2)->subj->date;
    return eq? eq > 0? sel_direction : -sel_direction : 0;
}

static int
artorder_lines(art1, art2)
register ARTICLE** art1;
register ARTICLE** art2;
{
    long eq = (*art1)->lines - (*art2)->lines;
    return eq? eq > 0? sel_direction : -sel_direction : artorder_date(art1,art2);
}

#ifdef SCORE
static int
artorder_score(art1, art2)
register ARTICLE** art1;
register ARTICLE** art2;
{
    int eq = sc_score_art(article_num(*art2),FALSE)
	   - sc_score_art(article_num(*art1),FALSE);
    return eq? eq > 0? sel_direction : -sel_direction : 0;
}
#endif

/* Sort the articles according to the chosen order.
*/
void
sort_articles()
{
    int (*sort_procedure)();

    build_artptrs();

    /* If we don't have at least two articles, we're done! */
    if (artptr_list_size < 2)
	return;

    switch (sel_sort) {
      case SS_DATE:
      default:
	sort_procedure = artorder_date;
	break;
      case SS_STRING:
	sort_procedure = artorder_subject;
	break;
      case SS_AUTHOR:
	sort_procedure = artorder_author;
	break;
      case SS_NATURAL:
	sort_procedure = artorder_number;
	break;
      case SS_GROUPS:
	sort_procedure = artorder_groups;
	break;
      case SS_LINES:
	sort_procedure = artorder_lines;
	break;
#ifdef SCORE
      case SS_SCORE:
	sort_procedure = artorder_score;
	break;
#endif
    }
    sel_page_app = 0;
    qsort(artptr_list, artptr_list_size, sizeof (ARTICLE*), sort_procedure);
}

static void
build_artptrs()
{
    ARTICLE** app;
    ARTICLE* ap;
    ART_NUM an;
    ART_NUM count = obj_count;
    int desired_flags = (sel_rereading? AF_EXISTS : (AF_EXISTS|AF_UNREAD));

    if (!artptr_list || artptr_list_size != count) {
	artptr_list = (ARTICLE**)saferealloc((char*)artptr_list,
		(MEM_SIZE)count * sizeof (ARTICLE*));
	artptr_list_size = count;
    }
    app = artptr_list;
    for (an = article_first(absfirst); count; an = article_next(an)) {
	ap = article_ptr(an);
	if ((ap->flags & (AF_EXISTS|AF_UNREAD)) == desired_flags) {
	    *app++ = ap;
	    count--;
	}
    }
}
