/* rt-page.c
*/
/* This software is copyrighted as detailed in the LICENSE file. */


#include "EXTERN.h"
#include "common.h"
#include "list.h"
#include "hash.h"
#include "cache.h"
#include "term.h"
#include "ngdata.h"
#include "nntpclient.h"
#include "datasrc.h"
#include "nntp.h"
#include "trn.h"
#include "env.h"
#include "util.h"
#include "util2.h"
#include "opt.h"
#include "only.h"
#include "addng.h"
#include "rcln.h"
#include "rcstuff.h"
#include "rthread.h"
#include "rt-select.h"
#include "rt-util.h"
#include "univ.h"
#include "color.h"
#include "INTERN.h"
#include "rt-page.h"
#include "rt-page.ih"

bool
set_sel_mode(ch)
char_int ch;
{
    switch (ch) {
      case 'a':
	set_selector(sel_defaultmode = SM_ARTICLE, 0);
	break;
      case 's':
	set_selector(sel_defaultmode = SM_SUBJECT, 0);
	break;
      case 't':
	if (in_ng && !ThreadedGroup) {
	    bool always_save = thread_always;
	    ThreadedGroup = TRUE;
	    thread_always = TRUE;
	    if (sel_rereading)
		firstart = absfirst;
	    printf("\nThreading the group. "), fflush(stdout);
	    termdown(1);
	    thread_open();
	    thread_always = always_save;
	    if (last_cached < lastart)
		ThreadedGroup = FALSE;
	}
	/* FALL THROUGH */
      case 'T':
	set_selector(sel_defaultmode = SM_THREAD, 0);
	break;
      default:
	set_selector(sel_defaultmode, 0);
	return FALSE;
    }
    return TRUE;
}

char*
get_sel_order(smode)
int smode;
{
    int save_sel_mode = sel_mode;
    set_selector(smode, 0);
    sprintf(buf,"%s%s", sel_direction < 0? "reverse " : nullstr,
	    sel_sort_string);
    sel_mode = save_sel_mode;
    set_selector(0, 0);
    return buf;
}

bool
set_sel_order(smode,str)
int smode;
char* str;
{
    bool reverse = 0;
    char ch;

    if (*str == 'r' || *str == 'R') {
	while (*str && *str != ' ') str++;
	while (*str == ' ') str++;
	reverse = 1;
    }

    ch = *str;
    if (reverse)
	ch = islower(ch)? toupper(ch) : ch;
    else
	ch = isupper(ch)? tolower(ch) : ch;

    return set_sel_sort(smode,ch);
}

bool
set_sel_sort(smode,ch)
int smode;
char_int ch;
{
    int save_sel_mode = sel_mode;
    int ssort;

    switch (ch) {
      case 'd':  case 'D':
	ssort = SS_DATE;
	break;
      case 's':  case 'S':
	ssort = SS_STRING;
	break;
      case 'a':  case 'A':
	ssort = SS_AUTHOR;
	break;
      case 'c':  case 'C':
	ssort = SS_COUNT;
	break;
      case 'n':  case 'N':
	ssort = SS_NATURAL;
	break;
      case 'g':  case 'G':
	ssort = SS_GROUPS;
	break;
      case 'l':  case 'L':
	ssort = SS_LINES;
	break;
      case 'p':  case 'P':
	ssort = SS_SCORE;
	break;
      default:
	return FALSE;
    }

    sel_mode = smode;
    if (isupper(ch))
	set_selector(0, -ssort);
    else
	set_selector(0, ssort);

    if (sel_mode != save_sel_mode) {
	sel_mode = save_sel_mode;
	set_selector(0, 0);
    }
    return TRUE;
}

void
set_selector(smode, ssort)
int smode;
int ssort;
{
    if (smode == 0) {
	if (sel_mode == SM_SUBJECT)
	    sel_mode = sel_threadmode;
	smode = sel_mode;
    }
    else
	sel_mode = smode;
    if (!ssort) {
	switch (sel_mode) {
	  case SM_MULTIRC:
	    ssort = SS_NATURAL;
	    break;
	  case SM_ADDGROUP:
	    ssort = sel_addgroupsort;
	    break;
	  case SM_NEWSGROUP:
	    ssort = sel_newsgroupsort;
	    break;
	  case SM_OPTIONS:
	    ssort = SS_NATURAL;
	    break;
	  case SM_THREAD:
	  case SM_SUBJECT:
	    ssort = sel_threadsort;
	    break;
	  case SM_ARTICLE:
	    ssort = sel_artsort;
	    break;
	  case SM_UNIVERSAL:
	    ssort = sel_univsort;
	    break;
	}
    }
    if (ssort > 0) {
	sel_direction = 1;
	sel_sort = ssort;
    }
    else {
	sel_direction = -1;
	sel_sort = -ssort;
    }

    if (sel_mode == SM_THREAD && !ThreadedGroup)
	sel_mode = SM_SUBJECT;

    switch (sel_mode) {
      case SM_MULTIRC:
	sel_mode_string = "a newsrc group";
	break;
      case SM_ADDGROUP:
	sel_mode_string = "a newsgroup to add";
	sel_addgroupsort = ssort;
	break;
      case SM_NEWSGROUP:
	sel_mode_string = "a newsgroup";
	sel_newsgroupsort = ssort;
	break;
      case SM_OPTIONS:
	sel_mode_string = "an option to change";
	break;
      case SM_UNIVERSAL:
	sel_mode_string = "an item";
	sel_univsort = ssort;
	break;
      case SM_THREAD:
	sel_mode_string = "threads";
	sel_threadmode = smode;
	sel_threadsort = ssort;
	goto thread_subj_sort;
      case SM_SUBJECT:
	sel_mode_string = "subjects";
	sel_threadmode = smode;
	sel_threadsort = ssort;
     thread_subj_sort:
	if (sel_sort == SS_AUTHOR || sel_sort == SS_GROUPS
	 || sel_sort == SS_NATURAL)
	    sel_sort = SS_DATE;
	break;
      case SM_ARTICLE:
	sel_mode_string = "articles";
	sel_artsort = ssort;
	if (sel_sort == SS_COUNT)
	    sel_sort = SS_DATE;
	break;
    }

    switch (sel_sort) {
      case SS_DATE:
	sel_sort_string = "date";
	break;
      case SS_STRING:
	sel_sort_string = "subject";
	break;
      case SS_AUTHOR:
	sel_sort_string = "author";
	break;
      case SS_COUNT:
	sel_sort_string = "count";
	break;
      case SS_LINES:
	sel_sort_string = "lines";
	break;
      case SS_NATURAL:
	sel_sort_string = "natural";
	break;
      case SS_GROUPS:
	if (sel_mode == SM_NEWSGROUP)
	    sel_sort_string = "group name";
	else
	    sel_sort_string = "SubjDate";
	break;
      case SS_SCORE:
	sel_sort_string = "points";
	break;
    }
}

static void
sel_page_init()
{
    sel_max_line_cnt = tc_LINES - (tc_COLS - mousebar_width < 50? 6 : 5);
    sel_chars = getval("SELECTCHARS", SELECTCHARS);
    /* The numeric option of up to 99 lines will require many adaptations
     * to be able to switch from a large numeric page (more than
     * strlen(sel_chars) lines) to an alphanumeric page. XXX
     */
    if (UseSelNum)
        sel_max_per_page = 99;
    else
	sel_max_per_page = strlen(sel_chars);
    if (sel_max_per_page > MAX_SEL)
	sel_max_per_page = MAX_SEL;
    if (sel_max_per_page > sel_max_line_cnt)
	sel_max_per_page = sel_max_line_cnt;
    sel_page_obj_cnt = 0;
    sel_page_item_cnt = 0;
}

