/* rt-select.c
*/
/* This software is copyrighted as detailed in the LICENSE file. */


#include "EXTERN.h"
#include "common.h"
#include "list.h"
#include "trn.h"
#include "term.h"
#include "final.h"
#include "util.h"
#include "util2.h"
#include "help.h"
#include "hash.h"
#include "cache.h"
#include "bits.h"
#include "artsrch.h"
#include "ng.h"
#include "opt.h"
#include "ngdata.h"
#include "nntpclient.h"
#include "datasrc.h"
#include "addng.h"
#include "nntp.h"
#include "ngstuff.h"
#include "ngsrch.h"
#include "rcstuff.h"
#include "rcln.h"
#include "kfile.h"
#include "intrp.h"
#include "search.h"
#include "rthread.h"
#include "univ.h"
#include "rt-page.h"
#include "rt-util.h"
#include "color.h"
#include "only.h"
#ifdef USE_TK
#include "tkstuff.h"
#include "tktree.h"
#endif
#include "INTERN.h"
#include "rt-select.h"
#include "rt-select.ih"

static char sel_ret;
static char page_char, end_char;
static int disp_status_line;
static bool clean_screen;
static int removed_prompt;
static int force_sel_pos;

#define START_SELECTOR(new_mode)\
    char save_mode = mode;\
    char save_gmode = gmode;\
    bos_on_stop = TRUE;\
    set_mode('s',new_mode)

#define END_SELECTOR()\
    bos_on_stop = FALSE;\
    set_mode(save_gmode,save_mode)

#define PUSH_SELECTOR()\
    int save_sel_mode = sel_mode;\
    bool save_sel_rereading = sel_rereading;\
    bool save_sel_exclusive = sel_exclusive;\
    ART_UNREAD save_selected_count = selected_count;\
    int (*save_extra_commands) _((char_int)) = extra_commands

#define POP_SELECTOR()\
    sel_exclusive = save_sel_exclusive;\
    sel_rereading = save_sel_rereading;\
    selected_count = save_selected_count;\
    extra_commands = save_extra_commands;\
    bos_on_stop = TRUE;\
    if (sel_mode != save_sel_mode) {\
	sel_mode = save_sel_mode;\
	set_selector(0, 0);\
	save_sel_mode = 0;\
    }

#define PUSH_UNIV_SELECTOR()\
    UNIV_ITEM* save_first_univ = first_univ;\
    UNIV_ITEM* save_last_univ = last_univ;\
    UNIV_ITEM* save_page_univ = sel_page_univ;\
    UNIV_ITEM* save_next_univ = sel_next_univ;\
    char* save_univ_fname = univ_fname;\
    char* save_univ_label = univ_label;\
    char* save_univ_title = univ_title;\
    char* save_univ_tmp_file = univ_tmp_file;\
    char save_sel_ret = sel_ret;\
    HASHTABLE* save_univ_ng_hash = univ_ng_hash;\
    HASHTABLE* save_univ_vg_hash = univ_vg_hash

#define POP_UNIV_SELECTOR()\
    first_univ = save_first_univ;\
    last_univ = save_last_univ;\
    sel_page_univ = save_page_univ;\
    sel_next_univ = save_next_univ;\
    univ_fname = save_univ_fname;\
    univ_label = save_univ_label;\
    univ_title = save_univ_title;\
    univ_tmp_file = save_univ_tmp_file;\
    sel_ret = save_sel_ret;\
    univ_ng_hash = save_univ_ng_hash;\
    univ_vg_hash = save_univ_vg_hash

static int (*extra_commands) _((char_int));

/* Display a menu of threads/subjects/articles for the user to choose from.
** If "cmd" is '+' we display all the unread items and allow the user to mark
** them as selected and perform various commands upon them.  If "cmd" is 'U'
** the list consists of previously-read items for the user to mark as unread.
*/
char
article_selector(cmd)
char_int cmd;
{
    bool save_selected_only;
    START_SELECTOR('t');

    sel_rereading = (cmd == 'U');

    art = lastart+1;
    extra_commands = article_commands;
    keep_the_group_static = (keep_the_group_static == 1);

    sel_mode = SM_ARTICLE;
    set_sel_mode(cmd);

    if (!cache_range(sel_rereading? absfirst : firstart, lastart)) {
	sel_ret = '+';
	goto sel_exit;
    }

  sel_restart:
    /* Setup for selecting articles to read or set unread */
    if (sel_rereading) {
	end_char = 'Z';
	page_char = '>';
	sel_page_app = NULL;
	sel_page_sp = NULL;
	sel_mask = AF_DELSEL;
    } else {
	end_char = NewsSelCmds[0];
	page_char = NewsSelCmds[1];
	if (curr_artp) {
	    sel_last_ap = curr_artp;
	    sel_last_sp = curr_artp->subj;
	}
	sel_mask = AF_SEL;
    }
    save_selected_only = selected_only;
    selected_only = TRUE;
    count_subjects(cmd? CS_UNSEL_STORE : CS_NORM);

    init_pages(FILL_LAST_PAGE);
    sel_item_index = 0;
    *msg = '\0';
    if (added_articles) {
	register long i = added_articles, j;
	for (j = lastart-i+1; j <= lastart; j++) {
	    if (!article_unread(j))
		i--;
	}
	if (i == added_articles)
	    sprintf(msg, "** %ld new article%s arrived **  ",
		(long)added_articles, PLURAL(added_articles));
	else
	    sprintf(msg, "** %ld of %ld new articles unread **  ",
		i, (long)added_articles);
	disp_status_line = 1;
    }
    added_articles = 0;
    if (cmd && selected_count) {
	sprintf(msg+strlen(msg), "%ld article%s selected.",
		(long)selected_count, selected_count == 1? " is" : "s are");
	disp_status_line = 1;
    }
    cmd = 0;

    sel_display();
    if (sel_input() == 'R')
	goto sel_restart;

    sel_cleanup();
    newline();
    if (mousebar_cnt)
	clear_rest();

sel_exit:
    if (sel_ret == '\033')
	sel_ret = '+';
    else if (sel_ret == '`')
	sel_ret = 'Q';
    if (sel_rereading) {
	sel_rereading = FALSE;
	sel_mask = AF_SEL;
    }
    if (sel_mode != SM_ARTICLE || sel_sort == SS_GROUPS
     || sel_sort == SS_STRING) {
	if (artptr_list) {
	    free((char*)artptr_list);
	    artptr_list = sel_page_app = NULL;
	    sort_subjects();
	}
	artptr = NULL;
#ifdef ARTSEARCH
	if (!ThreadedGroup)
	    srchahead = -1;
#endif
    }
#ifdef ARTSEARCH
    else
	srchahead = 0;
#endif
    selected_only = (selected_count != 0);
    if (sel_ret == '+') {
	selected_only = save_selected_only;
	count_subjects(CS_RESELECT);
    }
    else
	count_subjects(CS_UNSELECT);
    if (sel_ret == '+') {
	art = curr_art;
	artp = curr_artp;
    } else
	top_article();
    END_SELECTOR();
    return sel_ret;
}

static void
sel_dogroups()
{
    NGDATA* np;
    int ret;
    int save_selected_count = selected_count;

    for (np = first_ng; np; np = np->next) {
	if (!(np->flags & NF_VISIT))
	    continue;
      do_group:
	if (np->flags & NF_SEL) {
	    np->flags &= ~NF_SEL;
	    save_selected_count--;
	}
	set_ng(np);
	if (np != current_ng) {
	    recent_ng = current_ng;
	    current_ng = np;
	}
	ThreadedGroup = (use_threads && !(np->flags & NF_UNTHREADED));
	printf("Entering %s:", ngname);
#ifdef SCAN_ART
	if (sel_ret == ';') {
	    ret = do_newsgroup(savestr(";"));
	} else
#endif
	    ret = do_newsgroup(nullstr);
	switch (ret) {
	  case NG_NORM:
	  case NG_SELNEXT:
	    set_ng(np->next);
	    break;
	  case NG_NEXT:
	    set_ng(np->next);
	    goto loop_break;
	  case NG_ERROR:
	  case NG_ASK:
	    goto loop_break;
	  case NG_SELPRIOR:
	    while ((np = np->prev) != NULL) {
		if (np->flags & NF_VISIT)
		    goto do_group;
	    }
	    (void) first_page();
	    goto loop_break;
	  case NG_MINUS:
	    np = recent_ng;
#if 0
/* CAA: I'm not sure why I wrote this originally--it seems unnecessary */
	    np->flags |= NF_VISIT;
#endif
	    goto do_group;
#ifdef SUPPORT_NNTP
	  case NG_NOSERVER:
	    nntp_server_died(np->rc->datasrc);
	    (void) first_page();
	    break;
#endif
	  /* CAA extensions */
	  case NG_GO_ARTICLE:
	    np = ng_go_ngptr;
	    goto do_group;
	  /* later: possible go-to-newsgroup (applicable?) */
	}
    }
  loop_break:
    selected_count = save_selected_count;
}

char
multirc_selector()
{
    START_SELECTOR('c');

    sel_rereading = FALSE;
    sel_exclusive = FALSE;
    selected_count = 0;

    set_selector(SM_MULTIRC, 0);

  sel_restart:
    end_char = NewsrcSelCmds[0];
    page_char = NewsrcSelCmds[1];
    sel_mask = MF_SEL;
    extra_commands = multirc_commands;

    init_pages(FILL_LAST_PAGE);
    sel_item_index = 0;

    sel_display();
    if (sel_input() == 'R')
	goto sel_restart;

    newline();
    if (mousebar_cnt)
	clear_rest();

    if (sel_ret=='\r' || sel_ret=='\n' || sel_ret=='Z' || sel_ret=='\t') {
	MULTIRC* mp;
	NEWSRC* rp;
	PUSH_SELECTOR();
	for (mp = multirc_low(); mp; mp = multirc_next(mp)) {
	    if (mp->flags & MF_SEL) {
		mp->flags &= ~MF_SEL;
		save_selected_count--;
		for (rp = mp->first; rp; rp = rp->next)
		    rp->datasrc->flags &= ~DF_UNAVAILABLE;
		if (use_multirc(mp)) {
		    find_new_groups();
		    do_multirc();
		    unuse_multirc(mp);
		}
		else
		    mp->flags &= ~MF_INCLUDED;
	    }
	}
	POP_SELECTOR();
	goto sel_restart;
    }
    END_SELECTOR();
    return sel_ret;
}

char
newsgroup_selector()
{
    START_SELECTOR('w');

    sel_rereading = FALSE;
    sel_exclusive = FALSE;
    selected_count = 0;

    set_selector(SM_NEWSGROUP, 0);

  sel_restart:
    if (*sel_grp_dmode != 's') {
	NEWSRC* rp;
	for (rp = multirc->first; rp; rp = rp->next) {
	    if ((rp->flags & RF_ACTIVE) && !rp->datasrc->desc_sf.hp) {
		find_grpdesc(rp->datasrc, "control");
#ifdef SUPPORT_NNTP
		if (rp->datasrc->desc_sf.fp)
		    rp->datasrc->flags |= DF_NOXGTITLE; /*$$ ok?*/
		else
		    rp->datasrc->desc_sf.refetch_secs = 0;
#endif
	    }
	}
    }

    end_char = NewsgroupSelCmds[0];
    page_char = NewsgroupSelCmds[1];
    if (sel_rereading) {
	sel_mask = NF_DELSEL;
	sel_page_np = NULL;
    } else
	sel_mask = NF_SEL;
    extra_commands = newsgroup_commands;

    init_pages(FILL_LAST_PAGE);
    sel_item_index = 0;

    sel_display();
    if (sel_input() == 'R')
	goto sel_restart;

    newline();
    if (mousebar_cnt)
	clear_rest();

    if (sel_ret=='\r' || sel_ret=='\n' || sel_ret=='Z' || sel_ret=='\t'
#ifdef SCAN_ART
     || sel_ret==';'
#endif
    ) {
	NGDATA* np;
	PUSH_SELECTOR();
	for (np = first_ng; np; np = np->next) {
	    if ((np->flags & NF_INCLUDED)
	     && (!selected_count || (np->flags & sel_mask)))
		np->flags |= NF_VISIT;
	    else
		np->flags &= ~NF_VISIT;
	}
	sel_dogroups();
	save_selected_count = selected_count;
	POP_SELECTOR();
	if (multirc)
	    goto sel_restart;
	sel_ret = 'q';
    }
    sel_cleanup();
    END_SELECTOR();
    end_only();
    return sel_ret;
}

char
addgroup_selector(flags)
int flags;
{
    START_SELECTOR('j');

    sel_rereading = FALSE;
    sel_exclusive = FALSE;
    selected_count = 0;

    set_selector(SM_ADDGROUP, 0);

  sel_restart:
    if (*sel_grp_dmode != 's') {
	NEWSRC* rp;
	for (rp = multirc->first; rp; rp = rp->next) {
	    if ((rp->flags & RF_ACTIVE) && !rp->datasrc->desc_sf.hp) {
		find_grpdesc(rp->datasrc, "control");
#ifdef SUPPORT_NNTP
		if (!rp->datasrc->desc_sf.fp)
		    rp->datasrc->desc_sf.refetch_secs = 0;
#endif
	    }
	}
    }

    end_char = AddSelCmds[0];
    page_char = AddSelCmds[1];
    /* Setup for selecting articles to read or set unread */
    if (sel_rereading)
	sel_mask = AGF_DELSEL;
    else
	sel_mask = AGF_SEL;
    sel_page_gp = NULL;
    extra_commands = addgroup_commands;

    init_pages(FILL_LAST_PAGE);
    sel_item_index = 0;

    sel_display();
    if (sel_input() == 'R')
	goto sel_restart;

    selected_count = 0;
    newline();
    if (mousebar_cnt)
	clear_rest();

    if (sel_ret=='\r' || sel_ret=='\n' || sel_ret=='Z' || sel_ret=='\t') {
	ADDGROUP *gp;
	int i;
	addnewbydefault = ADDNEW_SUB;
	for (gp = first_addgroup, i = 0; gp; gp = gp->next, i++) {
	    if (gp->flags & NF_SEL) {
		gp->flags &= ~NF_SEL;
		get_ng(gp->name,flags);
	    }
	}
	addnewbydefault = 0;
    }
    sel_cleanup();
    END_SELECTOR();
    return sel_ret;
}

char
option_selector()
{
    int i;
    char** vals = INI_VALUES(options_ini);
    START_SELECTOR('l');

    sel_rereading = FALSE;
    sel_exclusive = FALSE;
    selected_count = 0;
    parse_ini_section(nullstr, options_ini);

    set_selector(SM_OPTIONS, 0);

  sel_restart:
    end_char = OptionSelCmds[0];
    page_char = OptionSelCmds[1];
    if (sel_rereading)
	sel_mask = AF_DELSEL;
    else
	sel_mask = AF_SEL;
    sel_page_op = -1;
    extra_commands = option_commands;

    init_pages(FILL_LAST_PAGE);
    sel_item_index = 0;

    sel_display();
    if (sel_input() == 'R' || sel_ret=='\r' || sel_ret=='\n')
	goto sel_restart;

    selected_count = 0;
    newline();
    if (mousebar_cnt)
	clear_rest();

    if (sel_ret=='Z' || sel_ret=='\t' || sel_ret == 'S') {
	set_options(vals);
	if (sel_ret == 'S')
	    save_options(ini_file);
    }
    for (i = 1; options_ini[i].checksum; i++) {
	if (vals[i]) {
	    if (option_saved_vals[i] && strEQ(vals[i],option_saved_vals[i])) {
		if (option_saved_vals[i] != option_def_vals[i])
		    free(option_saved_vals[i]);
		option_saved_vals[i] = NULL;
	    }
	    free(vals[i]);
	    vals[i] = NULL;
	}
    }
    END_SELECTOR();
    return sel_ret;
}

/* returns a command to do */
static int
univ_read(ui)
UNIV_ITEM* ui;
{
    int exit_code = UR_NORM;
    char ch;

    univ_follow_temp = FALSE;
    if (!ui) {
	printf("NULL UI passed to reader!\n") FLUSH;
	sleep(5);
	return exit_code;
    }
    printf("\n") FLUSH;			/* prepare for output msgs... */
    switch (ui->type) {
      case UN_DEBUG1: {
	char* s;
	s = ui->data.str;
	if (s && *s) {
	    printf("Not implemented yet (%s)\n",s) FLUSH;
	    sleep(5);
	    return exit_code;
	}
	break;
      }
      case UN_TEXTFILE: {
	char* s;
	s = ui->data.textfile.fname;
	if (s && *s) {
	    /* later have some way of getting a return code back */
	    univ_page_file(s);
	}
	break;
      }
      case UN_ARTICLE: {
	int ret;
	NGDATA* np;

	if (in_ng) {
	    /* XXX whine: can't recurse at this time */
	    break;
	}
	if (!ui->data.virt.ng)
	    break;			/* XXX whine */
	np = find_ng(ui->data.virt.ng);

	if (!np) {
	    printf("Universal: newsgroup %s not found!",
		   ui->data.virt.ng) FLUSH;
	    sleep(5);
	    return exit_code;
	}
	set_ng(np);
	if (np != current_ng) {
	    recent_ng = current_ng;
	    current_ng = np;
	}
	ThreadedGroup = (use_threads && !(np->flags & NF_UNTHREADED));
	printf("Virtual: Entering %s:\n", ngname) FLUSH;
	ng_go_artnum = ui->data.virt.num;
	univ_read_virtflag = TRUE;
	ret = do_newsgroup(nullstr);
	univ_read_virtflag = FALSE;
	switch (ret) {
	  case NG_NORM:		/* handle more cases later */
	  case NG_SELNEXT:
	  case NG_NEXT:
	    /* just continue reading */
	    break;
	  case NG_SELPRIOR:
	    /* not implemented yet */
	    /* FALL THROUGH */
	  case NG_ERROR:
	  case NG_ASK:
	    exit_code = UR_BREAK;
	    return exit_code;
	  case NG_MINUS:
	    /* not implemented */
	    break;
	  default:
	    break;
	}
	break;
      }
      case UN_GROUPMASK: {
	univ_mask_load(ui->data.gmask.masklist,ui->data.gmask.title);
	ch = universal_selector();
	switch (ch) {
	  case 'q':
	    exit_code = UR_BREAK;
	    break;
	  default:
	    exit_code = UR_NORM;
	    break;
	}
	return exit_code;
      }
      case UN_CONFIGFILE: {
	univ_file_load(ui->data.cfile.fname,ui->data.cfile.title,
		       ui->data.cfile.label);
	ch = universal_selector();
	switch (ch) {
	  case 'q':
	    exit_code = UR_BREAK;
	    break;
	  default:
	    exit_code = UR_NORM;
	    break;
	}
	return exit_code;
      }
      case UN_NEWSGROUP: {
	int ret;
	NGDATA* np;

	if (in_ng) {
	    /* XXX whine: can't recurse at this time */
	    break;
	}
	if (!ui->data.group.ng)
	    break;			/* XXX whine */
	np = find_ng(ui->data.group.ng);

	if (!np) {
	    printf("Universal: newsgroup %s not found!",
		   ui->data.group.ng) FLUSH;
	    sleep(5);
	    return exit_code;
	}
      do_group:
	set_ng(np);
	if (np != current_ng) {
	    recent_ng = current_ng;
	    current_ng = np;
	}
	ThreadedGroup = (use_threads && !(np->flags & NF_UNTHREADED));
	printf("Entering %s:", ngname) FLUSH;
#ifdef SCAN_ART
	if (sel_ret == ';')
	    ret = do_newsgroup(savestr(";"));
	else
#endif
	    ret = do_newsgroup(nullstr);
	switch (ret) {
	  case NG_NORM:		/* handle more cases later */
	  case NG_SELNEXT:
	  case NG_NEXT:
	    /* just continue reading */
	    break;
	  case NG_SELPRIOR:
	    /* not implemented yet */
	    /* FALL THROUGH */
	  case NG_ERROR:
	  case NG_ASK:
	    exit_code = UR_BREAK;
	    return exit_code;
	  case NG_MINUS:
	    np = recent_ng;
	    goto do_group;
#ifdef SUPPORT_NNTP
	  case NG_NOSERVER:
	    /* Eeep! */
	    break;
#endif
	}
	break;
      }
      case UN_HELPKEY:
	if (another_command(univ_key_help(ui->data.i)))
	    pushchar(sel_ret | 0200);
	break;
      default:
	break;
    }
    return exit_code;
}

char
universal_selector()
{
    START_SELECTOR('v');		/* kind of like 'v'irtual... */

    sel_rereading = FALSE;
    sel_exclusive = FALSE;
    selected_count = 0;

    set_selector(SM_UNIVERSAL, 0);

    selected_count = 0;

sel_restart:
    /* make options */
    end_char = 'Z';
    page_char = '>';

    /* Setup for selecting articles to read or set unread */
    if (sel_rereading)
	sel_mask = UF_DELSEL;
    else
	sel_mask = UF_SEL;
    sel_page_univ = NULL;
    extra_commands = universal_commands;

    init_pages(FILL_LAST_PAGE);
    sel_item_index = 0;

    sel_display();
    if (sel_input() == 'R')
	goto sel_restart;

    newline();
    if (mousebar_cnt)
	clear_rest();

    if (sel_ret=='\r' || sel_ret=='\n' || sel_ret=='\t'
#ifdef SCAN_ART
     || sel_ret==';'
#endif
     || sel_ret=='Z') {
	UNIV_ITEM *ui;
	int i;
	for (ui = first_univ, i = 0; ui; ui = ui->next, i++) {
	    int ret;
	    if (ui->flags & UF_SEL) {
		PUSH_SELECTOR();
		PUSH_UNIV_SELECTOR();

		ui->flags &= ~UF_SEL;
		save_selected_count--;
		ret = univ_read(ui);

		POP_UNIV_SELECTOR();
		POP_SELECTOR();
		if (ret == UR_ERROR || ret == UR_BREAK) {
		    sel_ret = ' ';	/* don't leave completely. */
		    break;		/* jump out of for loop */
		}
	    }
	}
    }
/*univ_loop_break:*/
    /* restart the selector unless the user explicitly quits.
     * possibly later have an option for 'Z' to quit levels>1.
     */
    if (sel_ret != 'q' && (sel_ret != 'Q'))
	goto sel_restart;
    sel_cleanup();
    univ_close();
    END_SELECTOR();
    return sel_ret;
}