void
init_pages(fill_last_page)
bool_int fill_last_page;
{
    SEL_UNION no_search;
    no_search.op = -1;
    sel_page_init();
try_again:
    sel_prior_obj_cnt = sel_total_obj_cnt = 0;

    switch (sel_mode) {
      case SM_MULTIRC: {
	MULTIRC* mp;
	for (mp = multirc_low(); mp; mp = multirc_next(mp)) {
	    if (mp->first) {
		mp->flags |= MF_INCLUDED;
		sel_total_obj_cnt++;
	    }
	    else
		mp->flags &= ~MF_INCLUDED;
	}
	if (sel_page_mp == NULL)
	    (void) first_page();
	break;
      }
      case SM_NEWSGROUP: {
	NGDATA* np;
	bool save_the_rest = FALSE;
	group_init_done = TRUE;
	sort_newsgroups();
	selected_count = 0;
	obj_count = 0;
	for (np = first_ng; np; np = np->next) {
	    if (sel_page_np == np)
		sel_prior_obj_cnt = sel_total_obj_cnt;
	    np->flags &= ~NF_INCLUDED;
	    if (np->toread < TR_NONE)
		continue;
	    if (!inlist(np->rcline))
		continue;
	    if (np->abs1st)
		;
	    else if (save_the_rest) {
		group_init_done = FALSE;
		np->toread = !sel_rereading;
	    }
	    else {
		/*ngptr = np; ??*/
		/*set_ngname(np->rcline);*/
		set_toread(np, ST_LAX);
		if (!np->rc->datasrc->act_sf.fp)
		    save_the_rest = (sel_rereading ^ (np->toread > TR_NONE));
	    }
	    if (paranoid) {
		current_ng = ngptr = np;
		/* this may move newsgroups around */
		cleanup_newsrc(np->rc);
		goto try_again;
	    }
	    if (!(np->flags & sel_mask)
	     && (sel_rereading? np->toread != TR_NONE
			      : np->toread < ng_min_toread))
		continue;
	    obj_count++;

	    if (!sel_exclusive || (np->flags & sel_mask)) {
		if (np->flags & sel_mask)
		    selected_count++;
		else if (sel_rereading)
		    np->flags |= NF_DEL;
		np->flags |= NF_INCLUDED;
		sel_total_obj_cnt++;
	    }
	}
	if (!sel_total_obj_cnt) {
	    if (sel_exclusive) {
		sel_exclusive = FALSE;
		sel_page_np = NULL;
		goto try_again;
	    }
	    if (maxngtodo) {
		end_only();
		fputs(msg, stdout);
		newline();
		if (fill_last_page)
		    get_anything();
		sel_page_np = NULL;
		goto try_again;
	    }
	}
	if (!sel_page_np)
	    (void) first_page();
	else if (sel_page_np == last_ng)
	    (void) last_page();
	else if (sel_prior_obj_cnt && fill_last_page) {
	    calc_page(no_search);
	    if (sel_prior_obj_cnt + sel_page_obj_cnt == sel_total_obj_cnt)
		(void) last_page();
	}
	break;
      }
      case SM_ADDGROUP: {
	ADDGROUP* gp;
	sort_addgroups();
	obj_count = 0;
	for (gp = first_addgroup; gp; gp = gp->next) {
	    if (sel_page_gp == gp)
		sel_prior_obj_cnt = sel_total_obj_cnt;
	    gp->flags &= ~AGF_INCLUDED;
	    if (!sel_rereading ^ !(gp->flags & AGF_EXCLUDED))
		continue;
	    if (!sel_exclusive || (gp->flags & sel_mask)) {
		if (sel_rereading && !(gp->flags & sel_mask))
		    gp->flags |= AGF_DEL;
		gp->flags |= AGF_INCLUDED;
		sel_total_obj_cnt++;
	    }
	    obj_count++;
	}
	if (!sel_total_obj_cnt && sel_exclusive) {
	    sel_exclusive = FALSE;
	    sel_page_gp = NULL;
	    goto try_again;
	}
	if (sel_page_gp == NULL)
	    (void) first_page();
	else if (sel_page_gp == last_addgroup)
	    (void) last_page();
	else if (sel_prior_obj_cnt && fill_last_page) {
	    calc_page(no_search);
	    if (sel_prior_obj_cnt + sel_page_obj_cnt == sel_total_obj_cnt)
		(void) last_page();
	}
	break;
      }
      case SM_UNIVERSAL: {
	UNIV_ITEM* ui;
	bool ui_elig;
	obj_count = 0;

	sort_univ();
	for (ui = first_univ; ui; ui = ui->next) {
	    if (sel_page_univ == ui)
		sel_prior_obj_cnt = sel_total_obj_cnt;
	    ui->flags &= ~UF_INCLUDED;
	    ui_elig = TRUE;
	    switch (ui->type) {
	      case UN_GROUP_DESEL:
	      case UN_VGROUP_DESEL:
	      case UN_DELETED:
	      case UN_VGROUP:		    /* first-pass item */
		/* always ineligible items */
		ui_elig = FALSE;
		break;
	      case UN_NEWSGROUP: {
		NGDATA* np;
		if (!ui->data.group.ng) {
		    ui_elig = FALSE;
		    break;
		}
		np = find_ng(ui->data.group.ng);
		if (!np) {
		    ui_elig = FALSE;
		    break;
		}
		if (!np->abs1st) {
		    toread_quiet = TRUE;
		    set_toread(np, ST_LAX);
		    toread_quiet = FALSE;
		}
		if (!(sel_rereading ^ (np->toread>TR_NONE))) {
		    ui_elig = FALSE;
		}
		break;
	      }
	      case UN_ARTICLE:
		/* later: use the datasrc of the newsgroup */
		ui_elig = !was_read_group(datasrc, ui->data.virt.num,
					  ui->data.virt.ng);
		if (sel_rereading)
		    ui_elig = !ui_elig;
		break;
	      default:
		ui_elig = !sel_rereading;
		break;
	    }
	    if (!ui_elig)
	        continue;
	    if (!sel_exclusive || (ui->flags & sel_mask)) {
		if (sel_rereading && !(ui->flags & sel_mask))
		    ui->flags |= UF_DEL;
		ui->flags |= UF_INCLUDED;
		sel_total_obj_cnt++;
	    }
	    obj_count++;
	}
	if (!sel_total_obj_cnt && sel_exclusive) {
	    sel_exclusive = FALSE;
	    sel_page_univ = NULL;
	    goto try_again;
	}
	if (sel_page_univ == NULL)
	    (void) first_page();
	else if (sel_page_univ == last_univ)
	    (void) last_page();
	else if (sel_prior_obj_cnt && fill_last_page) {
	    calc_page(no_search);
	    if (sel_prior_obj_cnt + sel_page_obj_cnt == sel_total_obj_cnt)
		(void) last_page();
	}
	break;
      }
      case SM_OPTIONS: {
	int op;
	int included = 0;
	obj_count = 0;
	for (op = 1; options_ini[op].checksum; op++) {
	    if (sel_page_op == op)
		sel_prior_obj_cnt = sel_total_obj_cnt;
	    if (*options_ini[op].item == '*') {
		included = (option_flags[op] & OF_SEL);
		sel_total_obj_cnt++;
		option_flags[op] |= OF_INCLUDED;
	    }
	    else if (included) {
		sel_total_obj_cnt++;
		option_flags[op] |= OF_INCLUDED;
	    }
	    else
		option_flags[op] &= ~OF_INCLUDED;
	    obj_count++;
	}
#if 0
	if (!sel_total_obj_cnt && sel_exclusive) {
	    sel_exclusive = FALSE;
	    sel_page_op = NULL;
	    goto try_again;
	}
#endif
	if (sel_page_op < 1)
	    (void) first_page();
	else if (sel_page_op >= obj_count)
	    (void) last_page();
	else if (sel_prior_obj_cnt && fill_last_page) {
	    calc_page(no_search);
	    if (sel_prior_obj_cnt + sel_page_obj_cnt == sel_total_obj_cnt)
		(void) last_page();
	}
	break;
      }
      case SM_ARTICLE: {
	ARTICLE* ap;
	ARTICLE** app;
	ARTICLE** limit;

	if (sel_page_app) {
	    int desired_flags = (sel_rereading? AF_EXISTS:(AF_EXISTS|AF_UNREAD));
	    limit = artptr_list + artptr_list_size;
	    ap = NULL;
	    for (app = sel_page_app; app < limit; app++) {
		ap = *app;
		if ((ap->flags & (AF_EXISTS|AF_UNREAD)) == desired_flags)
		    break;
	    }
	    sort_articles();
	    if (ap == NULL)
		sel_page_app = artptr_list + artptr_list_size;
	    else {
		for (app = artptr_list; app < limit; app++) {
		    if (*app == ap) {
			sel_page_app = app;
			break;
		    }
		}
	    }
	} else
	    sort_articles();

	while (sel_page_sp && sel_page_sp->misc == 0)
	    sel_page_sp = sel_page_sp->next;
	/* The artptr_list contains only unread or read articles, never both */
	limit = artptr_list + artptr_list_size;
	for (app = artptr_list; app < limit; app++) {
	    ap = *app;
	    if (sel_rereading && !(ap->flags & sel_mask))
		ap->flags |= AF_DEL;
	    if (sel_page_app == app
	     || (!sel_page_app && ap->subj == sel_page_sp)) {
		sel_page_app = app;
		sel_prior_obj_cnt = sel_total_obj_cnt;
	    }
	    if (!sel_exclusive || (ap->flags & sel_mask)) {
		sel_total_obj_cnt++;
		ap->flags |= AF_INCLUDED;
	    } else
		ap->flags &= ~AF_INCLUDED;
	}
	if (sel_exclusive && !sel_total_obj_cnt) {
	    sel_exclusive = FALSE;
	    sel_page_app = NULL;
	    goto try_again;
	}
	if (!sel_page_app)
	    (void) first_page();
	else if (sel_page_app >= limit)
	    (void) last_page();
	else if (sel_prior_obj_cnt && fill_last_page) {
	    calc_page(no_search);
	    if (sel_prior_obj_cnt + sel_page_obj_cnt == sel_total_obj_cnt)
		(void) last_page();
	}
	break;
      }
      default: {
	SUBJECT* sp;
	SUBJECT* group_sp;
	int group_arts;

	if (sel_page_sp) {
	    while (sel_page_sp && sel_page_sp->misc == 0)
		sel_page_sp = sel_page_sp->next;
	    sort_subjects();
	    if (!sel_page_sp)
		sel_page_sp = last_subject;
	} else
	    sort_subjects();
	for (sp = first_subject; sp; sp = sp->next) {
	    if (sel_rereading && !(sp->flags & sel_mask))
		sp->flags |= SF_DEL;

	    group_sp = sp;
	    group_arts = sp->misc;

	    if (!sel_exclusive || (sp->flags & sel_mask))
		sp->flags |= SF_INCLUDED;
	    else
		sp->flags &= ~SF_INCLUDED;

	    if (sel_page_sp == group_sp)
		sel_prior_obj_cnt = sel_total_obj_cnt;
	    if (sel_mode == SM_THREAD) {
		while (sp->next && sp->next->thread == sp->thread) {
		    sp = sp->next;
		    if (sp == sel_page_sp) {
			sel_prior_obj_cnt = sel_total_obj_cnt;
			sel_page_sp = group_sp;
		    }
		    sp->flags &= ~SF_INCLUDED;
		    if (sp->flags & sel_mask)
			group_sp->flags |= SF_INCLUDED;
		    else if (sel_rereading)
			sp->flags |= SF_DEL;
		    group_arts += sp->misc;
		}
	    }
	    if (group_sp->flags & SF_INCLUDED)
		sel_total_obj_cnt += group_arts;
	}
	if (sel_exclusive && !sel_total_obj_cnt) {
	    sel_exclusive = FALSE;
	    sel_page_sp = NULL;
	    goto try_again;
	}
	if (!sel_page_sp)
	    (void) first_page();
	else if (sel_page_sp == last_subject)
	    (void) last_page();
	else if (sel_prior_obj_cnt && fill_last_page) {
	    calc_page(no_search);
	    if (sel_prior_obj_cnt + sel_page_obj_cnt == sel_total_obj_cnt)
		(void) last_page();
	}
      }
    }
}

bool
first_page()
{
    sel_prior_obj_cnt = 0;

    switch (sel_mode) {
      case SM_MULTIRC: {
	MULTIRC* mp;
	for (mp = multirc_low(); mp; mp = multirc_next(mp)) {
	    if (mp->flags & MF_INCLUDED) {
		if (sel_page_mp != mp) {
		    sel_page_mp = mp;
		    return TRUE;
		}
		break;
	    }
	}
	break;
      }
      case SM_NEWSGROUP: {
	NGDATA* np;
	for (np = first_ng; np; np = np->next) {
	    if (np->flags & NF_INCLUDED) {
		if (sel_page_np != np) {
		    sel_page_np = np;
		    return TRUE;
		}
		break;
	    }
	}
	break;
      }
      case SM_ADDGROUP: {
	ADDGROUP* gp;
	for (gp = first_addgroup; gp; gp = gp->next) {
	    if (gp->flags & AGF_INCLUDED) {
		if (sel_page_gp != gp) {
		    sel_page_gp = gp;
		    return TRUE;
		}
		break;
	    }
	}
	break;
      }
      case SM_UNIVERSAL: {
	UNIV_ITEM* ui;
	for (ui = first_univ; ui; ui = ui->next) {
	    if (ui->flags & UF_INCLUDED) {
		if (sel_page_univ != ui) {
		    sel_page_univ = ui;
		    return TRUE;
		}
		break;
	    }
	}
	break;
      }
      case SM_OPTIONS: {
	if (sel_page_op != 1) {
	    sel_page_op = 1;
	    return TRUE;
	}
	break;
      }
      case SM_ARTICLE: {
	ARTICLE** app;
	ARTICLE** limit;

	limit = artptr_list + artptr_list_size;
	for (app = artptr_list; app < limit; app++) {
	    if ((*app)->flags & AF_INCLUDED) {
		if (sel_page_app != app) {
		    sel_page_app = app;
		    return TRUE;
		}
		break;
	    }
	}
	break;
      }
      default: {
	SUBJECT* sp;

	for (sp = first_subject; sp; sp = sp->next) {
	    if (sp->flags & SF_INCLUDED) {
		if (sel_page_sp != sp) {
		    sel_page_sp = sp;
		    return TRUE;
		}
		break;
	    }
	}
	break;
      }
    }
    return FALSE;
}