static void
sel_display()
{
    /* Present a page of items to the user */
    display_page();
    if (erase_screen && erase_each_line)
	erase_line(1);

    if (sel_item_index >= sel_page_item_cnt)
	sel_item_index = 0;

    if (disp_status_line == 1) {
	newline();
	fputs(msg, stdout);
	term_col = strlen(msg);
	disp_status_line = 2;
    }
}

static void
sel_status_msg(cp)
char* cp;
{
    if (can_home)
	goto_xy(0,sel_last_line+1);
    else
	newline();
    fputs(cp, stdout);
    term_col = strlen(cp);
    goto_xy(0,sel_items[sel_item_index].line);
    fflush(stdout);	/* CAA: otherwise may not be visible */
    disp_status_line = 2;
}

static char
sel_input()
{
    register int j;
    int ch, action;
    char* in_select;
    int got_dash, got_goto;
    int sel_number;
    int ch_num1;

    /* CAA: TRN proudly continues the state machine traditions of RN.
     *      April 2, 1996: 28 gotos in this function.  Conversion to
     *      structured programming is left as an exercise for the reader.
     */
    /* If one immediately types a goto command followed by a dash ('-'),
     * the following will be the default action.
     */
    action = '+';
  reask_selector:
    /* Prompt the user */
    sel_prompt();
  position_selector:
    got_dash = got_goto = 0;
    force_sel_pos = -1;
    if (removed_prompt & 1) {
	draw_mousebar(tc_COLS,0);
	removed_prompt &= ~1;
    }
    if (can_home)
	goto_xy(0,sel_items[sel_item_index].line);
#ifdef USE_TK
    /* Allow Tk to do something with the current positioning */
    if (ttk_running) {
	SEL_UNION u;

	u = sel_items[sel_item_index].u;
	switch (sel_mode) {
	  case SM_THREAD:
	  case SM_SUBJECT:
	    ttcl_eval("wipetree");
	    if (sel_page_item_cnt != 0)
                ttk_draw_tree(u.sp->thread, 0, 0);
	    break;
	  case SM_ARTICLE:
	  case SM_MULTIRC:
	  case SM_ADDGROUP:
	  case SM_NEWSGROUP:
	  case SM_OPTIONS:
	  case SM_UNIVERSAL:
	  default:
	    break;
	}
    }
#endif
reinp_selector:
    if (removed_prompt & 1)
	goto position_selector;	/* (CAA: TRN considered harmful? :-) */
    /* Grab some commands from the user */
    fflush(stdout);
    eat_typeahead();
    if (UseSelNum)
	spin_char = '0' + (sel_item_index+1)/10;	/* first digit */
    else
	spin_char = sel_chars[sel_item_index];
    cache_until_key();
    getcmd(buf);
    if (*buf == ' ')
	setdef(buf, sel_at_end? &end_char : &page_char);
    ch = *buf;
    if (errno)
	ch = Ctl('l');
    if (disp_status_line == 2) {
	if (can_home) {
	    goto_xy(0,sel_last_line+1);
	    erase_line(0);
	    if (term_line == tc_LINES-1)
		removed_prompt |= 1;
	}
	disp_status_line = 0;
    }
    if (ch == '-' && sel_page_item_cnt) {
	got_dash = 1;
	got_goto = 0;	/* right action is not clear if both are true */
	if (can_home) {
	    if (!input_pending()) {
		j = (sel_item_index > 0? sel_item_index : sel_page_item_cnt);
		if (UseSelNum)
		    sprintf(msg,"Range: %d-", j);
		else
		    sprintf(msg,"Range: %c-", sel_chars[j-1]);
		sel_status_msg(msg);
	    }
	}
	else {
	    putchar('-');
	    fflush(stdout);
	}
	goto reinp_selector;
    }
    /* allow the user to back out of a range or a goto with erase char */
    if (ch == ERASECH || ch == KILLCH) {
	/* later consider dingaling() if neither got_{dash,goto} is true */
	got_dash = 0;
	got_goto = 0;
	/* following if statement should be function */
	if (disp_status_line == 2) {	/* status was printed */
	    if (can_home) {
		goto_xy(0,sel_last_line+1);
		erase_line(0);
		if (term_line == tc_LINES-1)
		    removed_prompt |= 1;
	    }
	    disp_status_line = 0;
	}
	goto position_selector;
    }
    if (ch == Ctl('g')) {
	got_goto = 1;
	got_dash = 0;	/* right action is not clear if both are true */
	if (!input_pending()) {
	    if (UseSelNum)
		sel_status_msg("Go to number?");
	    else
		sel_status_msg("Go to letter?");
	}
	goto reinp_selector;
    }
    if (sel_mode == SM_OPTIONS && (ch == '\r' || ch == '\n'))
	ch = '.';
    in_select = index(sel_chars, ch);
    if (UseSelNum && ch >= '0' && ch <= '9') {
	ch_num1 = ch;
	/* would be *very* nice to use wait_key_pause() here */
	if (!input_pending()) {
	    if (got_dash) {
		if (sel_item_index > 0) {
		    j = sel_item_index;  /* -1, +1 to print */
		} else	/* wrap around from the bottom */
		    j = sel_page_item_cnt;
		sprintf(msg,"Range: %d-%c", j, ch);
	    } else {
		if (got_goto) {
		    sprintf(msg,"Go to number: %c", ch);
		} else {
		    sprintf(msg,"%c", ch);
		}
	    }
	    sel_status_msg(msg);
	}	
	/* Consider cache_until_key() here.  The middle of typing a
	 * number is a lousy time to delay, however.
	 */
	getcmd(buf);
	ch = *buf;
	if (disp_status_line == 2) {	/* status was printed */
	    if (can_home) {
		goto_xy(0,sel_last_line+1);
		erase_line(0);
		if (term_line == tc_LINES-1)
		    removed_prompt |= 1;
	    }
	    disp_status_line = 0;
	}
	if (ch == KILLCH) {	/* kill whole command in progress */
	    got_goto = 0;
	    got_dash = 0;
	    goto position_selector;
	}
	if (ch == ERASECH) {
	    /* Erase any first digit printed, but allow complex
	     * commands to continue.  Spaces at end of message are
	     * there to wipe out old first digit.
	     */
	    if (got_dash) {
		if (sel_item_index > 0) {
		    j = sel_item_index;  /* -1, +1 to print */
		} else	/* wrap around from the bottom */
		    j = sel_page_item_cnt;
		sprintf(msg,"Range: %d- ", j);
		sel_status_msg(msg);
		goto reinp_selector;
	    }
	    if (got_goto) {
		sel_status_msg("Go to number?  ");
		goto reinp_selector;
	    }
	    goto position_selector;
	}
	if (ch >= '0' && ch <= '9')
	    sel_number = ((ch_num1 - '0') * 10) + (ch - '0');
	else {
	    pushchar(ch);	/* for later use */
	    sel_number = (ch_num1 - '0');
	}
	j = sel_number-1;
	if ((j < 0) || (j >= sel_page_item_cnt)) {
	    dingaling();
	    sprintf(msg, "No item %c%c on this page.", ch_num1, ch);
	    sel_status_msg(msg);
	    goto position_selector;
	}
	else if (got_goto || (SelNumGoto && !got_dash)) {
	    /* (but only do always-goto if there was not a dash) */
	    sel_item_index = j;
	    goto position_selector;
	} else if (got_dash)
	    ;
	else if (sel_items[j].sel == 1)
	    action = (sel_rereading? 'k' : '-');
	else
	    action = '+';
    } else if (in_select && !UseSelNum) {
	j = in_select - sel_chars;
	if (j >= sel_page_item_cnt) {
	    dingaling();
	    sprintf(msg, "No item '%c' on this page.", ch);
	    sel_status_msg(msg);
	    goto position_selector;
	}
	else if (got_goto) {
	    sel_item_index = j;
	    goto position_selector;
	} else if (got_dash)
	    ;
	else if (sel_items[j].sel == 1)
	    action = (sel_rereading? 'k' : '-');
	else
	    action = '+';
    } else if (ch == '*' && sel_mode == SM_ARTICLE) {
	register ARTICLE* ap;
	if (!sel_page_item_cnt)
	    dingaling();
	else {
	    ap = sel_items[sel_item_index].u.ap;
	    if (sel_items[sel_item_index].sel)
		deselect_subject(ap->subj);
	    else
		select_subject(ap->subj, 0);
	    update_page();
	}
	goto position_selector;
    } else if (ch == 'y' || ch == '.' || ch == '*' || ch == Ctl('t')) {
	j = sel_item_index;
	if (sel_page_item_cnt && sel_items[j].sel == 1)
	    action = (sel_rereading? 'k' : '-');
	else
	    action = '+';
	if (ch == Ctl('t'))
	    force_sel_pos = j;
    } else if (ch == 'k' || ch == 'j' || ch == ',') {
	j = sel_item_index;
	action = 'k';
    } else if (ch == 'm' || ch == '|') {
	j = sel_item_index;
	action = '-';
    } else if (ch == 'M' && in_ng) {
	j = sel_item_index;
	action = 'M';
    } else if (ch == '@') {
	sel_item_index = 0;
	j = sel_page_item_cnt-1;
	got_dash = 1;
	action = '@';
    } else if (ch == '[' || ch == 'p') {
	if (--sel_item_index < 0)
	    sel_item_index = sel_page_item_cnt? sel_page_item_cnt-1 : 0;
	goto position_selector;
    } else if (ch == ']' || ch == 'n') {
	if (++sel_item_index >= sel_page_item_cnt)
	    sel_item_index = 0;
	goto position_selector;
    } else {
	sel_ret = ch;
	switch (sel_command(ch)) {
	  case DS_ASK:
	    if (!clean_screen) {
		sel_display();
		goto reask_selector;
	    }
	    if (removed_prompt & 2) {
		carriage_return();
		goto reask_selector;
	    }
	    goto position_selector;
	  case DS_DISPLAY:
	    sel_display();
	    goto reask_selector;
	  case DS_UPDATE:
	    if (!clean_screen) {
		sel_display();
		goto reask_selector;
	    }
	    if (disp_status_line == 1) {
		newline();
		fputs(msg,stdout);
		term_col = strlen(msg);
		if (removed_prompt & 1) {
		    draw_mousebar(tc_COLS,0);
		    removed_prompt &= ~1;
		}
		disp_status_line = 2;
	    }
	    update_page();
	    if (can_home)
		goto_xy(0,sel_last_line);
	    goto reask_selector;
	  case DS_RESTART:
	    return 'R'; /*Restart*/
	  case DS_QUIT:
	    return 'Q'; /*Quit*/
	  case DS_STATUS:
	    disp_status_line = 1;
	    if (!clean_screen) {
		sel_display();
		goto reask_selector;
	    }
	    sel_status_msg(msg);

	    if (!can_home)
		newline();
	    if (removed_prompt & 2)
		goto reask_selector;
	    goto position_selector;
	}
    }

    if (!sel_page_item_cnt) {
	dingaling();
	goto position_selector;
    }

    /* The user manipulated one of the letters -- handle it. */
    if (!got_dash)
	sel_item_index = j;
    else {
	if (j < sel_item_index) {
	    ch = sel_item_index-1;
	    sel_item_index = j;
	    j = ch;
	}
    }

    if (++j == sel_page_item_cnt)
	j = 0;
    do {
	register int sel = sel_items[sel_item_index].sel;
	if (can_home)
	    goto_xy(0,sel_items[sel_item_index].line);
	if (action == '@') {
	    if (sel == 2)
		ch = (sel_rereading? '+' : ' ');
	    else if (sel_rereading)
		ch = 'k';
	    else if (sel == 1)
		ch = '-';
	    else
		ch = '+';
	} else
	    ch = action;
	switch (ch) {
	  case '+':
	    if (select_item(sel_items[sel_item_index].u))
		output_sel(sel_item_index, 1, TRUE);
	    if (term_line >= sel_last_line) {
		sel_display();
		goto reask_selector;
	    }
	    break;
	  case '-':  case 'k':  case 'M': {
	    bool sel_reread_save = sel_rereading;
	    if (ch == 'M')
		delay_return_item(sel_items[sel_item_index].u);
	    if (ch == '-')
		sel_rereading = FALSE;
	    else
		sel_rereading = TRUE;
	    if (deselect_item(sel_items[sel_item_index].u))
		output_sel(sel_item_index, ch == '-' ? 0 : 2, TRUE);
	    sel_rereading = sel_reread_save;
	    if (term_line >= sel_last_line) {
		sel_display();
		goto reask_selector;
	    }
	    break;
	  }
	}
	if (can_home)
	    carriage_return();
	fflush(stdout);
	if (++sel_item_index == sel_page_item_cnt)
	    sel_item_index = 0;
    } while (sel_item_index != j);
    if (force_sel_pos >= 0)
	sel_item_index = force_sel_pos;
    goto position_selector;
}

static void
sel_prompt()
{
    draw_mousebar(tc_COLS,0);
    if (can_home)
	goto_xy(0,sel_last_line);
#ifdef MAILCALL
    setmail(FALSE);
#endif
    if (sel_at_end)
	sprintf(cmd_buf, "%s [%c%c] --",
		(!sel_prior_obj_cnt? "All" : "Bot"), end_char, page_char);
    else
	sprintf(cmd_buf, "%s%ld%% [%c%c] --",
		(!sel_prior_obj_cnt? "Top " : nullstr),
		(long)((sel_prior_obj_cnt+sel_page_obj_cnt)*100 / sel_total_obj_cnt),
		page_char, end_char);
    interp(buf, sizeof buf, mailcall);
    sprintf(msg, "%s-- %s %s (%s%s order) -- %s", buf,
	    sel_exclusive && in_ng? "SELECTED" : "Select", sel_mode_string,
	    sel_direction<0? "reverse " : nullstr, sel_sort_string, cmd_buf);
    color_string(COLOR_CMD,msg);
    term_col = strlen(msg);
    removed_prompt = 0;
}

static bool
select_item(u)
SEL_UNION u;
{
    switch (sel_mode) {
      case SM_MULTIRC:
	if (!(u.mp->flags & sel_mask))
	    selected_count++;
	u.mp->flags = (u.mp->flags /*& ~MF_DEL*/) | sel_mask;
	break;
      case SM_ADDGROUP:
	if (!(u.gp->flags & sel_mask))
	    selected_count++;
	u.gp->flags = (u.gp->flags & ~AGF_DEL) | sel_mask;
	break;
      case SM_NEWSGROUP:
	if (!(u.np->flags & sel_mask))
	    selected_count++;
	u.np->flags = (u.np->flags & ~NF_DEL) | sel_mask;
	break;
      case SM_OPTIONS:
	if (!select_option(u.op) || !INI_VALUE(options_ini,u.op))
	    return FALSE;
	break;
      case SM_THREAD:
	select_thread(u.sp->thread, 0);
	break;
      case SM_SUBJECT:
	select_subject(u.sp, 0);
	break;
      case SM_ARTICLE:
	select_article(u.ap, 0);
	break;
      case SM_UNIVERSAL:
	if (!(u.un->flags & sel_mask))
	    selected_count++;
	u.un->flags = (u.un->flags & ~UF_DEL) | sel_mask;
	break;
    }
    return TRUE;
}

static bool
delay_return_item(u)
SEL_UNION u;
{
    switch (sel_mode) {
      case SM_MULTIRC:
      case SM_ADDGROUP:
      case SM_NEWSGROUP:
      case SM_OPTIONS:
      case SM_UNIVERSAL:
	return FALSE;
      case SM_ARTICLE:
	delay_unmark(u.ap);
	break;
      default: {
	  register ARTICLE* ap;
	  if (sel_mode == SM_THREAD) {
	      for (ap = first_art(u.sp); ap; ap = next_art(ap))
		  if (!!(ap->flags & AF_UNREAD) ^ sel_rereading)
		      delay_unmark(ap);
	  } else {
	      for (ap = u.sp->articles; ap; ap = ap->subj_next)
		  if (!!(ap->flags & AF_UNREAD) ^ sel_rereading)
		      delay_unmark(ap);
	  }
	  break;
      }
    }
    return TRUE;
}

static bool
deselect_item(u)
SEL_UNION u;
{
    switch (sel_mode) {
      case SM_MULTIRC:
	if (u.mp->flags & sel_mask) {
	    u.mp->flags &= ~sel_mask;
	    selected_count--;
	}
#if 0
	if (sel_rereading)
	    u.mp->flags |= MF_DEL;
#endif
	break;
      case SM_ADDGROUP:
	if (u.gp->flags & sel_mask) {
	    u.gp->flags &= ~sel_mask;
	    selected_count--;
	}
	if (sel_rereading)
	    u.gp->flags |= AGF_DEL;
	break;
      case SM_NEWSGROUP:
	if (u.np->flags & sel_mask) {
	    u.np->flags &= ~sel_mask;
	    selected_count--;
	}
	if (sel_rereading)
	    u.np->flags |= NF_DEL;
	break;
      case SM_OPTIONS:
	if (!select_option(u.op) || INI_VALUE(options_ini,u.op))
	    return FALSE;
	break;
      case SM_THREAD:
	deselect_thread(u.sp->thread);
	break;
      case SM_SUBJECT:
	deselect_subject(u.sp);
	break;
      case SM_UNIVERSAL:
	if (u.un->flags & sel_mask) {
	    u.un->flags &= ~sel_mask;
	    selected_count--;
	}
	if (sel_rereading)
	    u.un->flags |= UF_DEL;
	break;
      default:
	deselect_article(u.ap, 0);
	break;
    }
    return TRUE;
}

static bool
select_option(i)
int i;
{
    bool changed = FALSE;
    char** vals = INI_VALUES(options_ini);
    char* val;
    char* oldval;

    if (*options_ini[i].item == '*') {
	option_flags[i] ^= OF_SEL;
	init_pages(FILL_LAST_PAGE);
	term_line = sel_last_line;
	return FALSE;
    }

    goto_xy(0,sel_last_line);
    erase_line(mousebar_cnt > 0);	/* erase the prompt */
    color_object(COLOR_CMD, 1);
    printf("Change `%s' (%s)",options_ini[i].item, options_ini[i].help_str);
    color_pop();	/* of COLOR_CMD */
    newline();
    *buf = '\0';
    oldval = savestr(quote_string(option_value(i)));
    val = vals[i]? vals[i] : oldval;
    clean_screen = in_choice("> ", val, options_ini[i].help_str, 'z');
    if (strNE(buf,val)) {
	char* to = buf;
	char* from = buf;
	parse_string(&to, &from);
	changed = TRUE;
	if (vals[i]) {
	    free(vals[i]);
	    selected_count--;
	}
	if (val != oldval && strEQ(buf,oldval))
	    vals[i] = NULL;
	else {
	    vals[i] = savestr(buf);
	    selected_count++;
	}
    }
    free(oldval);
    if (clean_screen) {
	up_line();
	erase_line(1);
	sel_prompt();
	goto_xy(0,sel_items[sel_item_index].line);
	if (changed) {
	    erase_line(0);
	    display_option(i,sel_item_index);
	    up_line();
	}
    }
    else
	return FALSE;
    return TRUE;
}

static void
sel_cleanup()
{
    switch (sel_mode) {
      case SM_MULTIRC:
	break;
      case SM_ADDGROUP:
	if (sel_rereading) {
	    ADDGROUP* gp;
	    for (gp = first_addgroup; gp; gp = gp->next) {
		if (gp->flags & AGF_DELSEL) {
		    if (!(gp->flags & AGF_SEL))
			selected_count++;
		    gp->flags = (gp->flags&~(AGF_DELSEL|AGF_EXCLUDED))|AGF_SEL;
		}
		gp->flags &= ~AGF_DEL;
	    }
	}
	else {
	    ADDGROUP* gp;
	    for (gp = first_addgroup; gp; gp = gp->next) {
		if (gp->flags & AGF_DEL)
		    gp->flags = (gp->flags & ~AGF_DEL) | AGF_EXCLUDED;
	    }
	}
	break;
      case SM_NEWSGROUP:
	if (sel_rereading) {
	    NGDATA* np;
	    for (np = first_ng; np; np = np->next) {
		if (np->flags & NF_DELSEL) {
		    if (!(np->flags & NF_SEL))
			selected_count++;
		    np->flags = (np->flags & ~NF_DELSEL) | NF_SEL;
		}
		np->flags &= ~NF_DEL;
	    }
	}
	else {
	    NGDATA* np;
	    for (np = first_ng; np; np = np->next) {
		if (np->flags & NF_DEL) {
		    np->flags &= ~NF_DEL;
		    catch_up(np, 0, 0);
		}
	    }
	}
	break;
      case SM_OPTIONS:
	break;
      /* should probably be expanded... */
      case SM_UNIVERSAL:
	break;
      default:
	if (sel_rereading) {
	    /* Turn selections into unread selected articles.  Let
	    ** count_subjects() fix the counts after we're through.
	    */
	    register SUBJECT* sp;
	    sel_last_ap = NULL;
	    sel_last_sp = NULL;
	    for (sp = first_subject; sp; sp = sp->next)
		unkill_subject(sp);
	}
	else {
	    if (sel_mode == SM_ARTICLE)
		article_walk(mark_DEL_as_READ, 0);
	    else {
		register SUBJECT* sp;
		for (sp = first_subject; sp; sp = sp->next) {
		    if (sp->flags & SF_DEL) {
			sp->flags &= ~SF_DEL;
			if (sel_mode == SM_THREAD)
			    kill_thread(sp->thread, AFFECT_UNSEL);
			else
			    kill_subject(sp, AFFECT_UNSEL);
		    }
		}
	    }
	}
	break;
    }
}