bool
last_page()
{
    sel_prior_obj_cnt = sel_total_obj_cnt;

    switch (sel_mode) {
      case SM_MULTIRC: {
	MULTIRC* mp = sel_page_mp;
	sel_page_mp = NULL;
	if (!prev_page())
	    sel_page_mp = mp;
	else if (mp != sel_page_mp)
	    return TRUE;
	break;
      }
      case SM_NEWSGROUP: {
	NGDATA* np = sel_page_np;
	sel_page_np = NULL;
	if (!prev_page())
	    sel_page_np = np;
	else if (np != sel_page_np)
	    return TRUE;
	break;
      }
      case SM_ADDGROUP: {
	ADDGROUP* gp = sel_page_gp;
	sel_page_gp = NULL;
	if (!prev_page())
	    sel_page_gp = gp;
	else if (gp != sel_page_gp)
	    return TRUE;
	break;
      }
      case SM_UNIVERSAL: {
	UNIV_ITEM* ui = sel_page_univ;
	sel_page_univ = NULL;
	if (!prev_page())
	    sel_page_univ = ui;
	else if (ui != sel_page_univ)
	    return TRUE;
	break;
      }
      case SM_OPTIONS: {
	int op = sel_page_op;
	sel_page_op = obj_count+1;
	if (!prev_page())
	    sel_page_op = op;
	else if (op != sel_page_op)
	    return TRUE;
	break;
      }
      case SM_ARTICLE: {
	ARTICLE** app = sel_page_app;
	sel_page_app = artptr_list + artptr_list_size;
	if (!prev_page())
	    sel_page_app = app;
	else if (app != sel_page_app)
	    return TRUE;
	break;
      }
      default: {
	SUBJECT* sp = sel_page_sp;
	sel_page_sp = NULL;
	if (!prev_page())
	    sel_page_sp = sp;
	else if (sp != sel_page_sp)
	    return TRUE;
	break;
      }
    }
    return FALSE;
}

bool
next_page()
{
    switch (sel_mode) {
      case SM_MULTIRC: {
	if (sel_next_mp) {
	    sel_page_mp = sel_next_mp;
	    sel_prior_obj_cnt += sel_page_obj_cnt;
	    return TRUE;
	}
	break;
      }
      case SM_NEWSGROUP: {
	if (sel_next_np) {
	    sel_page_np = sel_next_np;
	    sel_prior_obj_cnt += sel_page_obj_cnt;
	    return TRUE;
	}
	break;
      }
      case SM_ADDGROUP: {
	if (sel_next_gp) {
	    sel_page_gp = sel_next_gp;
	    sel_prior_obj_cnt += sel_page_obj_cnt;
	    return TRUE;
	}
	break;
      }
      case SM_UNIVERSAL: {
	if (sel_next_univ) {
	    sel_page_univ = sel_next_univ;
	    sel_prior_obj_cnt += sel_page_obj_cnt;
	    return TRUE;
	}
	break;
      }
      case SM_OPTIONS: {
	if (sel_next_op <= obj_count) {
	    sel_page_op = sel_next_op;
	    sel_prior_obj_cnt += sel_page_obj_cnt;
	    return TRUE;
	}
	break;
      }
      case SM_ARTICLE: {
	if (sel_next_app < artptr_list + artptr_list_size) {
	    sel_page_app = sel_next_app;
	    sel_prior_obj_cnt += sel_page_obj_cnt;
	    return TRUE;
	}
	break;
      }
      default: {
	if (sel_next_sp) {
	    sel_page_sp = sel_next_sp;
	    sel_prior_obj_cnt += sel_page_obj_cnt;
	    return TRUE;
	}
	break;
      }
    }
    return FALSE;
}

bool
prev_page()
{
    int item_cnt = 0;

    /* Scan the items in reverse to go back a page */
    switch (sel_mode) {
      case SM_MULTIRC: {
	MULTIRC* mp = sel_page_mp;
	MULTIRC* page_mp = sel_page_mp;

	if (!mp)
	    mp = multirc_high();
	else
	    mp = multirc_prev(multirc_ptr(mp->num));
	while (mp) {
	    if (mp->flags & MF_INCLUDED) {
		page_mp = mp;
		sel_prior_obj_cnt--;
		if (++item_cnt >= sel_max_per_page)
		    break;
	    }
	    mp = multirc_prev(mp);
	}
	if (sel_page_mp != page_mp) {
	    sel_page_mp = page_mp;
	    return TRUE;
	}
	break;
      }
      case SM_NEWSGROUP: {
	NGDATA* np = sel_page_np;
	NGDATA* page_np = sel_page_np;
	
	if (!np)
	    np = last_ng;
	else
	    np = np->prev;
	while (np) {
	    if (np->flags & NF_INCLUDED) {
		page_np = np;
		sel_prior_obj_cnt--;
		if (++item_cnt >= sel_max_per_page)
		    break;
	    }
	    np = np->prev;
	}
	if (sel_page_np != page_np) {
	    sel_page_np = page_np;
	    return TRUE;
	}
	break;
      }
      case SM_ADDGROUP: {
	ADDGROUP* gp = sel_page_gp;
	ADDGROUP* page_gp = sel_page_gp;

	if (!gp)
	    gp = last_addgroup;
	else
	    gp = gp->prev;
	while (gp) {
	    if (gp->flags & AGF_INCLUDED) {
		page_gp = gp;
		sel_prior_obj_cnt--;
		if (++item_cnt >= sel_max_per_page)
		    break;
	    }
	    gp = gp->prev;
	}
	if (sel_page_gp != page_gp) {
	    sel_page_gp = page_gp;
	    return TRUE;
	}
	break;
      }
      case SM_UNIVERSAL: {
	UNIV_ITEM* ui = sel_page_univ;
	UNIV_ITEM* page_ui = sel_page_univ;

	if (!ui)
	    ui = last_univ;
	else
	    ui = ui->prev;
	while (ui) {
	    if (ui->flags & UF_INCLUDED) {
		page_ui = ui;
		sel_prior_obj_cnt--;
		if (++item_cnt >= sel_max_per_page)
		    break;
	    }
	    ui = ui->prev;
	}
	if (sel_page_univ != page_ui) {
	    sel_page_univ = page_ui;
	    return TRUE;
	}
	break;
      }
      case SM_OPTIONS: {
	int op = sel_page_op;
	int page_op = sel_page_op;

	while (--op > 0) {
	    if (option_flags[op] & OF_INCLUDED) {
		page_op = op;
		sel_prior_obj_cnt--;
		if (++item_cnt >= sel_max_per_page)
		    break;
	    }
	}
	if (sel_page_op != page_op) {
	    sel_page_op = page_op;
	    return TRUE;
	}
	break;
      }
      case SM_ARTICLE: {
	ARTICLE** app;
	ARTICLE** page_app = sel_page_app;

	for (app = sel_page_app; --app >= artptr_list; ) {
	    if ((*app)->flags & AF_INCLUDED) {
		page_app = app;
		sel_prior_obj_cnt--;
		if (++item_cnt >= sel_max_per_page)
		    break;
	    }
	}
	if (sel_page_app != page_app) {
	    sel_page_app = page_app;
	    return TRUE;
	}
      }
      default: {
	SUBJECT* sp;
	SUBJECT* page_sp = sel_page_sp;
	int line_cnt, item_arts, line;

	line = 2;
	for (sp = (!page_sp? last_subject : page_sp->prev); sp; sp=sp->prev) {
	    item_arts = sp->misc;
	    if (sel_mode == SM_THREAD) {
		while (sp->prev && sp->prev->thread == sp->thread) {
		    sp = sp->prev;
		    item_arts += sp->misc;
		}
		line_cnt = count_thread_lines(sp, (int*)NULL);
	    } else
		line_cnt = count_subject_lines(sp, (int*)NULL);
	    if (!(sp->flags & SF_INCLUDED) || !line_cnt)
		continue;
	    if (line_cnt > sel_max_line_cnt)
		line_cnt = sel_max_line_cnt;
	    line += line_cnt;
	    if (line > sel_max_line_cnt + 2) {
		sp = page_sp;
		break;
	    }
	    sel_prior_obj_cnt -= item_arts;
	    page_sp = sp;
	    if (++item_cnt >= sel_max_per_page)
		break;
	}
	if (sel_page_sp != page_sp) {
	    sel_page_sp = (page_sp? page_sp : first_subject);
	    return TRUE;
	}
	break;
      }
    }
    return FALSE;
}