static bool
mark_DEL_as_READ(ptr, arg)
char* ptr;
int arg;
{
    register ARTICLE* ap = (ARTICLE*)ptr;
    if (ap->flags & AF_DEL) {
	ap->flags &= ~AF_DEL;
	set_read(ap);
    }
    return 0;
}

static int
sel_command(ch)
char_int ch;
{
    int ret;
    if (can_home)
	goto_xy(0,sel_last_line);
    clean_screen = TRUE;
    term_scrolled = 0;
    page_line = 1;
    if (sel_mode == SM_NEWSGROUP) {
	if (sel_item_index < sel_page_item_cnt)
	    set_ng(sel_items[sel_item_index].u.np);
	else
	    ngptr = NULL;
    }
  do_command:
    *buf = ch;
    buf[1] = FINISHCMD;
    output_chase_phrase = TRUE;
    switch (ch) {
      case '>':
	sel_item_index = 0;
	if (next_page())
	    return DS_DISPLAY;
	break;
      case '<':
	sel_item_index = 0;
	if (prev_page())
	    return DS_DISPLAY;
	break;
      case '^':  case Ctl('r'):
	sel_item_index = 0;
	if (first_page())
	    return DS_DISPLAY;
	break;
      case '$':
	sel_item_index = 0;
	if (last_page())
	    return DS_DISPLAY;
	break;
      case Ctl('l'):
	return DS_DISPLAY;
      case Ctl('^'):
	erase_line(0);		/* erase the prompt */
	removed_prompt |= 2;
#ifdef MAILCALL
	setmail(TRUE);		/* force a mail check */
#endif
	break;
      case '\r':  case '\n':
	if (!selected_count && sel_page_item_cnt) {
	    if (sel_rereading || sel_items[sel_item_index].sel != 2)
		select_item(sel_items[sel_item_index].u);
	}
	return DS_QUIT;
      case 'Z':  case '\t':
	return DS_QUIT;
      case 'q':  case 'Q':  case '+':  case '`':
	return DS_QUIT;
      case Ctl('Q'):  case '\033':
	sel_ret = '\033';
	return DS_QUIT;
      case '\b':  case '\177':
	return DS_QUIT;
      case Ctl('k'):
	edit_kfile();
	return DS_DISPLAY;
      case '&':  case '!':
	erase_line(mousebar_cnt > 0);	/* erase the prompt */
	removed_prompt = 3;
	if (!finish_command(TRUE)) {	/* get rest of command */
	    if (clean_screen)
		break;
	}
	else {
	    PUSH_SELECTOR();
	    one_command = TRUE;
	    perform(buf, FALSE);
	    one_command = FALSE;
	    if (term_line != sel_last_line+1 || term_scrolled)
		clean_screen = FALSE;
	    POP_SELECTOR();
	    if (!save_sel_mode)
		return DS_RESTART;
	    if (clean_screen) {
		erase_line(0);
		return DS_ASK;
	    }
	}
	if ((ch = another_command(1)) != '\0')
	    goto do_command;
	return DS_DISPLAY;
      case 'v':
	newline();
	trn_version();
	if ((ch = another_command(1)) != '\0')
	    goto do_command;
	return DS_DISPLAY;
      case '\\':
	erase_line(mousebar_cnt > 0);	/* erase the prompt */
	removed_prompt = 3;
	if (sel_mode == SM_NEWSGROUP)
	    printf("[%s] Cmd: ", ngptr? ngptr->rcline : "*End*");
	else
	    fputs("Cmd: ", stdout);
	fflush(stdout);
	getcmd(buf);
	if (*buf == '\\')
	    goto the_default;
	if (*buf != ' ' && *buf != '\n' && *buf != '\r') {
	    ch = *buf;
	    goto do_command;
	}
	if (clean_screen) {
	    erase_line(0);
	    break;
	}
	if ((ch = another_command(1)) != '\0')
	    goto do_command;
	return DS_DISPLAY;
      default:
      the_default:
	ret = extra_commands(ch);
	switch (ret) {
	  case DS_ERROR:
	    break;
	  case DS_DOCOMMAND:
	    ch = sel_ret;
	    goto do_command;
	  default:
	    return ret;
	}
	strcpy(msg,"Type ? for help.");
	settle_down();
	if (clean_screen)
	    return DS_STATUS;
	printf("\n%s\n",msg);
	if ((ch = another_command(1)) != '\0')
	    goto do_command;
	return DS_DISPLAY;
    }
    return DS_ASK;
}

static bool
sel_perform_change(cnt, obj_type)
long cnt;
char* obj_type;
{
    int ret;

    carriage_return();
    if (page_line == 1) {
	disp_status_line = 1;
	if (term_line != sel_last_line+1 || term_scrolled)
	    clean_screen = FALSE;
    }
    else
	clean_screen = FALSE;

    if (error_occurred) {
	print_lines(msg,NOMARKING);
	clean_screen = error_occurred = FALSE;
    }

    ret = perform_status_end(cnt, obj_type);
    if (ret)
	disp_status_line = 1; 
    if (clean_screen) {
	if (ret != 2) {
	    up_line();
	    return TRUE;
	}
    }
    else if (disp_status_line == 1) {
	print_lines(msg,NOMARKING);
	disp_status_line = 0;
    }

    init_pages(PRESERVE_PAGE);

    return FALSE;
}

#ifdef SCAN_ART
#define SPECIAL_CMD_LETTERS "<+>^$!?&:/\\hDEJLNOPqQRSUXYZ\n\r\t\033;"
#else
#define SPECIAL_CMD_LETTERS "<+>^$!?&:/\\hDEJLNOPqQRSUXYZ\n\r\t\033"
#endif

static char
another_command(ch)
char_int ch;
{
    bool skip_q = !ch;
    if (ch < 0)
	return 0;
    if (ch > 1) {
	read_tty(buf,1);
	ch = *buf;
    }
    else
	ch = pause_getcmd();
    if (ch != 0 && ch != '\n' && ch != '\r' && (!skip_q || ch != 'q')) {
	if (ch > 0) {
	    /* try to optimize the screen update for some commands. */
	    if (!index(sel_chars, ch)
	     && (index(SPECIAL_CMD_LETTERS, ch) || ch == Ctl('k'))) {
		sel_ret = ch;
		return ch;
	    }
	    pushchar(ch | 0200);
	}
    }
    return '\0';
}

static int
article_commands(ch)
char_int ch;
{
    switch (ch) {
      case 'U':
	sel_cleanup();
	sel_rereading = !sel_rereading;
	sel_page_sp = NULL;
	sel_page_app = NULL;
	if (!cache_range(sel_rereading? absfirst : firstart, lastart))
	    sel_rereading = !sel_rereading;
	return DS_RESTART;
      case '#':
	if (sel_page_item_cnt) {
	    register SUBJECT* sp;
	    for (sp = first_subject; sp; sp = sp->next)
		sp->flags &= ~SF_SEL;
	    selected_count = 0;
	    deselect_item(sel_items[sel_item_index].u);
	    select_item(sel_items[sel_item_index].u);
	    if (!keep_the_group_static)
		keep_the_group_static = 2;
	}
	return DS_QUIT;
      case 'N':  case 'P':
	return DS_QUIT;
      case 'L':
	switch_dmode(&sel_art_dmode);	    /* sets msg */
	return DS_DISPLAY;
      case 'Y':
	if (!dmcount) {
	    strcpy(msg,"No marked articles to yank back.");
	    return DS_STATUS;
	}
	yankback();
	if (!sel_rereading)
	    sel_cleanup();
	disp_status_line = 1;
	count_subjects(CS_NORM);
	sel_page_sp = NULL;
	sel_page_app = NULL;
	init_pages(PRESERVE_PAGE);
	return DS_DISPLAY;
      case '=':
	if (!sel_rereading)
	    sel_cleanup();
	if (sel_mode == SM_ARTICLE) {
	    set_selector(sel_threadmode, 0);
	    sel_page_sp = sel_page_app? sel_page_app[0]->subj : NULL;
	} else {
	    set_selector(SM_ARTICLE, 0);
	    sel_page_app = 0;
	}
	count_subjects(CS_NORM);
	sel_item_index = 0;
	init_pages(FILL_LAST_PAGE);
	return DS_DISPLAY;
      case 'S':
	if (!sel_rereading)
	    sel_cleanup();
	erase_line(mousebar_cnt > 0);	/* erase the prompt */
	removed_prompt = 3;
      reask_output:
	in_char("Selector mode:  Threads, Subjects, Articles?", 'o', "tsa");
#ifdef VERIFY
	printcmd();
#endif
	if (*buf == 'h' || *buf == 'H') {
#ifdef VERBOSE
	    IF(verbose)
		fputs("\n\
Type t or SP to display/select thread groups (threads the group, if needed).\n\
Type s to display/select subject groups.\n\
Type a to display/select individual articles.\n\
Type q to leave things as they are.\n\n\
",stdout) FLUSH;
	    ELSE
#endif
#ifdef TERSE
		fputs("\n\
t or SP selects thread groups (threads the group too).\n\
s selects subject groups.\n\
a selects individual articles.\n\
q does nothing.\n\n\
",stdout) FLUSH;
#endif
	    clean_screen = FALSE;
	    goto reask_output;
	} else if (*buf == 'q') {
	    if (can_home)
		erase_line(0);
	    return DS_ASK;
	}
	if (isupper(*buf))
	    *buf = tolower(*buf);
	set_sel_mode(*buf);
	count_subjects(CS_NORM);
	init_pages(FILL_LAST_PAGE);
	return DS_DISPLAY;
      case 'O':
	if (!sel_rereading)
	    sel_cleanup();
	erase_line(mousebar_cnt > 0);	/* erase the prompt */
	removed_prompt = 3;
      reask_sort:
	if (sel_mode == SM_ARTICLE)
	    in_char(
	       "Order by Date,Subject,Author,Number,subj-date Groups,Points?",
		    'q', "dsangpDSANGP");
	else
	    in_char("Order by Date, Subject, Count, Lines, or Points?",
		    'q', "dsclpDSCLP");
#ifdef VERIFY
	printcmd();
#endif
	if (*buf == 'h' || *buf == 'H') {
#ifdef VERBOSE
	    IF(verbose) {
		fputs("\n\
Type d or SP to order the displayed items by date.\n\
Type s to order the items by subject.\n\
Type p to order the items by score points.\n\
",stdout) FLUSH;
		if (sel_mode == SM_ARTICLE)
		    fputs("\
Type a to order the items by author.\n\
Type n to order the items by number.\n\
Type g to order the items in subject-groups by date.\n\
",stdout) FLUSH;
		else
		    fputs("\
Type c to order the items by count.\n\
",stdout) FLUSH;
		fputs("\
Typing your selection in upper case it will reverse the selected order.\n\
Type q to leave things as they are.\n\n\
",stdout) FLUSH;
	    }
	    ELSE
#endif
#ifdef TERSE
	    {
		fputs("\n\
d or SP sorts by date.\n\
s sorts by subject.\n\
p sorts by points.\n\
",stdout) FLUSH;
		if (sel_mode == SM_ARTICLE)
		    fputs("\
a sorts by author.\n\
g sorts in subject-groups by date.\n\
",stdout) FLUSH;
		else
		    fputs("\
c sorts by count.\n\
",stdout) FLUSH;
		fputs("\
Upper case reverses the sort.\n\
q does nothing.\n\n\
",stdout) FLUSH;
	    }
#endif
	    clean_screen = FALSE;
	    goto reask_sort;
	} else if (*buf == 'q') {
	    if (can_home)
		erase_line(0);
	    return DS_ASK;
	}
	set_sel_sort(sel_mode,*buf);
	count_subjects(CS_NORM);
	sel_page_sp = NULL;
	sel_page_app = NULL;
	init_pages(FILL_LAST_PAGE);
	return DS_DISPLAY;
      case 'R':
	if (!sel_rereading)
	    sel_cleanup();
	set_selector(sel_mode, sel_sort * -sel_direction);
	count_subjects(CS_NORM);
	sel_page_sp = NULL;
	sel_page_app = NULL;
	init_pages(FILL_LAST_PAGE);
	return DS_DISPLAY;
      case 'E':
	if (!sel_rereading)
	    sel_cleanup();
	sel_exclusive = !sel_exclusive;
	count_subjects(CS_NORM);
	sel_page_sp = NULL;
	sel_page_app = NULL;
	init_pages(FILL_LAST_PAGE);
	sel_item_index = 0;
	return DS_DISPLAY;
      case 'X':  case 'D':  case 'J':
	if (!sel_rereading) {
	    if (sel_mode == SM_ARTICLE) {
		register ARTICLE* ap;
		register ARTICLE** app;
		register ARTICLE** limit;
		limit = artptr_list + artptr_list_size;
		if (ch == 'D')
		    app = sel_page_app;
		else
		    app = artptr_list;
		while (app < limit) {
		    ap = *app;
		    if ((!(ap->flags & AF_SEL) ^ (ch == 'J'))
		     || (ap->flags & AF_DEL))
			if (ch == 'J' || !sel_exclusive
			 || (ap->flags & AF_INCLUDED)) {
			    set_read(ap);
			}
		    app++;
		    if (ch == 'D' && app == sel_next_app)
			break;
		}
	    } else {
		register SUBJECT* sp;
		if (ch == 'D')
		    sp = sel_page_sp;
		else
		    sp = first_subject;
		while (sp) {
		    if (((!(sp->flags & SF_SEL) ^ (ch == 'J')) && sp->misc)
		     || (sp->flags & SF_DEL)) {
			if (ch == 'J' || !sel_exclusive
			 || (sp->flags & SF_INCLUDED)) {
			    kill_subject(sp, ch=='J'? AFFECT_ALL:AFFECT_UNSEL);
			}
		    }
		    sp = sp->next;
		    if (ch == 'D' && sp == sel_next_sp)
			break;
		}
	    }
	    count_subjects(CS_UNSELECT);
	    if (obj_count && (ch == 'J' || (ch == 'D' && !selected_count))) {
		init_pages(FILL_LAST_PAGE);
		sel_item_index = 0;
		return DS_DISPLAY;
	    }
	    if (artptr_list && obj_count)
		sort_articles();
	} else if (ch == 'J') {
	    register SUBJECT* sp;
	    for (sp = first_subject; sp; sp = sp->next)
		deselect_subject(sp);
	    selected_subj_cnt = selected_count = 0;
	    return DS_DISPLAY;
	}
	return DS_QUIT;
      case 'T':
	if (!ThreadedGroup) {
	    strcpy(msg,"Group is not threaded.");
	    return DS_STATUS;
	}
	/* FALL THROUGH */
      case 'A':
	if (!sel_page_item_cnt) {
	    dingaling();
	    break;
	}
	erase_line(mousebar_cnt > 0);	/* erase the prompt */
	removed_prompt = 3;
	if (sel_mode == SM_ARTICLE)
	    artp = sel_items[sel_item_index].u.ap;
	else {
	    register SUBJECT* sp = sel_items[sel_item_index].u.sp;
	    if (sel_mode == SM_THREAD) {
		while (!sp->misc)
		    sp = sp->thread_link;
	    }
	    artp = sp->articles;
	}
	art = article_num(artp);
	/* This call executes the action too */
	switch (ask_memorize(ch)) {
	  case 'J': case 'j': case 'K':  case ',':
	    count_subjects(sel_rereading? CS_NORM : CS_UNSELECT);
	    init_pages(PRESERVE_PAGE);
	    strcpy(msg,"Kill memorized.");
	    disp_status_line = 1;
	    return DS_DISPLAY;
	  case '+': case '.': case 'S': case 'm':
	    strcpy(msg,"Selection memorized.");
	    disp_status_line = 1;
	    return DS_UPDATE;
	  case 'c':  case 'C':
	    strcpy(msg,"Auto-commands cleared.");
	    disp_status_line = 1;
	    return DS_UPDATE;
	  case 'q':
	    return DS_UPDATE;
	  case 'Q':
	    break;
	}
	if (can_home)
	    erase_line(0);
	break;
#ifdef SCAN_ART
      case ';':
	sel_ret = ';';
	return DS_QUIT;
#endif
      case ':':
	if (sel_page_item_cnt) {
	    if (sel_mode == SM_ARTICLE)
		artp = sel_items[sel_item_index].u.ap;
	    else {
		register SUBJECT* sp = sel_items[sel_item_index].u.sp;
		if (sel_mode == SM_THREAD) {
		    while (!sp->misc)
			sp = sp->thread_link;
		}
		artp = sp->articles;
	    }
	    art = article_num(artp);
	}
	else
	    art = 0;
	/* FALL THROUGH */
      case '/':
	erase_line(mousebar_cnt > 0);	/* erase the prompt */
	removed_prompt = 3;
	if (!finish_command(TRUE)) {	/* get rest of command */
	    if (clean_screen)
		break;
	}
	else {
	    if (ch == ':') {
		thread_perform();
		if (!sel_rereading) {
		    register SUBJECT* sp;
		    for (sp = first_subject; sp; sp = sp->next) {
			if (sp->flags & SF_DEL) {
			    sp->flags = 0;
			    if (sel_mode == SM_THREAD)
				kill_thread(sp->thread, AFFECT_UNSEL);
			    else
				kill_subject(sp, AFFECT_UNSEL);
			}
		    }
		}
	    } else {
		/* Force the search to begin at absfirst or firstart,
		** depending upon whether they specified the 'r' option.
		*/
		art = lastart+1;
		switch (art_search(buf, sizeof buf, FALSE)) {
		  case SRCH_ERROR:
		  case SRCH_ABORT:
		    break;
		  case SRCH_INTR:
		    errormsg("Interrupted");
		    break;
		  case SRCH_DONE:
		  case SRCH_SUBJDONE:
		  case SRCH_FOUND:
		    break;
		  case SRCH_NOTFOUND:
		    errormsg("Not found.");
		    break;
		}
	    }
	    sel_item_index = 0;
	    /* Recount, in case something has changed. */
	    count_subjects(sel_rereading? CS_NORM : CS_UNSELECT);

	    if (sel_perform_change(ngptr->toread, "article"))
		return DS_UPDATE;
	    if (clean_screen)
		return DS_DISPLAY;
	}
	if (another_command(1))
	    return DS_DOCOMMAND;
	return DS_DISPLAY;
      case 'c':
	erase_line(mousebar_cnt > 0);	/* erase the prompt */
	removed_prompt = 3;
	if ((ch = ask_catchup()) == 'y' || ch == 'u') {
	    count_subjects(CS_UNSELECT);
	    if (ch != 'u' && obj_count) {
		sel_page_sp = NULL;
		sel_page_app = NULL;
		init_pages(FILL_LAST_PAGE);
		return DS_DISPLAY;
	    }
	    sel_ret = 'Z';
	    return DS_QUIT;
	}
	if (ch != 'N')
	    return DS_DISPLAY;
	if (can_home)
	    erase_line(0);
	break;
      case 'h':
      case '?':
	univ_help(UHELP_ARTSEL);
	return DS_RESTART;
      case 'H':
	newline();
	if (another_command(help_artsel()))
	    return DS_DOCOMMAND;
        return DS_DISPLAY;
      default:
	return DS_ERROR;
    }
    return DS_ASK;
}