/* Return TRUE if we had to change pages to find the object */
bool
calc_page(u)
SEL_UNION u;
{
    int ret = FALSE;
    if (u.op != -1)
	sel_item_index = -1;
try_again:
    sel_page_obj_cnt = 0;
    sel_page_item_cnt = 0;
    term_line = 2;

    switch (sel_mode) {
      case SM_MULTIRC: {
	MULTIRC* mp = sel_page_mp;
	if (mp)
	    (void) multirc_ptr(mp->num);
	for (; mp && sel_page_item_cnt<sel_max_per_page; mp=multirc_next(mp)) {
	    if (mp == u.mp)
		sel_item_index = sel_page_item_cnt;
	    if (mp->flags & MF_INCLUDED)
		sel_page_item_cnt++;
	}
	sel_page_obj_cnt = sel_page_item_cnt;
	sel_next_mp = mp;
	break;
      }
      case SM_NEWSGROUP: {
	NGDATA* np = sel_page_np;
	for (; np && sel_page_item_cnt < sel_max_per_page; np = np->next) {
	    if (np == u.np)
		sel_item_index = sel_page_item_cnt;
	    if (np->flags & NF_INCLUDED)
		sel_page_item_cnt++;
	}
	sel_page_obj_cnt = sel_page_item_cnt;
	sel_next_np = np;
	break;
      }
      case SM_ADDGROUP: {
	ADDGROUP* gp = sel_page_gp;
	for (; gp && sel_page_item_cnt < sel_max_per_page; gp = gp->next) {
	    if (gp == u.gp)
		sel_item_index = sel_page_item_cnt;
	    if (gp->flags & AGF_INCLUDED)
		sel_page_item_cnt++;
	}
	sel_page_obj_cnt = sel_page_item_cnt;
	sel_next_gp = gp;
	break;
      }
      case SM_UNIVERSAL: {
	UNIV_ITEM* ui = sel_page_univ;
	for (; ui && sel_page_item_cnt < sel_max_per_page; ui = ui->next) {
	    if (ui == u.un)
		sel_item_index = sel_page_item_cnt;
	    if (ui->flags & UF_INCLUDED)
		sel_page_item_cnt++;
	}
	sel_page_obj_cnt = sel_page_item_cnt;
	sel_next_univ = ui;
	break;
      }
      case SM_OPTIONS: {
	int op = sel_page_op;
	for (; op <= obj_count && sel_page_item_cnt < sel_max_per_page; op++) {
	    if (op == u.op)
		sel_item_index = sel_page_item_cnt;
	    if (option_flags[op] & OF_INCLUDED)
		sel_page_item_cnt++;
	}
	sel_page_obj_cnt = sel_page_item_cnt;
	sel_next_op = op;
	break;
      }
      case SM_ARTICLE: {
	ARTICLE** app = sel_page_app;
	ARTICLE** limit = artptr_list + artptr_list_size;
	for (; app < limit && sel_page_item_cnt < sel_max_per_page; app++) {
	    if (*app == u.ap)
		sel_item_index = sel_page_item_cnt;
	    if ((*app)->flags & AF_INCLUDED)
		sel_page_item_cnt++;
	}
	sel_page_obj_cnt = sel_page_item_cnt;
	sel_next_app = app;
	break;
      }
      default: {
	SUBJECT* sp = sel_page_sp;
	int line_cnt, sel;
	for (; sp && sel_page_item_cnt < sel_max_per_page; sp = sp->next) {
	    if (sp == u.sp)
		sel_item_index = sel_page_item_cnt;
	    if (sp->flags & SF_INCLUDED) {
		if (sel_mode == SM_THREAD)
		    line_cnt = count_thread_lines(sp, &sel);
		else
		    line_cnt = count_subject_lines(sp, &sel);
		if (line_cnt) {
		    if (line_cnt > sel_max_line_cnt)
			line_cnt = sel_max_line_cnt;
		    if (term_line + line_cnt > sel_max_line_cnt+2)
			break;
		    sel_page_obj_cnt += sp->misc;
		    sel_page_item_cnt++;
		}
	    } else
		line_cnt = 0;
	    if (sel_mode == SM_THREAD) {
		while (sp->next && sp->next->thread == sp->thread) {
		    sp = sp->next;
		    if (!line_cnt || !sp->misc)
			continue;
		    sel_page_obj_cnt += sp->misc;
		}
	    }
	}
	sel_next_sp = sp;
	break;
      }
    }
    if (u.op != -1 && sel_item_index < 0) {
	if (!ret) {
	    if (first_page()) {
		ret = TRUE;
		goto try_again;
	    }
	}
	if (next_page()) {
	    ret = TRUE;
	    goto try_again;
	}
	first_page();
	sel_item_index = 0;
    }
    return ret;
}

void
display_page_title(home_only)
bool_int home_only;
{
    if (home_only || (erase_screen && erase_each_line))
	home_cursor();
    else
	clear();
    if (sel_mode == SM_MULTIRC)
	color_string(COLOR_HEADING,"Newsrcs");
    else if (sel_mode == SM_OPTIONS)
	color_string(COLOR_HEADING,"Options");
    else if (sel_mode == SM_UNIVERSAL) {
	color_object(COLOR_HEADING, 1);
	printf("[%d] %s",univ_level,
	       univ_title? univ_title : "Universal Selector");
	color_pop();
    }
    else if (in_ng)
	color_string(COLOR_NGNAME, ngname);
    else if (sel_mode == SM_ADDGROUP)
	color_string(COLOR_HEADING,"ADD newsgroups");
    else {
	int len;
	NEWSRC* rp;
	color_object(COLOR_HEADING, 1);
	printf("Newsgroups");
	for (rp = multirc->first, len = 0; rp && len < 34; rp = rp->next) {
	    if (rp->flags & RF_ACTIVE) {
		sprintf(buf+len, ", %s", rp->datasrc->name);
		len += strlen(buf+len);
	    }
	}
	if (rp)
	    strcpy(buf+len, ", ...");
	if (strNE(buf+2,"default"))
	    printf(" (group #%d: %s)",multirc->num, buf+2);
	color_pop();	/* of COLOR_HEADING */
    }
    if (home_only || (erase_screen && erase_each_line))
	erase_eol();
    if (sel_mode == SM_MULTIRC)
	;
    else if (sel_mode == SM_UNIVERSAL)
	;
    else if (sel_mode == SM_OPTIONS)
	printf("      (Press 'S' to save changes, 'q' to abandon, or TAB to use.)");
    else if (in_ng) {
	printf("          %ld %sarticle%s", (long)sel_total_obj_cnt,
	       sel_rereading? "read " : nullstr,
	       sel_total_obj_cnt == 1 ? nullstr : "s");
	if (sel_exclusive)
	    printf(" out of %ld", (long)obj_count);
	fputs(moderated,stdout);
    }
    else {
	printf("       %s%ld group%s",group_init_done? nullstr : "~",
	    (long)sel_total_obj_cnt, PLURAL(sel_total_obj_cnt));
	if (sel_exclusive)
	    printf(" out of %ld", (long)obj_count);
	if (maxngtodo)
	    printf(" (Restriction)");
    }
    home_cursor();
    newline();
    maybe_eol();
    if (in_ng && redirected && redirected != nullstr)
	printf("\t** Please start using %s **", redirected);
    newline();
}