static int
newsgroup_commands(ch)
char_int ch;
{
    switch (ch) {
      case Ctl('n'):
      case Ctl('p'):
	return DS_QUIT;
      case 'U':
	sel_cleanup();
	sel_rereading = !sel_rereading;
	sel_page_np = NULL;
	return DS_RESTART;
      case 'L':
	switch_dmode(&sel_grp_dmode);	    /* sets msg */
	if (*sel_grp_dmode != 's' && !multirc->first->datasrc->desc_sf.hp) {
	    newline();
	    return DS_RESTART;
	}
	return DS_DISPLAY;
      case 'X':  case 'D':  case 'J':
	if (!sel_rereading) {
	    register NGDATA* np;
	    if (ch == 'D')
		np = sel_page_np;
	    else
		np = first_ng;
	    while (np) {
		if (((!(np->flags&NF_SEL) ^ (ch=='J')) && np->toread!=TR_UNSUB)
		 || (np->flags & NF_DEL)) {
		    if (ch == 'J' || (np->flags & NF_INCLUDED))
			catch_up(np, 0, 0);
		    np->flags &= ~(NF_DEL|NF_SEL);
		}
		np = np->next;
		if (ch == 'D' && np == sel_next_np)
		    break;
	    }
	    if (ch == 'J' || (ch == 'D' && !selected_count)) {
		init_pages(FILL_LAST_PAGE);
		if (sel_total_obj_cnt) {
		    sel_item_index = 0;
		    return DS_DISPLAY;
		}
	    }
	} else if (ch == 'J') {
	    register NGDATA* np;
	    for (np = first_ng; np; np = np->next)
		np->flags &= ~NF_DELSEL;
	    selected_count = 0;
	    return DS_DISPLAY;
	}
	return DS_QUIT;
      case '=': {
	NGDATA* np;
	sel_cleanup();
	missing_count = 0;
	for (np = first_ng; np; np = np->next) {
	    if (np->toread > TR_UNSUB && np->toread < ng_min_toread)
		newsgroup_toread++;
	    np->abs1st = 0;
	}
	erase_line(0);
#ifdef SUPPORT_NNTP
	check_active_refetch(TRUE);
#endif
	return DS_RESTART;
      }
      case 'O':
	if (!sel_rereading)
	    sel_cleanup();
	erase_line(mousebar_cnt > 0);	/* erase the prompt */
	removed_prompt = 3;
      reask_sort:
	in_char("Order by Newsrc, Group name, or Count?", 'q', "ngcNGC");
#ifdef VERIFY
	printcmd();
#endif
	switch (*buf) {
	  case 'n': case 'N':
	    break;
	  case 'g': case 'G':
	    *buf += 's' - 'g';		/* Group name == SS_STRING */
	    break;
	  case 'c': case 'C':
	    break;
	  case 'h': case 'H':
#ifdef VERBOSE
	    IF(verbose) {
		fputs("\n\
Type n or SP to order the newsgroups in the .newsrc order.\n\
Type g to order the items by group name.\n\
Type c to order the items by count.\n\
",stdout) FLUSH;
		fputs("\
Typing your selection in upper case it will reverse the selected order.\n\
Type q to leave things as they are.\n\n\
",stdout) FLUSH;
	    }
	    ELSE
#endif
#ifdef TERSE
	    {
		fputs("\n\
n or SP sorts by .newsrc.\n\
g sorts by group name.\n\
c sorts by count.\n\
",stdout) FLUSH;
		fputs("\
Upper case reverses the sort.\n\
q does nothing.\n\n\
",stdout) FLUSH;
	    }
#endif
	    clean_screen = FALSE;
	    goto reask_sort;
	  default:
	    if (can_home)
		erase_line(0);
	    return DS_ASK;
	}
	set_sel_sort(sel_mode,*buf);
	sel_page_np = NULL;
	init_pages(FILL_LAST_PAGE);
	return DS_DISPLAY;
      case 'R':
	if (!sel_rereading)
	    sel_cleanup();
	set_selector(sel_mode, sel_sort * -sel_direction);
	sel_page_np = NULL;
	init_pages(FILL_LAST_PAGE);
	return DS_DISPLAY;
      case 'E':
	if (!sel_rereading)
	    sel_cleanup();
	sel_exclusive = !sel_exclusive;
	sel_page_np = NULL;
	init_pages(FILL_LAST_PAGE);
	sel_item_index = 0;
	return DS_DISPLAY;
      case ':':
#if 0
	if (ngptr != current_ng) {
	    recent_ng = current_ng;
	    current_ng = ngptr;
	}
	/* FALL THROUGH */
#endif
      case '/':
	erase_line(mousebar_cnt > 0);	/* erase the prompt */
	removed_prompt = 3;
	if (!finish_command(TRUE)) {	/* get rest of command */
	    if (clean_screen)
		break;
	}
	else {
	    if (ch == ':') {
		ngsel_perform();
	    } else {
#ifdef NGSEARCH
		ngptr = NULL;
		switch (ng_search(buf,FALSE)) {
		  case NGS_ERROR:
		  case NGS_ABORT:
		    break;
		  case NGS_INTR:
		    errormsg("Interrupted");
		    break;
		  case NGS_FOUND:
		  case NGS_NOTFOUND:
		  case NGS_DONE:
		    break;
		}
		ngptr = current_ng;
#else
		notincl("/");
#endif
	    }
	    sel_item_index = 0;

	    if (sel_perform_change(newsgroup_toread, "newsgroup"))
		return DS_UPDATE;
	    if (clean_screen)
		return DS_DISPLAY;
	}
	if (another_command(1))
	    return DS_DOCOMMAND;
	return DS_DISPLAY;
      case 'c':
	if (sel_item_index < sel_page_item_cnt)
	    set_ng(sel_items[sel_item_index].u.np);
	else {
	    strcpy(msg, "No newsgroup to catchup.");
	    disp_status_line = 1;
	    return DS_UPDATE;
	}
	if (ngptr != current_ng) {
	    recent_ng = current_ng;
	    current_ng = ngptr;
	}
	erase_line(mousebar_cnt > 0);	/* erase the prompt */
	removed_prompt = 3;
	if ((ch = ask_catchup()) == 'y' || ch == 'u')
	    return DS_DISPLAY;
	if (ch != 'N')
	    return DS_DISPLAY;
	if (can_home)
	    erase_line(0);
	break;
      case 'h':
      case '?':
	univ_help(UHELP_NGSEL);
	return DS_RESTART;
      case 'H':
	newline();
	if (another_command(help_ngsel()))
	    return DS_DOCOMMAND;
        return DS_DISPLAY;
#ifdef SCAN_ART
      case ';':
	sel_ret = ';';
	return DS_QUIT;
#endif
      default: {
	SEL_UNION u;
	int ret;
	bool was_at_top = !sel_prior_obj_cnt;
	PUSH_SELECTOR();
	if (!(removed_prompt & 2)) {
	    erase_line(mousebar_cnt > 0);	/* erase the prompt */
	    removed_prompt = 3;
	    printf("[%s] Cmd: ", ngptr? ngptr->rcline : "*End*");
	    fflush(stdout);
	}
	dfltcmd = "\\";
	set_mode('r','n');
	if (ch == '\\') {
	    putchar(ch);
	    fflush(stdout);
	}
	else
	    pushchar(ch | 0200);
	do {
	    ret = input_newsgroup();
	} while (ret == ING_INPUT);
	set_mode('s','w');
	POP_SELECTOR();
	switch (ret) {
#ifdef SUPPORT_NNTP
	  case ING_NOSERVER:
	    if (multirc) {
		if (!was_at_top)
		    (void) first_page();
		return DS_RESTART;
	    }
	    /* FALL THROUGH */
#endif
	  case ING_QUIT:
	    sel_ret = 'q';
	    return DS_QUIT;
	  case ING_ERROR:
	    return DS_ERROR;
	  case ING_ERASE:
	    if (clean_screen) {
		erase_line(0);
		return DS_ASK;
	    }
	    break;
	  default:
	    if (!save_sel_mode)
		return DS_RESTART;
	    if (term_line == sel_last_line)
		newline();
	    if (term_line != sel_last_line+1 || term_scrolled)
		clean_screen = FALSE;
	    break;
	}
	sel_item_index = 0;
	init_pages(PRESERVE_PAGE);
	if (ret == ING_SPECIAL && ngptr && ngptr->toread < ng_min_toread){
	    ngptr->flags |= NF_INCLUDED;
	    sel_total_obj_cnt++;
	    ret = ING_DISPLAY;
	}
	u.np = ngptr;
	if ((calc_page(u) || ret == ING_DISPLAY) && clean_screen)
	    return DS_DISPLAY;
	if (ret == ING_MESSAGE) {
	    clean_screen = 0;
	    return DS_STATUS;
	}
	if (was_at_top)
	    (void) first_page();
	if (clean_screen)
	    return DS_ASK;
	newline();
	if (another_command(1))
	    return DS_DOCOMMAND;
	return DS_DISPLAY;
      }
    }
    return DS_ASK;
}

static int
addgroup_commands(ch)
char_int ch;
{
    switch (ch) {
      case 'O':
	if (!sel_rereading)
	    sel_cleanup();
	erase_line(mousebar_cnt > 0);	/* erase the prompt */
	removed_prompt = 3;
      reask_sort:
	in_char("Order by Natural-order, Group name, or Count?", 'q', "ngcNGC");
#ifdef VERIFY
	printcmd();
#endif
	switch (*buf) {
	  case 'n': case 'N':
	    break;
	  case 'g': case 'G':
	    *buf += 's' - 'g';		/* Group name == SS_STRING */
	    break;
	  case 'c': case 'C':
	    break;
	  case 'h': case 'H':
#ifdef VERBOSE
	    IF(verbose) {
		fputs("\n\
Type n or SP to order the items in their naturally occurring order.\n\
Type g to order the items by newsgroup name.\n\
Type c to order the items by article count.\n\
",stdout) FLUSH;
		fputs("\
Typing your selection in upper case it will reverse the selected order.\n\
Type q to leave things as they are.\n\n\
",stdout) FLUSH;
	    }
	    ELSE
#endif
#ifdef TERSE
	    {
		fputs("\n\
n or SP sorts by natural order.\n\
g sorts by newsgroup name.\n\
c sorts by article count.\n\
",stdout) FLUSH;
		fputs("\
Upper case reverses the sort.\n\
q does nothing.\n\n\
",stdout) FLUSH;
	    }
#endif
	    clean_screen = FALSE;
	    goto reask_sort;
	  default:
	    if (can_home)
		erase_line(0);
	    return DS_ASK;
	}
	set_sel_sort(sel_mode,*buf);
	sel_page_np = NULL;
	init_pages(FILL_LAST_PAGE);
	return DS_DISPLAY;
      case 'U':
	sel_cleanup();
	sel_rereading = !sel_rereading;
	sel_page_gp = NULL;
	return DS_RESTART;
      case 'R':
	if (!sel_rereading)
	    sel_cleanup();
	set_selector(sel_mode, sel_sort * -sel_direction);
	sel_page_gp = NULL;
	init_pages(FILL_LAST_PAGE);
	return DS_DISPLAY;
      case 'E':
	if (!sel_rereading)
	    sel_cleanup();
	sel_exclusive = !sel_exclusive;
	sel_page_gp = NULL;
	init_pages(FILL_LAST_PAGE);
	return DS_DISPLAY;
      case 'L':
	switch_dmode(&sel_grp_dmode);	    /* sets msg */
	if (*sel_grp_dmode != 's' && !datasrc->desc_sf.hp) {
	    newline();
	    return DS_RESTART;
	}
	return DS_DISPLAY;
      case 'X':  case 'D':  case 'J':
	if (!sel_rereading) {
	    register ADDGROUP* gp;
	    if (ch == 'D')
		gp = sel_page_gp;
	    else
		gp = first_addgroup;
	    while (gp) {
		if ((!(gp->flags&AGF_SEL) ^ (ch=='J'))
		 || (gp->flags & AGF_DEL)) {
		    if (ch == 'J' || (gp->flags & AGF_INCLUDED))
			gp->flags |= AGF_EXCLUDED;
		    gp->flags &= ~(AGF_DEL|AGF_SEL);
		}
		gp = gp->next;
		if (ch == 'D' && gp == sel_next_gp)
		    break;
	    }
	    if (ch == 'J' || (ch == 'D' && !selected_count)) {
		init_pages(FILL_LAST_PAGE);
		if (sel_total_obj_cnt) {
		    sel_item_index = 0;
		    return DS_DISPLAY;
		}
	    }
	} else if (ch == 'J') {
	    register ADDGROUP* gp;
	    for (gp = first_addgroup; gp; gp = gp->next)
		gp->flags &= ~AGF_DELSEL;
	    selected_count = 0;
	    return DS_DISPLAY;
	}
	return DS_QUIT;
      case ':':
      case '/':
	erase_line(mousebar_cnt > 0);	/* erase the prompt */
	removed_prompt = 3;
	if (!finish_command(TRUE)) {	/* get rest of command */
	    if (clean_screen)
		break;
	}
	else {
	    if (ch == ':') {
		addgrp_sel_perform();
	    } else {
#ifdef NGSEARCH
		switch (ng_search(buf,FALSE)) {
		  case NGS_ERROR:
		  case NGS_ABORT:
		    break;
		  case NGS_INTR:
		    errormsg("Interrupted");
		    break;
		  case NGS_FOUND:
		  case NGS_NOTFOUND:
		  case NGS_DONE:
		    break;
		}
#else
		notincl("/");
#endif
	    }
	    sel_item_index = 0;

	    if (sel_perform_change(newsgroup_toread, "newsgroup"))
		return DS_UPDATE;
	    if (clean_screen)
		return DS_DISPLAY;
	}
	if (another_command(1))
	    return DS_DOCOMMAND;
	return DS_DISPLAY;
      case 'h':
      case '?':
	univ_help(UHELP_ADDSEL);
	return DS_RESTART;
      case 'H':
	newline();
	if (another_command(help_addsel()))
	    return DS_DOCOMMAND;
        return DS_DISPLAY;
      default:
	return DS_ERROR;
    }
    return DS_ASK;
}

static int
multirc_commands(ch)
char_int ch;
{
    switch (ch) {
      case 'R':
	set_selector(sel_mode, sel_sort * -sel_direction);
	sel_page_mp = NULL;
	init_pages(FILL_LAST_PAGE);
	return DS_DISPLAY;
      case 'E':
	if (!sel_rereading)
	    sel_cleanup();
	sel_exclusive = !sel_exclusive;
	sel_page_mp = NULL;
	init_pages(FILL_LAST_PAGE);
	return DS_DISPLAY;
      case '/':
	/*$$$$*/
	break;
      case 'h':
      case '?':
	univ_help(UHELP_MULTIRC);
	return DS_RESTART;
      case 'H':
	newline();
	if (another_command(help_multirc()))
	    return DS_DOCOMMAND;
        return DS_DISPLAY;
      default:
	return DS_ERROR;
    }
    return DS_ASK;
}

static int
option_commands(ch)
char_int ch;
{
    switch (ch) {
      case 'R':
	set_selector(sel_mode, sel_sort * -sel_direction);
	sel_page_op = 1;
	init_pages(FILL_LAST_PAGE);
	return DS_DISPLAY;
      case 'E':
	if (!sel_rereading)
	    sel_cleanup();
	sel_exclusive = !sel_exclusive;
	sel_page_op = 1;
	init_pages(FILL_LAST_PAGE);
	return DS_DISPLAY;
      case 'S':
	return DS_QUIT;
      case '/': {
	extern COMPEX optcompex;
	SEL_UNION u;
	char* s;
	char* pattern;
	int i, j;
	erase_line(mousebar_cnt > 0);	/* erase the prompt */
	removed_prompt = 3;
	if (!finish_command(TRUE))	/* get rest of command */
	    break;
	s = cpytill(buf,buf+1,'/');
	for (pattern = buf; *pattern == ' '; pattern++) ;
	if ((s = compile(&optcompex,pattern,TRUE,TRUE)) != NULL) {
	    strcpy(msg,s);
	    return DS_STATUS;
	}
	i = j = sel_items[sel_item_index].u.op;
	do {
	    if (++i > obj_count)
		i = 1;
	    if (*options_ini[i].item == '*')
		continue;
	    if (execute(&optcompex,options_ini[i].item))
		break;
	} while (i != j);
	u.op = i;
	if (!(option_flags[i] & OF_INCLUDED)) {
	    for (j = i-1; *options_ini[j].item != '*'; j--) ;
	    option_flags[j] |= OF_SEL;
	    init_pages(FILL_LAST_PAGE);
	    calc_page(u);
	    return DS_DISPLAY;
	}
	if (calc_page(u))
	    return DS_DISPLAY;
	return DS_ASK;
      }
      case 'h':
      case '?':
	univ_help(UHELP_OPTIONS);
	return DS_RESTART;
      case 'H':
	newline();
	if (another_command(help_options()))
	    return DS_DOCOMMAND;
        return DS_DISPLAY;
      default:
	return DS_ERROR;
    }
    return DS_ASK;
}

static int
universal_commands(ch)
char_int ch;
{
    switch (ch) {
      case 'R':
	set_selector(sel_mode, sel_sort * -sel_direction);
	sel_page_univ = NULL;
	init_pages(FILL_LAST_PAGE);
	return DS_DISPLAY;
      case 'E':
	if (!sel_rereading)
	    sel_cleanup();
	sel_exclusive = !sel_exclusive;
	sel_page_univ = NULL;
	init_pages(FILL_LAST_PAGE);
	return DS_DISPLAY;
#ifdef SCAN_ART
      case ';':
	sel_ret = ';';
	return DS_QUIT;
#endif
      case 'U':
	sel_cleanup();
	sel_rereading = !sel_rereading;
	sel_page_univ = NULL;
	return DS_RESTART;
      case Ctl('e'):
	univ_edit();
	univ_redofile();
	sel_cleanup();
	sel_page_univ = NULL;
	return DS_RESTART;
      case 'h':
      case '?':
	univ_help(UHELP_UNIV);
	return DS_RESTART;
      case 'H':
	newline();
	if (another_command(help_univ()))
	    return DS_DOCOMMAND;
        return DS_DISPLAY;
#ifdef SCORE
      case 'O':
	if (!sel_rereading)
	    sel_cleanup();
	erase_line(mousebar_cnt > 0);	/* erase the prompt */
	removed_prompt = 3;
      reask_sort:
	in_char("Order by Natural, or score Points?", 'q', "npNP");
#ifdef VERIFY
	printcmd();
#endif
	if (*buf == 'h' || *buf == 'H') {
#ifdef VERBOSE
	    IF(verbose) {
		fputs("\n\
Type n or SP to order the items in the natural order.\n\
Type p to order the items by score points.\n\
",stdout) FLUSH;
		fputs("\
Typing your selection in upper case it will reverse the selected order.\n\
Type q to leave things as they are.\n\n\
",stdout) FLUSH;
	    }
	    ELSE
#endif
#ifdef TERSE
	    {
		fputs("\n\
n or SP sorts by natural order.\n\
p sorts by score.\n\
",stdout) FLUSH;
		fputs("\
Upper case reverses the sort.\n\
q does nothing.\n\n\
",stdout) FLUSH;
	    }
#endif
	    clean_screen = FALSE;
	    goto reask_sort;
	} else if (*buf == 'q' ||
		   (tolower(*buf) != 'n' && tolower(*buf) != 'p')) {
	    if (can_home)
		erase_line(0);
	    return DS_ASK;
	}
	set_sel_sort(sel_mode,*buf);
	sel_page_univ = NULL;
	init_pages(FILL_LAST_PAGE);
	return DS_DISPLAY;
#endif
      case '~':
	univ_virt_pass();
	sel_cleanup();
	sel_page_univ = NULL;
	return DS_RESTART;
      default:
	break;
    }
    return DS_ERROR;
}

static void
switch_dmode(dmode_cpp)
char** dmode_cpp;
{
    char* s;

    if (!*++*dmode_cpp) {
	do {
	    --*dmode_cpp;
	} while ((*dmode_cpp)[-1] != '*');
    }
    switch (**dmode_cpp) {
      case 's':
	s = "short";
	break;
      case 'm':
	s = "medium";
	break;
      case 'd':
	s = "date";
	break;
      case 'l':
	s = "long";
	break;
    }
    sprintf(msg,"(%s display style)",s);
    disp_status_line = 1;
}

static int
find_line(y)
int y;
{
    int i;
    for (i = 0; i < sel_page_item_cnt; i++) {
	if (sel_items[i].line > y)
	    break;
    }
    if (i > 0)
	i--;
    return i;
}

/* On click:
 *    btn = 0 (left), 1 (middle), or 2 (right) + 4 if double-clicked;
 *    x = 0 to tc_COLS-1; y = 0 to tc_LINES-1;
 *    btn_clk = 0, 1, or 2 (no 4); x_clk = x; y_clk = y.
 * On release:
 *    btn = 3; x = release's x; y = release's y;
 *    btn_clk = click's 0, 1, or 2; x_clk = click's x; y_clk = click's y.
 */
void
selector_mouse(btn, x,y, btn_clk, x_clk,y_clk)
int btn;
int x, y;
int btn_clk;
int x_clk, y_clk;
{
    if (check_mousebar(btn, x,y, btn_clk, x_clk,y_clk))
	return;

    if (btn != 3) {
	/* Handle button-down event */
	switch (btn_clk) {
	  case 0:
	  case 1:
	    if (y > 0 && y < sel_last_line) {
		if (btn & 4) {
		    pushchar(btn_clk == 0? '\n' : 'Z');
		    mouse_is_down = FALSE;
		}
		else {
		    force_sel_pos = find_line(y);
		    if (UseSelNum) {
			pushchar(('0' + (force_sel_pos+1) % 10) | 0200);
			pushchar(('0' + (force_sel_pos+1)/10) | 0200);
		    } else {
			pushchar(sel_chars[force_sel_pos] | 0200);
		    }
		    if (btn == 1)
			pushchar(Ctl('g') | 0200);
		}
	    }
	    break;
	  case 2:
	    break;
	}
    }
    else {
	/* Handle the button-up event */
	switch (btn_clk) {
	  case 0:
	    if (!y)
		pushchar('<' | 0200);
	    else if (y >= sel_last_line)
		pushchar(' ');
	    else {
		int i = find_line(y);
		if (sel_items[i].line != term_line) {
		    if (UseSelNum) {
			pushchar(('0' + (i+1) % 10) | 0200);
			pushchar(('0' + (i+1) / 10) | 0200);
		    } else {
			pushchar(sel_chars[i] | 0200);
		    }
		    pushchar('-' | 0200);
		    force_sel_pos = i;
		}
	    }
	    break;
	  case 1:
	    if (!y)
		pushchar('<' | 0200);
	    else if (y >= sel_last_line)
		pushchar('>' | 0200);
	    break;
	  case 2:
	    /* move forward or backwards a page:
	     *   if cursor in top half: backwards
	     *   if cursor in bottom half: forwards
	     */
	    if (y < tc_LINES/2)
		pushchar('<' | 0200);
	    else
		pushchar('>' | 0200);
	    break;
	}
    }
}

/* Icky placement, but the PUSH/POP stuff is local to this file */
int
univ_visit_group(gname)
char* gname;
{
    PUSH_SELECTOR();

    univ_visit_group_main(gname);

    POP_SELECTOR();
    return 0;		/* later may have some error return values */
}

/* later consider returning universal_selector() value */
void
univ_visit_help(where)
int where;
{
    PUSH_SELECTOR();
    PUSH_UNIV_SELECTOR();

    univ_help_main(where);
    (void)universal_selector();

    POP_UNIV_SELECTOR();
    POP_SELECTOR();
}