void
display_page()
{
    int sel;

    display_page_title(FALSE);

try_again:
    sel_page_init();

    if (!sel_total_obj_cnt)
	;
    else if (sel_mode == SM_MULTIRC) {
	MULTIRC* mp = sel_page_mp;
	NEWSRC* rp;
	int len;
	if (mp)
	    (void) multirc_ptr(mp->num);
	for (; mp && sel_page_item_cnt<sel_max_per_page; mp=multirc_next(mp)) {
#if 0
	    if (mp == multirc)
		sel_item_index = sel_page_item_cnt;
#endif

	    if (!(mp->flags & MF_INCLUDED))
		continue;

	    sel = !!(mp->flags & sel_mask);
	    sel_items[sel_page_item_cnt].u.mp = mp;
	    sel_items[sel_page_item_cnt].line = term_line;
	    sel_items[sel_page_item_cnt].sel = sel;
	    sel_page_obj_cnt++;

	    maybe_eol();
	    for (rp = mp->first, len = 0; rp && len < 34; rp = rp->next) {
		sprintf(buf+len, ", %s", rp->datasrc->name);
		len += strlen(buf+len);
	    }
	    if (rp)
		strcpy(buf+len, ", ...");
	    output_sel(sel_page_item_cnt, sel, FALSE);
	    printf("%5d %s\n", mp->num, buf+2);
	    termdown(1);
	    sel_page_item_cnt++;
	}
	if (!sel_page_obj_cnt) {
	    if (last_page())
		goto try_again;
	}
	sel_next_mp = mp;
    }
    else if (sel_mode == SM_NEWSGROUP) {
	NGDATA* np;
	int max_len = 0;
	int outputting = (*sel_grp_dmode != 'l');
      start_of_loop:
	for (np = sel_page_np; np; np = np->next) {
	    if (np == ngptr)
		sel_item_index = sel_page_item_cnt;

	    if (!(np->flags & NF_INCLUDED))
		continue;

	    if (!np->abs1st) {
		set_toread(np, ST_LAX);
		if (paranoid) {
		    newline();
		    current_ng = ngptr = np;
		    /* this may move newsgroups around */
		    cleanup_newsrc(np->rc);
		    init_pages(PRESERVE_PAGE);
		    display_page_title(FALSE);
		    goto try_again;
		}
		if (sel_rereading? (np->toread != TR_NONE)
				 : (np->toread < ng_min_toread)) {
		    np->flags &= ~NF_INCLUDED;
		    sel_total_obj_cnt--;
		    newsgroup_toread--;
		    missing_count++;
		    continue;
		}
	    }

	    if (sel_page_item_cnt >= sel_max_per_page)
		break;

	    if (outputting) {
		sel = !!(np->flags & sel_mask) + (np->flags & NF_DEL);
		sel_items[sel_page_item_cnt].u.np = np;
		sel_items[sel_page_item_cnt].line = term_line;
		sel_items[sel_page_item_cnt].sel = sel;
		sel_page_obj_cnt++;

		maybe_eol();
		output_sel(sel_page_item_cnt, sel, FALSE);
		printf("%5ld ", (long)np->toread);
		display_group(np->rc->datasrc,np->rcline,np->numoffset-1,max_len);
	    }
	    else if (np->numoffset >= max_len)
		max_len = np->numoffset + 1;
	    sel_page_item_cnt++;
	}
	if (!outputting) {
	    outputting = 1;
	    sel_page_init();
	    goto start_of_loop;
	}
	if (!sel_page_obj_cnt) {
	    if (last_page())
		goto try_again;
	}
	sel_next_np = np;
	if (!group_init_done) {
	    for (; np; np = np->next) {
		if (!np->abs1st)
		    break;
	    }
	    if (!np) {
		int line = term_line;
		group_init_done = TRUE;
		display_page_title(TRUE);
		goto_xy(0,line);
	    }
	}
    }
    else if (sel_mode == SM_ADDGROUP) {
	ADDGROUP* gp = sel_page_gp;
	int max_len = 0;
	if (*sel_grp_dmode == 'l') {
	    int i = 0;
	    int len;
	    for (; gp && i < sel_max_per_page; gp = gp->next) {
		if (!(gp->flags & AGF_INCLUDED))
		    continue;
		len = strlen(gp->name)+2;
		if (len > max_len)
		    max_len = len;
		i++;
	    }
	    gp = sel_page_gp;
	}
	for (; gp && sel_page_item_cnt < sel_max_per_page; gp = gp->next) {
#if 0
	    if (gp == xx)
		sel_item_index = sel_page_item_cnt;
#endif

	    if (!(gp->flags & AGF_INCLUDED))
		continue;

	    sel = !!(gp->flags & sel_mask) + (gp->flags & AGF_DEL);
	    sel_items[sel_page_item_cnt].u.gp = gp;
	    sel_items[sel_page_item_cnt].line = term_line;
	    sel_items[sel_page_item_cnt].sel = sel;
	    sel_page_obj_cnt++;

	    maybe_eol();
	    output_sel(sel_page_item_cnt, sel, FALSE);
	    printf("%5ld ", (long)gp->toread);
	    display_group(gp->datasrc, gp->name, strlen(gp->name), max_len);
	    sel_page_item_cnt++;
	}
	if (!sel_page_obj_cnt) {
	    if (last_page())
		goto try_again;
	}
	sel_next_gp = gp;
    }
    else if (sel_mode == SM_UNIVERSAL) {
	UNIV_ITEM* ui = sel_page_univ;
	for (; ui && sel_page_item_cnt < sel_max_per_page; ui = ui->next) {
#if 0
	    if (ui == xx)
		sel_item_index = sel_page_item_cnt;
#endif

	    if (!(ui->flags & UF_INCLUDED))
		continue;

	    sel = !!(ui->flags & sel_mask) + (ui->flags & UF_DEL);
	    sel_items[sel_page_item_cnt].u.un = ui;
	    sel_items[sel_page_item_cnt].line = term_line;
	    sel_items[sel_page_item_cnt].sel = sel;
	    sel_page_obj_cnt++;

	    maybe_eol();
	    output_sel(sel_page_item_cnt, sel, FALSE);
	    putchar(' ');
	    display_univ(ui);
	    sel_page_item_cnt++;
	}
	if (!sel_page_obj_cnt) {
	    if (last_page())
		goto try_again;
	}
	sel_next_univ = ui;
    }
    else if (sel_mode == SM_OPTIONS) {
	int op = sel_page_op;
	for (; op <= obj_count && sel_page_item_cnt<sel_max_per_page; op++) {
#if 0
	    if (op == xx)
		sel_item_index = sel_page_item_cnt;
#endif

	    if (!(option_flags[op] & OF_INCLUDED))
		continue;

	    if (*options_ini[op].item == '*')
		sel = !!(option_flags[op] & OF_SEL);
	    else
		sel = (INI_VALUE(options_ini,op)? 1 :
		       (option_saved_vals[op]? 3 :
			(option_def_vals[op]? 0 : 2)));
	    sel_items[sel_page_item_cnt].u.op = op;
	    sel_items[sel_page_item_cnt].line = term_line;
	    sel_items[sel_page_item_cnt].sel = sel;
	    sel_page_obj_cnt++;

	    maybe_eol();
	    display_option(op,sel_page_item_cnt);
	    sel_page_item_cnt++;
	}
	if (!sel_page_obj_cnt) {
	    if (last_page())
		goto try_again;
	}
	sel_next_op = op;
    }
    else if (sel_mode == SM_ARTICLE) {
	ARTICLE* ap;
	ARTICLE** app;
	ARTICLE** limit;

	limit = artptr_list + artptr_list_size;
	app = sel_page_app;
	for (; app < limit && sel_page_item_cnt < sel_max_per_page; app++) {
	    ap = *app;
	    if (ap == sel_last_ap)
		sel_item_index = sel_page_item_cnt;
	    if (!(ap->flags & AF_INCLUDED))
		continue;
	    sel = !!(ap->flags & sel_mask) + (ap->flags & AF_DEL);
	    sel_items[sel_page_item_cnt].u.ap = ap;
	    sel_items[sel_page_item_cnt].line = term_line;
	    sel_items[sel_page_item_cnt].sel = sel;
	    sel_page_obj_cnt++;
	    /* Output the article, with optional author */
	    display_article(ap, sel_page_item_cnt, sel);
	    sel_page_item_cnt++;
	}
	if (!sel_page_obj_cnt) {
	    if (last_page())
		goto try_again;
	}
	sel_next_app = app;
    }
    else {
	SUBJECT* sp;
	int line_cnt;
	int etc = 0;
	int ix = -1;	/* item # or -1 */

	sp = sel_page_sp;
	for (; sp && !etc && sel_page_item_cnt<sel_max_per_page; sp=sp->next) {
	    if (sp == sel_last_sp)
		sel_item_index = sel_page_item_cnt;

	    if (sp->flags & SF_INCLUDED) {
		/* Compute how many lines we need to display this group */
		if (sel_mode == SM_THREAD)
		    line_cnt = count_thread_lines(sp, &sel);
		else
		    line_cnt = count_subject_lines(sp, &sel);
		if (line_cnt) {
		    /* If this item is too long to fit on the screen all by
		    ** itself, trim it to fit and set the "etc" flag.
		    */
		    if (line_cnt > sel_max_line_cnt) {
			etc = line_cnt;
			line_cnt = sel_max_line_cnt;
		    }
		    /* If it doesn't fit, save it for the next page */
		    if (term_line + line_cnt > sel_max_line_cnt + 2)
			break;
		    sel_items[sel_page_item_cnt].u.sp = sp;
		    sel_items[sel_page_item_cnt].line = term_line;
		    sel_items[sel_page_item_cnt].sel = sel;
		    sel_page_obj_cnt += sp->misc;

		    ix = sel_page_item_cnt;
		    sel = sel_items[sel_page_item_cnt].sel;
		    sel_page_item_cnt++;
		    if (sp->misc) {
			display_subject(sp, ix, sel);
			ix = -1;
		    }
		}
	    } else
		line_cnt = 0;
	    if (sel_mode == SM_THREAD) {
		while (sp->next && sp->next->thread == sp->thread) {
		    sp = sp->next;
		    if (!line_cnt || !sp->misc)
			continue;
		    if (term_line < sel_max_line_cnt + 2)
			display_subject(sp, ix, sel);
		    ix = -1;
		    sel_page_obj_cnt += sp->misc;
		}
	    }
	    if (etc)
		printf("     ... etc. [%d lines total]", etc);
	}
	if (!sel_page_obj_cnt) {
	    if (last_page())
		goto try_again;
	}
	sel_next_sp = sp;
    }
    sel_last_ap = NULL;
    sel_last_sp = NULL;
    sel_at_end = (sel_prior_obj_cnt + sel_page_obj_cnt == sel_total_obj_cnt);
    maybe_eol();
    newline();
    sel_last_line = term_line;
}

void
update_page()
{
    SEL_UNION u;
    int sel;
    int j;
    for (j = 0; j < sel_page_item_cnt; j++) {
	u = sel_items[j].u;
	switch (sel_mode) {
	  case SM_MULTIRC:
	    sel = !!(u.mp->flags & sel_mask);
	    break;
	  case SM_NEWSGROUP:
	    sel = !!(u.np->flags & sel_mask) + (u.np->flags & NF_DEL);
	    break;
	  case SM_ADDGROUP:
	    sel = !!(u.gp->flags & sel_mask) + (u.gp->flags & AGF_DEL);
	    break;
	  case SM_UNIVERSAL:
	    sel = !!(u.un->flags & sel_mask) + (u.un->flags & UF_DEL);
	    break;
	  case SM_OPTIONS:
	    if (*options_ini[u.op].item == '*')
		sel = !!(option_flags[u.op] & OF_SEL);
	    else
		sel = (INI_VALUE(options_ini,u.op)? 1 :
		       (option_saved_vals[u.op]? 3 :
			(option_def_vals[u.op]? 0 : 2)));
	    break;
	  case SM_ARTICLE:
	    sel = !!(u.ap->flags & sel_mask) + (u.ap->flags & AF_DEL);
	    break;
	  case SM_THREAD:
	    (void) count_thread_lines(u.sp, &sel);
	    break;
	  default:
	    (void) count_subject_lines(u.sp, &sel);
	    break;
	}
	if (sel == sel_items[j].sel)
	    continue;
	goto_xy(0,sel_items[j].line);
	sel_item_index = j;
	output_sel(sel_item_index, sel, TRUE);
    }
    if (++sel_item_index == sel_page_item_cnt)
	sel_item_index = 0;
}

void
output_sel(ix, sel, update)
int ix;
int sel;
bool_int update;
{
    if (ix < 0) {
	if (UseSelNum)
	    putchar(' ');
	putchar(' ');
	putchar(' ');
	return;
    }

    if (UseSelNum) {
	/* later consider no-leading-zero option */
	printf("%02d",ix+1);
    } else
	putchar(sel_chars[ix]);

    switch (sel) {
      case 1:			/* '+' */
	color_object(COLOR_PLUS, 1);
	break;
      case 2:			/* '-' */
	color_object(COLOR_MINUS, 1);
	break;
      case 3:			/* '*' */
	color_object(COLOR_STAR, 1);
	break;
      default:
	color_object(COLOR_DEFAULT, 1);
	break;
    }
    putchar(sel_disp_char[sel]);
    color_pop();	/* of COLOR_PLUS/MINUS/STAR/DEFAULT */

    if (update)
	sel_items[ix].sel = sel;
}

/* Counts the number of lines needed to output a subject, including
** optional authors.
*/
static int
count_subject_lines(subj, selptr)
SUBJECT* subj;
int* selptr;
{
    register ARTICLE* ap;
    register int sel;

    if (subj->flags & SF_DEL)
	sel = 2;
    else if (subj->flags & sel_mask) {
	sel = 1;
	for (ap = subj->articles; ap; ap = ap->subj_next) {
	    if ((!!(ap->flags&AF_UNREAD) ^ sel_rereading)
	      && !(ap->flags & sel_mask)) {
		sel = 3;
		break;
	    }
	}
    } else
	sel = 0;
    if (selptr)
	*selptr = sel;
    if (*sel_art_dmode == 'l')
	return subj->misc;
    if (*sel_art_dmode == 'm')
	return (subj->misc <= 4? subj->misc : (subj->misc - 4) / 3 + 4);
    return (subj->misc != 0);
}

/* Counts the number of lines needed to output a thread, including
** optional authors.
*/
static int
count_thread_lines(subj, selptr)
SUBJECT* subj;
int* selptr;
{
    register int total = 0;
    register ARTICLE* thread = subj->thread;
    int sel = -1, subj_sel;

    do {
	if (subj->misc) {
	    total += count_subject_lines(subj, &subj_sel);
	    if (sel < 0)
		sel = subj_sel;
	    else if (sel != subj_sel)
		sel = 3;
	}
    } while ((subj = subj->next) != NULL && subj->thread == thread);
    if (selptr)
	*selptr = (sel < 0? 0 : sel);
    return total;
}

/* Display an article, perhaps with its author.
*/
static void
display_article(ap, ix, sel)
register ARTICLE* ap;
int ix;
int sel;
{
    int subj_width = tc_COLS - 5 - UseSelNum;
    int from_width = tc_COLS / 5;
    int date_width = tc_COLS / 5;

    maybe_eol();
    if (subj_width < 32)
	subj_width = 32;
    
    output_sel(ix, sel, FALSE);
    if (*sel_art_dmode == 's' || from_width < 8)
	printf("  %s\n",compress_subj(ap->subj->articles,subj_width)) FLUSH;
    else if (*sel_art_dmode == 'd') {
  	printf("%s  %s\n",
	       compress_date(ap, date_width),
	       compress_subj(ap, subj_width - date_width)) FLUSH;
    } else {
	printf("%s  %s\n",
	       compress_from(ap->from, from_width),
	       compress_subj(ap, subj_width - from_width)) FLUSH;
    }
    termdown(1);
}

/* Display the given subject group, with optional authors.
*/
static void
display_subject(subj, ix, sel)
SUBJECT* subj;
int ix;
int sel;
{
    register ARTICLE* ap;
    register int j, i;
    int subj_width = tc_COLS - 8 - UseSelNum;
    int from_width = tc_COLS / 5;
    int date_width = tc_COLS / 5;

    maybe_eol();
    if (subj_width < 32)
	subj_width = 32;

    j = subj->misc;

    output_sel(ix, sel, FALSE);
    if (*sel_art_dmode == 's' || from_width < 8) {
	printf("%3d  %s\n",j,compress_subj(subj->articles,subj_width)) FLUSH;
	termdown(1);
    }
    else {
	ARTICLE* first_ap;
	/* Find the first unread article so we get the author right */
	if ((first_ap = subj->thread) != NULL
	 && (first_ap->subj != subj || first_ap->from == NULL
	  || (!(first_ap->flags&AF_UNREAD) ^ sel_rereading)))
	    first_ap = NULL;
	for (ap = subj->articles; ap; ap = ap->subj_next) {
	    if (!!(ap->flags&AF_UNREAD) ^ sel_rereading)
		break;
	}
	if (!first_ap)
	    first_ap = ap;
	if (*sel_art_dmode == 'd') {
	    printf("%s%3d  %s\n",
		   compress_date(first_ap, date_width), j,
		   compress_subj(first_ap, subj_width - date_width)) FLUSH;
	} else {
	    printf("%s%3d  %s\n",
		   compress_from(first_ap? first_ap->from : NULL, from_width), j,
		   compress_subj(first_ap, subj_width - from_width)) FLUSH;
	}
	termdown(1);
	i = -1;
	if (*sel_art_dmode != 'd' && --j && ap) {
	    for ( ; ap && j; ap = ap->subj_next) {
		if ((!(ap->flags&AF_UNREAD) ^ sel_rereading)
		 || ap == first_ap)
		    continue;
		j--;
		if (i < 0)
		    i = 0;
		else if (*sel_art_dmode == 'm') {
		    if (!j) {
			if (i)
			    newline();
		    }
		    else {
			if (i == 3 || !i) {
			    if (i)
				newline();
			    if (term_line >= sel_max_line_cnt + 2)
				return;
			    maybe_eol();
			    i = 1;
			} else
			    i++;
			if (UseSelNum)
			    putchar(' ');
			printf("  %s      ",
			       compress_from(ap? ap->from : NULL, from_width)) FLUSH;
			continue;
		    }
		}
		if (term_line >= sel_max_line_cnt + 2)
		    return;
		maybe_eol();
		if (UseSelNum)
		    putchar(' ');
		printf("  %s\n", compress_from(ap? ap->from : NULL, from_width)) FLUSH;
		termdown(1);
	    }
	}
    }
}

void
display_option(op,item_index)
int op;
int item_index;
{
    int len;
    char* pre;
    char* item;
    char* post;
    char* val;
    if (*options_ini[op].item == '*') {
	len = strlen(options_ini[op].item+1);
	pre = "==";
	item = options_ini[op].item+1;
	post = "==================================";
	val = nullstr;
    }
    else {
	len = (options_ini[op].checksum & 0xff);
	pre = "  ";
	item = options_ini[op].item;
	post = "..................................";
	val = INI_VALUE(options_ini,op);
	if (!val)
	    val = quote_string(option_value(op));
    }
    output_sel(item_index, sel_items[item_index].sel, FALSE);
    printf(" %s%s%s %.39s\n", pre, item, post + len, val);
    termdown(1);
}

static void
display_univ(ui)
UNIV_ITEM* ui;
{
    if (!ui) {
	fputs("****EMPTY****",stdout);
    } else {
	switch(ui->type) {
	  case UN_NEWSGROUP: {
	    NGDATA* np;

	    /* later error check the UI? */
	    np = find_ng(ui->data.group.ng);
	    if (!np)
		printf("!!!!! could not find %s",ui->data.group.ng);
	    else {
		int numarts;
		/* XXX set_toread() can print sometimes... */
		if (!np->abs1st)
		    set_toread(np, ST_LAX);
		numarts = np->toread;
		if (numarts >= 0)
		    printf("%5ld ",(long)numarts);
		else if (numarts == TR_UNSUB)
		    printf("UNSUB ");
		else
		    printf("***** ");
		fputs(ui->data.group.ng,stdout);
	    }
	    newline();
	    break;
	  }
	  case UN_ARTICLE:
	    printf("      %s",ui->desc? ui->desc : univ_article_desc(ui));
	    newline();
	    break;
	  case UN_HELPKEY:
	    printf("      Help on the %s",univ_keyhelp_modestr(ui));
	    newline();
	    break;
	  default:
	    printf("      %s",ui->desc? ui->desc : "[No Description]");
	    newline();
	    break;
	}
    }
}

static void
display_group(dp, group, len, max_len)
DATASRC* dp;
char* group;
int len;
int max_len;
{
    if (*sel_grp_dmode == 's')
	fputs(group, stdout);
    else {
	char* end;
	char* cp = find_grpdesc(dp, group);
	if (*cp != '?' && (end = index(cp, '\n')) != NULL
	 && end != cp) {
	    char ch;
	    if (end - cp > tc_COLS - max_len - 8 - 1 - UseSelNum)
		end = cp + tc_COLS - max_len - 8 - 1 - UseSelNum;
	    ch = *end;
	    *end = '\0';
	    if (*sel_grp_dmode == 'm')
		fputs(cp, stdout);
	    else {
		int i = max_len - len;
		fputs(group, stdout);
		do {
		    putchar(' ');
		} while (--i > 0);
		fputs(cp, stdout);
	    }
	    *end = ch;
	}
	else
	    fputs(group, stdout);
    }
    newline();
}
