/* ng.c
 */
/* This software is copyrighted as detailed in the LICENSE file. */


#include "EXTERN.h"
#include "common.h"
#include "list.h"
#include "trn.h"
#include "term.h"
#include "final.h"
#include "env.h"
#include "util.h"
#include "util2.h"
#include "hash.h"
#include "cache.h"
#include "bits.h"
#include "artsrch.h"
#include "help.h"
#include "kfile.h"
#include "ngdata.h"
#include "nntpclient.h"
#include "datasrc.h"
#include "nntp.h"
#include "rcstuff.h"
#include "head.h"
#include "mime.h"
#include "art.h"
#include "artio.h"
#include "addng.h"
#include "ngstuff.h"
#include "intrp.h"
#include "respond.h"
#include "backpage.h"
#include "rcln.h"
#include "sw.h"
#include "last.h"
#include "search.h"
#include "rthread.h"
#include "rt-select.h"
#include "rt-wumpus.h"
#include "rt-util.h"
#include "decode.h"
#include "charsubst.h"
#ifdef SCAN
#include "scan.h"
#include "smisc.h"
#include "scanart.h"
#endif
#ifdef SCORE
#include "score.h"
#endif
#ifdef USE_TK
#include "tkstuff.h"
#endif
#include "univ.h"
#include "artstate.h"
#include "color.h"
#include "INTERN.h"
#include "ng.h"
#include "ng.ih"
#ifdef USE_FILTER
#include "filter.h"
#endif

/* art_switch() return values */

#define AS_NORM 0
#define AS_INP 1
#define AS_ASK 2
#define AS_CLEAN 3
#define AS_SA 4
#define AS_QUITNOW 5
#define AS_SV 6

int exit_code = NG_NORM;

void
ng_init()
{
    setdfltcmd();

#ifdef KILLFILES
    open_kfile(KF_GLOBAL);
#endif
#ifdef CUSTOMLINES
    init_compex(&hide_compex);
    init_compex(&page_compex);
#endif
}

/* do newsgroup pointed to by ngptr with name ngname
 *
 * The basic structure is:
 *	for each desired article
 *		for each desired page
 *			for each line on page
 *				if we need another line from file
 *					get it
 *					if it's a header line
 *						do special things
 *				for each column on page
 *					put out a character
 *				end loop
 *			end loop
 *		end loop
 *	end loop
 *
 *	(Actually, the pager is in another routine.)
 *
 * The chief problem is deciding what is meant by "desired".  Most of
 * the messiness of this routine is due to the fact that people want
 * to do unstructured things all the time.  I have used a few judicious
 * goto's where I thought it improved readability.  The rest of the messiness
 * arises from trying to be both space and time efficient.  Have fun.
 */

int
do_newsgroup(start_command)
char* start_command;			/* command to fake up first */
{
    char mode_save = mode;
    char gmode_save = gmode;
    int ret;
    char* whatnext = "%s%sWhat next? [%s]";
    bool ng_virtual = FALSE;

    set_datasrc(ngptr->rc->datasrc);

    if (chdir(datasrc->spool_dir)) {
	printf(nocd,datasrc->spool_dir) FLUSH;
	return -1;
    }

    exit_code = NG_NORM;
    kf_state &= ~(KFS_LOCAL_CHANGES | KFS_THREAD_CHANGES
		 |KFS_NORMAL_LINES  | KFS_THREAD_LINES);
    killfirst = 0;

    safefree0(extractdest);
    safefree0(extractprog);

    /* initialize the newsgroup data structures */

    sel_rereading = 0;
    sel_mask = AF_SEL;
    ret = access_ng();
    if (ret == -2)
	return NG_NOSERVER;
    if (ret <= 0)
	return NG_ERROR;

#ifdef ARTSEARCH
    srchahead = (scanon && !ThreadedGroup	/* did they say -S? */
	      && ((ART_NUM)ngptr->toread) >= scanon ? -1 : 0);
#endif

    /* FROM HERE ON, RETURN THRU CLEANUP OR WE ARE SCREWED */

    forcelast = TRUE;			/* if 0 unread, do not bomb out */
    recent_artp = curr_artp = NULL;
    recent_art = curr_art = lastart+1;
    prompt = whatnext;
#ifdef CHARSUBST
    charsubst = charsets;
#endif

    /* see if there are any special searches to do */

#ifdef KILLFILES
    open_kfile(KF_LOCAL);
# ifdef VERBOSE
    IF(verbose)
	kill_unwanted(firstart,"Processing memorized commands...\n\n", TRUE);
    ELSE
# endif
# ifdef TERSE
	kill_unwanted(firstart,"Auto-processing...\n\n",TRUE);
# endif
#endif

#ifdef USE_FILTER

    /* Tell the filter process what newsgroup we're in.  We should
       do this before sc_init, or filter_nginit will be called twice.
       (Not destructive, but potentially wasteful.) */

    filter_nginit();
#endif /* USE_FILTER */

#ifdef SCORE
#ifdef SCAN_ART
    sc_init((sa_never_initialized || sa_mode_order == 2)
	 && start_command && *start_command == ';');
#else
    sc_init(FALSE);
#endif /* SCAN_ART */
#endif /* SCORE */

    if (univ_ng_virtflag) {
	univ_ng_virtual();
	goto cleanup;
    }
    if (!selected_count)
	selected_only = FALSE;
    top_article();

    /* do they want a special top line? */

    firstline = getval("FIRSTLINE",(char*)NULL);

    /* custom line suppression, custom page ending */

#ifdef CUSTOMLINES
    if ((hideline = getval("HIDELINE",(char*)NULL)) != NULL)
	compile(&hide_compex,hideline,TRUE,TRUE);
    if ((pagestop = getval("PAGESTOP",(char*)NULL)) != NULL)
	compile(&page_compex,pagestop,TRUE,TRUE);
#endif

    /* now read each unread article */

    /* CAA: if we are going directly to an article, set things up here */
    if (ng_go_artnum) {
	ng_virtual = TRUE;
	if (ng_go_artnum >= absfirst) {
	    art = ng_go_artnum;
	    artp = article_ptr(art);
	}
	ng_go_artnum = 0;
    }
    else if (ng_go_msgid) {
	/* not implemented yet */
	ng_virtual = TRUE;
	ng_go_msgid = 0;
    }

    doing_ng = TRUE;			/* enter the twilight zone */
    ngptr->rc->flags |= RF_RCCHANGED;
    if (!unsafe_rc_saves)
	checkcount = 0;			/* do not checkpoint for a while */
    do_fseek = FALSE;			/* start 1st article at top */
    for (; art <= lastart+1; ) {	/* for each article */
	set_mode('r','a');

	/* do we need to "grow" the newsgroup? */

	if ((art > lastart || forcegrow) && !keep_the_group_static) {
	    ART_NUM oldlast = lastart;
#ifdef SUPPORT_NNTP
	    if (artsize < 0)
		nntp_finishbody(FB_SILENT);
	    if (datasrc->flags & DF_REMOTE) {
		if (datasrc->act_sf.fp || getngsize(ngptr) > lastart) {
		    if (nntp_group(ngname,ngptr) <= 0) {
			exit_code = NG_NOSERVER;
			goto cleanup;
		    }
		    grow_ng(ngptr->ngmax);
		}
	    }
	    else
#endif
		grow_ng(getngsize(ngptr));
	    if (forcelast && art > oldlast)
		art = lastart+1;
	}
	if (art != 0 || (artp && !(artp->flags & AF_TMPMEM)))
	    artp = article_find(art);
	if (start_command) {		/* do we have an initial command? */
	    if (start_command == nullstr) {
		if (UseNewsSelector >= 0
		 && !ng_virtual
		 && ngptr->toread >= (ART_UNREAD)UseNewsSelector)
		    pushchar('+');
	    }
	    else {
		hide_pending();
		pushstring(start_command, 0);
		free(start_command);
	    }
	    start_command = NULL;
	    if (input_pending()) {
		art = curr_art = lastart+1;
		artp = curr_artp = NULL;
		goto reinp_article;
	    }
	}
	if (art > lastart) {		/* are we off the end still? */
	    art = lastart + 1;		/* keep pointer references sane */
	    if (!forcelast && ngptr->toread && selected_only && !selected_count) {
		art = curr_art;
		artp = curr_artp;
		strcpy(buf, "+");
		goto article_level;
	    }
	    count_subjects(CS_RETAIN);
	    article_walk(count_unCACHED_article, 0);
	    ngptr->toread = (ART_UNREAD)obj_count;
	    if (artp != curr_artp) {
		recent_art = curr_art;	/* remember last article # (for '-') */
		curr_art = art;		/* set current article # */
		recent_artp = curr_artp;
		curr_artp = artp;
#ifdef CHARSUBST
		charsubst = charsets;
#endif
		first_view = 0;
	    }
#ifdef SCAN_ART
	    if (sa_in) {
		sa_go = TRUE;
		goto article_level;
	    }
#endif
	    if (erase_screen)
		clear();			/* clear the screen */
	    else {
		fputs("\n\n",stdout) FLUSH;
		termdown(2);
	    }
#ifdef VERBOSE
	    IF(verbose)
		printf("End of newsgroup %s.",ngname);
					/* print pseudo-article */
	    ELSE
#endif
#ifdef TERSE
		printf("End of %s",ngname);
#endif
	    if (obj_count) {
		if (selected_only)
		    printf("  (%ld + %ld articles still unread)",
			(long)selected_count,
			(long)obj_count-selected_count);
		else
		    printf("  (%ld article%s still unread)",
			(long)obj_count,PLURAL(obj_count));
	    }
	    if (redirected) {
		if (redirected == nullstr)
		    printf("\n\n** This group has been disabled by your news admin **");
		else
		    printf("\n\n** Please start using %s **", redirected);
		termdown(2);
	    }
	    else if (!obj_count && !forcelast)
		goto cleanup;		/* actually exit newsgroup */
	    set_mode(gmode,'e');
	    prompt = whatnext;
#ifdef ARTSEARCH
	    srchahead = 0;		/* no more subject search mode */
#endif
	    fputs("\n\n",stdout) FLUSH;
	    termdown(2);
	}
	else if (!reread && (was_read(art)
		|| (selected_only && !(artp->flags & AF_SEL)))) {
					/* has this article been read? */
	    inc_art(selected_only,FALSE);/* then skip it */
	    continue;
	}
	else if (!reread && (!(artp->flags & AF_EXISTS) || !parseheader(art))) {
	    oneless(artp);		/* mark deleted as read */
	    ng_skip();
	    continue;
	}
	else {				/* we have a real live article */
	    if (artp != curr_artp) {
		recent_art = curr_art;	/* remember last article # (for '-') */
		curr_art = art;		/* set current article # */
		recent_artp = curr_artp;
		curr_artp = artp;
#ifdef CHARSUBST
		charsubst = charsets;
#endif
		first_view = 0;
		do_hiding = TRUE;
		rotate = FALSE;
	    }
	    if (!do_fseek) {		/* starting at top of article? */
		artline = 0;		/* start at the beginning */
		topline = -1;		/* and remember top line of screen */
					/*  (line # within article file) */
	    }
	    clear();			/* clear screen */
#ifdef SUPPORT_NNTP
	    if (art == 0 && artp && artp->msgid && (datasrc->flags&DF_REMOTE)
	     && !(artp->flags & AF_CACHED)) {
		art = nntp_stat_id(artp->msgid);
		if (art < 0) {
		    exit_code = NG_NOSERVER;
		    goto cleanup;
		}
		if (art)
		    artp = article_find(art);
	    }
#endif
	    /* make sure article is found & open */
	    if (!artopen(art,(ART_POS)0)) {
		char tmpbuf[256];
		ART_LINE linenum;
		/* see if we have tree data for this article anyway */
		init_tree();
		sprintf(tmpbuf,"%s: article is not available.",ngname);
		if (artp && !(artp->flags & AF_CACHED)) {
		    if (absfirst < first_cached || last_cached < lastart
		     || !cached_all_in_range)
			sprintf(tmpbuf,"%s: article may show up in a moment.",
				ngname);
		}
		linenum = tree_puts(tmpbuf,0,0);
		vwtary(artline,(ART_POS)0);
		finish_tree(linenum);
		prompt = whatnext;
#ifdef ARTSEARCH
		srchahead = 0;
#endif
	    }
	    else {			/* found it, so print it */
		switch (do_article()) {
		case DA_CLEAN:		/* quit newsgroup */
		    goto cleanup;
		case DA_TOEND:		/* do not mark as read */
		    goto reask_article; 
		case DA_RAISE:		/* reparse command at end of art */
		    goto article_level;
		case DA_NORM:		/* normal end of article */
		    break;
		}
	    }
	    if (art >= absfirst)	/* don't mark non-existant articles */
		mark_as_read(artp);	/* mark current article as read */
	}

/* if these gotos bother you, think of this as a little state machine */

reask_article:
#ifdef MAILCALL
	setmail(FALSE);
#endif
	setdfltcmd();
	if (erase_screen && erase_each_line)
	    erase_line(1);
	if (term_line >= tc_LINES) {
	    term_scrolled += term_line - tc_LINES + 1;
	    term_line = tc_LINES-1;
	}
	unflush_output();		/* disable any ^O in effect */
	/* print prompt, whatever it is */
	interp(cmd_buf, sizeof cmd_buf, mailcall);
	sprintf(buf,prompt,cmd_buf,
#ifdef CHARSUBST
		current_charsubst(),
#else
		nullstr,
#endif
		dfltcmd);
	draw_mousebar(tc_COLS - (term_line == tc_LINES-1? strlen(buf)+5 : 0), 1);
	color_string(COLOR_CMD,buf);
	putchar(' ');
	fflush(stdout);
	term_col = strlen(buf) + 1;
reinp_article:
	reread = FALSE;
	forcelast = FALSE;
	eat_typeahead();
#ifdef PENDING
	look_ahead();			/* see what we can do in advance */
	cache_until_key();
#endif
	art = curr_art;
	artp = curr_artp;
	getcmd(buf);
	if (errno || *buf == '\f') {
	    if (tc_LINES < 100 && !int_count)
		*buf = '\f';		/* on CONT fake up refresh */
	    else {
		newline();		/* but only on a crt */
		goto reask_article;
	    }
	}
article_level:
	output_chase_phrase = TRUE;  /* Allow "Chasing Xrefs..." output */
	if (mousebar_cnt)
	    clear_rest();

#ifdef SCAN_ART
	if (sa_go) {
	    switch (sa_main()) {
	      case SA_NORM:
		continue;		/* ...the article (for) loop */
	      case SA_NEXT:		/* goto next newsgroup */
		exit_code = NG_SELNEXT;
		goto cleanup;
	      case SA_PRIOR:		/* goto prior newsgroup */
		exit_code = NG_SELPRIOR;
		goto cleanup;
	      case SA_QUIT:
	      case SA_ERR:
		goto cleanup;
	      case SA_QUIT_SEL:
		exit_code = NG_ASK;
		goto cleanup;
	      case SA_FAKE:
		lastchar = buf[0];	/* needed for fake to work */
		break;			/* fall through to art_switch */
	    }
	}
#endif /* SCAN_ART */

	/* parse and process article level command */

	switch (art_switch()) {
	  case AS_INP:			/* multichar command rubbed out */
	    goto reinp_article;
	  case AS_ASK:			/* reprompt "End of article..." */
	    goto reask_article;
	  case AS_CLEAN:		/* exit newsgroup */
	    goto cleanup;
	  case AS_QUITNOW:		/* just leave, cleanup already done */
	    goto cleanup2;
	  case AS_NORM:			/* display article art */
	    break;
#ifdef SCAN_ART
	  case AS_SA:			/* go to article scan mode */
	    sa_go = TRUE;
	    goto article_level;
#endif
	}
    }					/* end of article selection loop */
    
/* shut down newsgroup */

cleanup:
#ifdef KILLFILES
    kill_unwanted(firstart,"\nCleaning up...\n\n",FALSE);
					/* do cleanup from KILL file, if any */
#endif
#ifdef SCAN_ART
    if (sa_initialized)
	sa_cleanup();
#endif
#ifdef SCORE
    if (sc_initialized)
	sc_cleanup();
#endif
    chase_xrefs(FALSE);
    if (!univ_ng_virtflag) {
#ifdef USE_TK
	if (ttcl_running)
	    ttcl_eval("endgroup");
#endif
    }

    in_ng = FALSE;			/* leave newsgroup state */
    artclose();
    if (!univ_ng_virtflag)
	newline();
    deselect_all();
    yankback();				/* do a Y command */
    bits_to_rc();			/* reconstitute .newsrc line */
cleanup2:
/* go here if already cleaned up */
    doing_ng = FALSE;			/* tell sig_catcher to cool it */
    /* XXX later, make an option for faster/less-safe virtual groups */
    if (!univ_ng_virtflag &&
	!(univ_read_virtflag && !(univ_follow || univ_follow_temp))) {
	if (!unsafe_rc_saves) {
	    if (!write_newsrcs(multirc)) /* and update .newsrc */
		get_anything();
#ifdef KILLFILES
	    update_thread_kfile();
#endif
	}
    }

#ifdef KILLFILES
    if (localkfp) {
	fclose(localkfp);
	localkfp = NULL;
    }
#endif
    set_mode(gmode_save,mode_save);
    return exit_code;
}					/* Whew! */

/* decide what to do at the end of an article */

int
art_switch()
{
    setdef(buf,dfltcmd);
#ifdef VERIFY
    printcmd();
#endif

    buf[2] = '\0';
    switch (*buf) {
      case Ctl('v'):		/* verify signature */
	verify_sig();
	return AS_ASK;
#ifdef SCAN_ART
      case ';':			/* enter ScanArticle mode */
	sa_go_explicit = TRUE;
	return AS_SA;
#endif
#ifdef SCORE
      case '"':			/* append to local SCORE file */
	buf[0] = ':';		/* enter command on next line */
	buf[1] = FINISHCMD;
	printf("\nEnter score append command or type RETURN for a menu\n");
	termdown(2);
	fflush(stdout);
	if (finish_command(TRUE))	/* command entered successfully */
	    sc_append(buf+1);
	return AS_ASK;
      case '\'':		/* execute scoring command */
	buf[0] = ':';
	buf[1] = FINISHCMD;
	printf("\nEnter scoring command or type RETURN for a menu\n");
	termdown(2);
	fflush(stdout);
	if (finish_command(TRUE))	/* command entered successfully */
	    sc_score_cmd(buf+1);
	return AS_ASK;
#endif
      case '<':			/* goto previous subject/thread */
	visit_prev_thread();
	return AS_NORM;
      case '>':			/* goto next subject/thread */
	visit_next_thread();
	return AS_NORM;
      case 'U': {		/* unread some articles */
	char* u_prompt;
	char* u_help_thread;

	if (!artp) {
	    u_help_thread = nullstr;
#ifdef VERBOSE
	    IF(verbose)
		u_prompt = "\nUnkill: +select or all?";
	    ELSE
#endif
#ifdef TERSE
		u_prompt = "\nUnkill?";
#endif
	    dfltcmd = "+anq";
	}
	else {
#ifdef VERBOSE
	    IF(verbose) {
		u_prompt = "\n\
Unkill: +select, thread, subthread, or all?";
		u_help_thread = "\
Type t or SP to mark this thread's articles as unread.\n\
Type s to mark the current article and its descendants as unread.\n";
	    }
	    ELSE
#endif
#ifdef TERSE
	    {
		u_prompt = "\nUnkill?";
		u_help_thread = "\
t or SP to mark thread unread.\n\
s to mark subthread unread.\n";
	    }
#endif
	    dfltcmd = "+tsanq";
	}
      reask_unread:
	in_char(u_prompt,'u',dfltcmd);
#ifdef VERIFY
	printcmd();
#endif
	newline();
	if (*buf == 'h') {
#ifdef VERBOSE
	    IF(verbose)
	    {
		fputs("\
Type + to enter select thread mode using all the already-read articles.\n\
(The selected threads will be marked as unread and displayed as usual.)\n\
",stdout) FLUSH;
		fputs(u_help_thread,stdout);
		fputs("\
Type a to mark all articles in this group as unread.\n\
Type n or q to change nothing.\n\
",stdout) FLUSH;
		termdown(6);
	    }
	    ELSE
#endif
#ifdef TERSE
	    {
		fputs("\
+ to select threads from the unread.\n\
",stdout) FLUSH;
		fputs(u_help_thread,stdout);
		fputs("\
a to mark all articles unread.\n\
n or q to change nothing.\n\
",stdout) FLUSH;
		termdown(5);
	    }
#endif
	    goto reask_unread;
	}
	else if (*buf == 'n' || *buf == 'q')
	    return AS_ASK;
	else if (*buf == 't' && u_help_thread != nullstr) {
	    if (artp->subj->thread)
		unkill_thread(artp->subj->thread);
	    else
		unkill_subject(artp->subj);
	    if ((artp = first_art(artp->subj)) != NULL)
		art = article_num(artp);
	}
	else if (*buf == 's' && u_help_thread != nullstr)
	    unkill_subthread(artp);
	else if (*buf == 'a') {
	    check_first(absfirst);
	    article_walk(mark_all_unREAD, 0);
	    count_subjects(CS_NORM);
	    ngptr->toread = (ART_UNREAD)obj_count;
	}
	else if (*buf == '+') {
	    *buf = 'U';
	    goto run_the_selector;
	}
	else {
	    fputs(hforhelp,stdout) FLUSH;
	    termdown(1);
	    settle_down();
	    goto reask_unread;
	}
	return AS_NORM;
      }
      case '[':			/* goto parent article */
      case '{':			/* goto thread's root article */
	if (artp && ThreadedGroup) {
	    if (!find_parent(*buf == '{')) {
		register char* cp = (*buf=='['?"parent":"root");
#ifdef VERBOSE
		IF(verbose)
		    printf("\nThere is no %s article prior to this one.\n",
			cp) FLUSH;
		ELSE
#endif
#ifdef TERSE
		    printf("\nNo prior %s.\n",cp) FLUSH;
#endif
		termdown(2);
		return AS_ASK;
	    }
	    reread = TRUE;
#ifdef SCAN
	    s_follow_temp = TRUE;
#endif
	    univ_follow_temp = TRUE;
	    return AS_NORM;
	}
not_threaded:
	if (!artp) {
#ifdef VERBOSE
	    IF(verbose)
		fputs("\nYou're at the end of the group.\n",stdout) FLUSH;
	    ELSE
#endif
#ifdef TERSE
		fputs("\nEnd of group.\n",stdout) FLUSH;
#endif
	    termdown(2);
	    return AS_ASK;
	}
#ifdef VERBOSE
	IF(verbose)
	    fputs("\nThis group is not threaded.\n",stdout) FLUSH;
	ELSE
#endif
#ifdef TERSE
	    fputs("\nUnthreaded group.\n",stdout) FLUSH;
#endif
	termdown(2);
	return AS_ASK;
      case ']':			/* goto child article */
      case '}':			/* goto thread's leaf article */
	if (artp && ThreadedGroup) {
	    if (!find_leaf(*buf == '}')) {
#ifdef VERBOSE
		IF(verbose)
		    fputs("\n\
This is the last leaf in this tree.\n",stdout) FLUSH;
		ELSE
#endif
#ifdef TERSE
		    fputs("\nLast leaf.\n",stdout) FLUSH;
#endif
		termdown(2);
		return AS_ASK;
	    }
	    reread = TRUE;
#ifdef SCAN
	    s_follow_temp = TRUE;
#endif
	    univ_follow_temp = TRUE;
	    return AS_NORM;
	}
	goto not_threaded;
      case '(':			/* goto previous sibling */
      case ')':			/* goto next sibling */
	if (artp && ThreadedGroup) {
	    if (!(*buf == '(' ? find_prev_sib() : find_next_sib())) {
		register char* cp = (*buf == '(' ? "previous" : "next");
#ifdef VERBOSE
		IF(verbose)
		    printf("\nThis article has no %s sibling.\n",cp) FLUSH;
		ELSE
#endif
#ifdef TERSE
		    printf("\nNo %s sibling.\n",cp) FLUSH;
#endif
		termdown(2);
		return AS_ASK;
	    }
	    reread = TRUE;
#ifdef SCAN
	    s_follow_temp = TRUE;
#endif
	    univ_follow_temp = TRUE;
	    return AS_NORM;
	}
	goto not_threaded;
      case 'T':
	if (!ThreadedGroup)
	    goto not_threaded;
	/* FALL THROUGH */
      case 'A':
	if (!artp)
	    goto not_threaded;
	switch (ask_memorize(*buf)) {
	  case ',':  case 'J': case 'K': case 'j':
	    return AS_NORM;
	}
	return AS_ASK;
    case 'K':
	if (!artp)
	    goto not_threaded;
	/* first, write kill-subject command */
	(void)art_search(buf, (sizeof buf), TRUE);
	art = curr_art;
	artp = curr_artp;
	kill_subject(artp->subj,AFFECT_ALL);/* take care of any prior subjects */
#ifdef SCAN_ART
	if (sa_in && !(sa_follow || s_follow_temp))
	    return AS_SA;
#endif
	return AS_NORM;
      case ',':		/* kill this node and all descendants */
	if (!artp)
	    goto not_threaded;
	if (ThreadedGroup)
	    kill_subthread(artp,AFFECT_ALL);
	else if (art >= absfirst && art <= lastart)
	    mark_as_read(artp);
#ifdef SCAN_ART
	if (sa_in && !(sa_follow || s_follow_temp))
	    return AS_SA;
#endif
	return AS_NORM;
      case 'J':		/* Junk all nodes in this thread */
	if (!artp)
	    goto not_threaded;
	if (ThreadedGroup) {
	    kill_thread(artp->subj->thread,AFFECT_ALL);
#ifdef SCAN_ART
	    if (sa_in)
		return AS_SA;
#endif
	    return AS_NORM;
	}
	/* FALL THROUGH */
      case 'k':		/* kill current subject */
	if (!artp)
	    goto not_threaded;
	kill_subject(artp->subj,AFFECT_ALL);
	if (!ThreadedGroup || last_cached < lastart) {
	    *buf = 'k';
	    goto normal_search;
	}
#ifdef SCAN_ART
	if (sa_in && !(sa_follow || s_follow_temp))
	    return AS_SA;
#endif
	return AS_NORM;
      case 't':
	erase_line(erase_screen && erase_each_line);
	page_line = 1;
	entire_tree(curr_artp);
	return AS_ASK;
      case ':':			/* execute command on selected articles */
	page_line = 1;
	if (!thread_perform())
	    return AS_INP;
	carriage_return();
	perform_status_end(ngptr->toread, "article");
	fputs(msg, stdout);
	newline();
	art = curr_art;
	artp = curr_artp;
	return AS_ASK;
      case 'p':			/* find previous unread article */
#ifdef SCAN
	s_follow_temp = TRUE;	/* keep going until change req. */
#endif
	univ_follow_temp = TRUE;
	do {
	    dec_art(selected_only,FALSE);
	} while (art >= firstart && (was_read(art) || !parseheader(art)));
#ifdef ARTSEARCH
	srchahead = 0;
#endif
	if (art >= firstart)
	    return AS_NORM;
	art = absfirst;	
	/* FALL THROUGH */
      case 'P':		/* goto previous article */
#ifdef SCAN
	s_follow_temp = TRUE;	/* keep going until change req. */
#endif
	univ_follow_temp = TRUE;
	dec_art(FALSE,TRUE);
      check_dec_art:
	if (art < absfirst) {
#ifdef VERBOSE
	    IF(verbose)
		printf("\nThere are no%s%s articles prior to this one.\n",
			*buf=='P'?nullstr:" unread",
			selected_only?" selected":nullstr) FLUSH;
	    ELSE
#endif
#ifdef TERSE
		printf("\nNo previous%s%s articles\n",
			*buf=='P'?nullstr:" unread",
			selected_only?" selected":nullstr) FLUSH;
#endif
	    termdown(2);
	    art = curr_art;
	    artp = curr_artp;
	    return AS_ASK;
	}
	reread = TRUE;
#ifdef ARTSEARCH
	srchahead = 0;
#endif
	return AS_NORM;
      case '-':
      case '\b':  case '\177':
	if (recent_art >= 0) {
	    art = recent_art;
	    artp = recent_artp;
	    reread = TRUE;
	    forcelast = TRUE;
#ifdef ARTSEARCH
	    srchahead = -(srchahead != 0);
#endif
	    return AS_NORM;
	}
	exit_code = NG_MINUS;
	return AS_CLEAN;
      case 'n':		/* find next unread article? */
#ifdef SCAN_ART
	if (sa_in && s_default_cmd && !(sa_follow || s_follow_temp))
	    return AS_SA;
#endif
	if (univ_read_virtflag && univ_default_cmd &&
#ifdef SCAN_ART
	    !(sa_in && (sa_follow || s_follow_temp)) &&
#endif
	    !(univ_follow || univ_follow_temp)) {
	    exit_code = NG_NEXT;
	    return AS_CLEAN;
	}
	if (!univ_default_cmd)
	    univ_follow_temp = TRUE;
#ifdef SCAN
	if (!s_default_cmd)
	    s_follow_temp = TRUE;	/* keep going until change req. */
#endif
	if (art > lastart) {
	    if (!ngptr->toread)
		return AS_CLEAN;
	    top_article();
#ifdef SCAN_ART
	    if (sa_in)
		return AS_SA;
#endif
	}
#ifdef ARTSEARCH
	else if (scanon && !ThreadedGroup && srchahead) {
	    *buf = Ctl('n');
	    if (!next_art_with_subj())
		goto normal_search;
	    return AS_NORM;
	}
#endif
	else {
#ifdef SCAN_ART
	    /* $$ will this work with 4.0? CAA */
	    if (sa_in && ThreadedGroup) {
		ARTICLE* old_artp = artp;
		inc_art(selected_only,FALSE);
		if (!artp || !old_artp)
		    return AS_SA;
		switch (sel_mode) {
		  case SM_ARTICLE:
		    if (s_default_cmd)
			return AS_SA;
		    break;
		  case SM_SUBJECT:
		    if (old_artp->subj != artp->subj)
			return AS_SA;
		    break;
		  case SM_THREAD:
		    if (old_artp->subj->thread != artp->subj->thread)
			return AS_SA;
		    break;
		  default:
		    /* HUH?  Just hope for the best */
		    break;
		}
	    } else
#endif
		inc_art(selected_only,FALSE);
	    if (art > lastart)
		top_article();
	}
#ifdef ARTSEARCH
	srchahead = 0;
#endif
	return AS_NORM;
      case 'N':			/* goto next article */
#ifdef SCAN_ART
	if (sa_in && s_default_cmd && !(sa_follow || s_follow_temp))
	    return AS_SA;
#endif
	if (univ_read_virtflag && univ_default_cmd &&
#ifdef SCAN_ART
	    !(sa_in && (sa_follow || s_follow_temp)) &&
#endif
	    !(univ_follow || univ_follow_temp)) {
	    exit_code = NG_NEXT;
	    return AS_CLEAN;
	}
	if (!univ_default_cmd)
	    univ_follow_temp = TRUE;
#ifdef SCAN
	if (!s_default_cmd)
	    s_follow_temp = TRUE;	/* keep going until change req. */
#endif
	if (art > lastart) {
	    if (!first_subject) {
		art = absfirst;
		artp = article_ptr(art);
	    } else {
		artp = first_subject->articles;
		if (artp->flags & AF_EXISTS)
		    art = article_num(artp);
		else
		    inc_art(FALSE,TRUE);
	    }
	}
	else
	    inc_art(FALSE,TRUE);
	if (art <= lastart)
	    reread = TRUE;
	else
	    forcelast = TRUE;
#ifdef ARTSEARCH
	srchahead = 0;
#endif
	return AS_NORM;
      case '$':
	art = lastart+1;
	artp = NULL;
	forcelast = TRUE;
#ifdef ARTSEARCH
	srchahead = 0;
#endif
	return AS_NORM;
      case '0':
      case '1': case '2': case '3':	/* goto specified article */
      case '4': case '5': case '6':	/* or do something with a range */
      case '7': case '8': case '9': case '.':
	forcelast = TRUE;
	switch (numnum()) {
	  case NN_INP:
	    return AS_INP;
	  case NN_ASK:
	    return AS_ASK;
	  case NN_REREAD:
	    reread = TRUE;
#ifdef ARTSEARCH
	    if (srchahead)
		srchahead = -1;
#endif
	    break;
	  case NN_NORM:
	    if (use_threads) {
		erase_line(0);
		perform_status_end(ngptr->toread, "article");
		fputs(msg, stdout) FLUSH;
	    }
	    newline();
	    return AS_ASK;
	}
	return AS_NORM;
      case Ctl('k'):
	edit_kfile();
	return AS_ASK;
      case Ctl('n'):	/* search for next article with same subject */
      case Ctl('p'):	/* search for previous article with same subject */
#ifdef SCAN_ART
	if (sa_in && s_default_cmd && *buf == Ctl('n')
	 && !(sa_follow || s_follow_temp))
	    return AS_SA;
#endif
	if (univ_read_virtflag && univ_default_cmd &&
	    (*buf == Ctl('n')) &&
#ifdef SCAN_ART
	    !(sa_in && (sa_follow || s_follow_temp)) &&
#endif
	    !(univ_follow || univ_follow_temp)) {
	    exit_code = NG_NEXT;
	    return AS_CLEAN;
	}
	if (!univ_default_cmd)
	    univ_follow_temp = TRUE;
#ifdef SCAN
	if (!s_default_cmd)
	    s_follow_temp = TRUE;	/* keep going until change req. */
#endif
	if (*buf == Ctl('n')? next_art_with_subj() : prev_art_with_subj())
	    return AS_NORM;
      case '/': case '?':
normal_search:
#ifdef ARTSEARCH
      {		/* search for article by pattern */
	char cmd = *buf;
	
	reread = TRUE;		/* assume this */
	page_line = 1;
	switch (art_search(buf, (sizeof buf), TRUE)) {
	  case SRCH_ERROR:
	    art = curr_art;
	    return AS_ASK;
	  case SRCH_ABORT:
	    art = curr_art;
	    return AS_INP;
	  case SRCH_INTR:
#ifdef VERBOSE
	    IF(verbose)
		printf("\n(Interrupted at article %ld)\n",(long)art) FLUSH;
	    ELSE
#endif
#ifdef TERSE
		printf("\n(Intr at %ld)\n",(long)art) FLUSH;
#endif
	    termdown(2);
	    art = curr_art;	    /* restore to current article */
	    return AS_ASK;
	  case SRCH_DONE:
	    if (use_threads) {
		erase_line(0);
		perform_status_end(ngptr->toread, "article");
		printf("%s\n",msg) FLUSH;
	    }
	    else
		fputs("done\n",stdout) FLUSH;
	    termdown(1);
	    pad(just_a_sec/3);	/* 1/3 second */
	    if (!srchahead) {
		art = curr_art;
		return AS_ASK;
	    }
	    top_article();
	    reread = FALSE;
	    return AS_NORM;
	  case SRCH_SUBJDONE:
#ifdef SCAN_ART
	    if (sa_in)
		return AS_SA;
#endif
	    top_article();
	    reread = FALSE;
	    return AS_NORM;
	  case SRCH_NOTFOUND:
	    fputs("\n\n\n\nNot found.\n",stdout) FLUSH;
	    termdown(5);
	    art = curr_art;  /* restore to current article */
#ifdef SCAN_ART
	    if (sa_in)
		return AS_SA;
#endif
	    return AS_ASK;
	  case SRCH_FOUND:
	    if (cmd == Ctl('n') || cmd == Ctl('p')) {
		oldsubject = TRUE;
		reread = FALSE;
	    }
	    break;
	}
	return AS_NORM;
      }
#else /* !ARTSEARCH */
	buf[1] = '\0';
	notincl(buf);
	return AS_ASK;
#endif
      case 'u':			/* unsubscribe from this newsgroup? */
	newline();
	printf(unsubto,ngname) FLUSH;
	termdown(1);
	ngptr->subscribechar = NEGCHAR;
	ngptr->rc->flags |= RF_RCCHANGED;
	newsgroup_toread--;
	return AS_CLEAN;
      case 'M':
	if (art <= lastart) {
	    delay_unmark(artp);
	    oneless(artp);
	    printf("\nArticle %ld will return.\n",(long)art) FLUSH;
	    termdown(2);
	}
	return AS_ASK;
      case 'm':
	if (art >= absfirst && art <= lastart) {
	    unmark_as_read(artp);
	    printf("\nArticle %ld marked as still unread.\n",(long)art) FLUSH;
	    termdown(2);
	}
	return AS_ASK;
      case 'c':			/* catch up */
	switch (ask_catchup()) {
	case 'n':
	    return AS_ASK;
	case 'u':
	    return AS_CLEAN;
	}
	art = lastart+1;
	artp = NULL;
	forcelast = FALSE;
	return AS_NORM;
      case 'Q':  case '`':
	exit_code = NG_ASK;
	return AS_CLEAN;
      case 'q':			/* go back up to newsgroup level? */
	exit_code = NG_NEXT;
	return AS_CLEAN;
      case 'i':
	if ((auto_view_inline = !auto_view_inline) != 0)
	    first_view = 0;
	printf("\nAuto-View inlined mime is %s\n", auto_view_inline? "on" : "off");
	termdown(2);
	break;
      case 'j':
	newline();
	if (art >= absfirst && art <= lastart)
	    mark_as_read(artp);
	return AS_ASK;
      case 'h':
	univ_help(UHELP_ART);
	return AS_ASK;
      case 'H':			/* help? */
	help_art();
	return AS_ASK;
      case '&':
	if (switcheroo()) /* get rest of command */
	    return AS_INP;	/* if rubbed out, try something else */
	return AS_ASK;
      case '#':
#ifdef VERBOSE
	IF(verbose)
	    printf("\nThe last article is %ld.\n",(long)lastart) FLUSH;
	ELSE
#endif
#ifdef TERSE
	    printf("\n%ld\n",(long)lastart) FLUSH;
#endif
	termdown(2);
	return AS_ASK;
      case '+':			/* enter selection mode */
run_the_selector:
	if (art_sel_ilock) {
	    printf("\nAlready inside article selector!\n") FLUSH;
	    termdown(2);
	    return AS_ASK;
	}
#ifdef SCAN_ART
	/* modes do not mix very well, so turn off the SA mode */
	sa_in = FALSE;
#endif
#ifdef SCAN
	/* turn on temporary follow */
	s_follow_temp = TRUE;
#endif
	univ_follow_temp = TRUE;
	art_sel_ilock = TRUE;
	*buf = article_selector(*buf);
	art_sel_ilock = FALSE;
	switch (*buf) {
	  case '+':
	    newline();
	    term_scrolled = tc_LINES;
	    term_line = tc_LINES-1;
	    return AS_ASK;
	  case 'Q':
	    exit_code = NG_ASK;
	    break;
	  case 'q':
	    exit_code = NG_NEXT;
	    break;
	  case 'N':
	    exit_code = NG_SELNEXT;
	    break;
	  case 'P':
	    exit_code = NG_SELPRIOR;
	    break;
#ifdef SCAN_ART
	  case ';':
	    sa_do_selthreads = TRUE;
	    sa_go_explicit = TRUE;
	    return AS_SA;
#endif
	  default:
	    if (ngptr->toread)
		return AS_NORM;
	    break;
	}
	return AS_CLEAN;
      case '=': {		/* list subjects */
	ART_NUM oldart = art;
	page_start();
	article_walk(output_subject, AF_UNREAD);
	int_count = 0;
	subjline = NULL;
	art = oldart;
	return AS_ASK;
      }
      case '^':
	top_article();
#ifdef ARTSEARCH
	srchahead = 0;
#endif
	return AS_NORM;
#ifdef DEBUG
      case 'D':
	printf("\nFirst article: %ld\n",(long)firstart) FLUSH;
	termdown(2);
	article_walk(debug_article_output, 0);
	int_count = 0;
	return AS_ASK;
#endif
      case 'v':
	if (art <= lastart) {
	    reread = TRUE;
	    do_hiding = FALSE;
	}
	return AS_NORM;
      case Ctl('r'):
	do_hiding = TRUE;
	rotate = FALSE;
	if (art <= lastart)
	    reread = TRUE;
	else
	    forcelast = TRUE;
	return AS_NORM;
      case 'x':
      case Ctl('x'):
	/* In the future the behavior of 'x' may change back to a
	 * filter-select mechanism.
	 * Currently, both keys do ROT-13 translation.
	 */
	rotate = TRUE;
	if (art <= lastart)
	    reread = TRUE;
	else
	    forcelast = TRUE;
	return AS_NORM;
      case 'X':
	rotate = !rotate;
	/* FALL THROUGH */
      case 'l': case Ctl('l'):		/* refresh screen */
      refresh_screen:
	if (art <= lastart) {
	    reread = TRUE;
	    clear();
	    do_fseek = TRUE;
	    artline = topline;
	    if (artline < 0)
		artline = 0;
	}
	return AS_NORM;
      case Ctl('^'):
	erase_line(0);		/* erase the prompt */
#ifdef MAILCALL
	setmail(TRUE);		/* force a mail check */
#endif
	return AS_ASK;
#ifdef INNERSEARCH
      case Ctl('e'):
	if (art <= lastart) {
#ifdef SUPPORT_NNTP
	    if (artsize < 0) {
		nntp_finishbody(FB_OUTPUT);
		raw_artsize = nntp_artsize();
		artsize = raw_artsize-artbuf_seek+artbuf_len+htype[PAST_HEADER].minpos;
	    }
#endif
	    if (do_hiding) {
		seekartbuf(artsize);
		seekartbuf(artpos);
	    }
	    reread = TRUE;
	    do_fseek = TRUE;
	    topline = artline;
	    innerlight = artline - 1;
	    innersearch = artsize;
	    gline = 0;
	    hide_everything = 'b';
	}
	return AS_NORM;
#endif
      case 'B':				/* back up one line */
      case 'b': case Ctl('b'):		/* back up a page */
	if (art <= lastart) {
	    ART_LINE target;

	    reread = TRUE;
	    clear();
	    do_fseek = TRUE;
	    if (*buf == 'B')
		target = topline - 1;
	    else {
		target = topline - (tc_LINES - 2);
		if (marking && (marking_areas & BACKPAGE_MARKING)) {
		    highlight = topline;
		}
	    }
	    artline = topline;
	    if (artline >= 0) do {
		artline--;
	    } while(artline >= 0 && artline > target && vrdary(artline-1) >= 0);
	    topline = artline;
	    if (artline < 0)
		artline = 0;
	}
	return AS_NORM;
      case '!':			/* shell escape */
	if (escapade())
	    return AS_INP;
	return AS_ASK;
      case 'C':
	cancel_article();
	return AS_ASK;
      case 'Z':
      case 'z':
	supersede_article();	/* supersedes */
	return AS_ASK;
      case 'R':
      case 'r': {		/* reply? */
	reply();
	return AS_ASK;
      }
      case 'F':
      case 'f': {		/* followup command */
	followup();
	forcegrow = TRUE;		/* recalculate lastart */
	return AS_ASK;
      }
      case Ctl('f'): {			/* forward? */
	forward();
	return AS_ASK;
      }
      case '|':
      case 'w': case 'W':
      case 's': case 'S':	/* save command */
      case 'e':			/* extract command */
	if (save_article() == SAVE_ABORT)
	    return AS_INP;
	int_count = 0;
	return AS_ASK;
#if 0
      case 'E':
	if (decode_fp)
	    decode_end();
	else
	    newline();
	return AS_ASK;
#endif
      case 'a':			/* attachment-view command */
	newline();
	if (view_article() == SAVE_ABORT)
	    return AS_INP;
	int_count = 0;
	return AS_ASK;
      case 'Y':				/* yank back M articles */
	yankback();
	top_article();			/* from the beginning */
	return AS_NORM;			/* pretend nothing happened */
#ifdef STRICTCR
      case '\n':   case '\r':
	fputs(badcr,stdout) FLUSH;
	return AS_ASK;
#endif
      case '_':
	if (!finish_dblchar())
	    return AS_INP;
	switch (buf[1] & 0177) {
	  case 'P':
	    art--;
	    goto check_dec_art;
	  case 'N':
	    if (art > lastart)
		art = absfirst;
	    else
		art++;
	    if (art <= lastart)
		reread = TRUE;
#ifdef ARTSEARCH
	    srchahead = 0;
#endif
	    return AS_NORM;
	  case '+':
	    if (!artp)
		goto not_threaded;
	    if (ThreadedGroup) {
		select_arts_thread(artp, 0);
		printf("\nSelected all articles in this thread.\n");
	    } else {
		select_arts_subject(artp, 0);
		printf("\nSelected all articles in this subject.\n");
	    }
	    termdown(2);
	    if ((artp = first_art(artp->subj)) != NULL) {
		if (art == article_num(artp))
		    return AS_ASK;
		art = article_num(artp);
	    }
	    return AS_NORM;
	  case '-':
	    if (!artp)
		goto not_threaded;
	    if (sel_mode == SM_THREAD) {
		deselect_arts_thread(artp);
		printf("\nDeselected all articles in this thread.\n");
	    } else {
		deselect_arts_subject(artp);
		printf("\nDeselected all articles in this subject.\n");
	    }
	    termdown(2);
	    return AS_ASK;
#ifdef CHARSUBST
	  case 'C':
	    if (!*(++charsubst))
		charsubst = charsets;
	    goto refresh_screen;
#endif
	  case 'a':  case 's':  case 't':  case 'T':
	    *buf = buf[1];
	    goto run_the_selector;
	  case 'm':
	    if (!artp)
		goto not_threaded;
	    kill_subthread(artp, SET_TORETURN | AFFECT_ALL);
	    return AS_NORM;
	  case 'M':
	    if (!artp)
		goto not_threaded;
	    kill_arts_thread(artp, SET_TORETURN | AFFECT_ALL);
	    return AS_NORM;
	}
	/* FALL THROUGH */
      default:
	printf("\n%s",hforhelp) FLUSH;
	termdown(2);
	settle_down();
	break;
    }
    return AS_ASK;
}

/* see if there is any mail */

#ifdef MAILCALL
void
setmail(force)
bool_int force;
{
    if (force)
	mailcount = 0;
    if (!(mailcount++)) {
	char* mailfile = filexp(getval("MAILFILE",MAILFILE));
	
	if (stat(mailfile,&filestat) < 0 || !filestat.st_size
	    || filestat.st_atime > filestat.st_mtime)
	    mailcall = nullstr;
	else
	    mailcall = getval("MAILCALL","(Mail) ");
    }
    mailcount %= 5;			/* check every 5 articles */
}
#endif

void
setdfltcmd()
{
    if (!ngptr || !ngptr->toread)
	dfltcmd = "npq";
    else {
#if 0
	if (multimedia_mime == TRUE) {
	    multimedia_mime++;
	    dfltcmd = "anpq";
	} else
#endif
#ifdef ARTSEARCH
	if (srchahead)
	    dfltcmd = "^Nnpq";
	else
#endif
	    dfltcmd = "npq";
    }
}

/* Ask the user about catching-up the current group.  Returns 'y' if yes,
** 'n' or 'N' if no ('N' means we used one line when in the selector),
** or 'u' for yes with unsubscribe.  Actually performs the catchup and
** unsubscription as needed.
*/
char
ask_catchup()
{
    char ch;
    bool use_one_line = (gmode == 's');
    int leave_unread = 0;

    if (!use_one_line)
	newline();
reask_catchup:
#ifdef VERBOSE
    IF(verbose)
	sprintf(buf,"Mark everything in %s as read?",ngname);
    ELSE
#endif
#ifdef TERSE
	sprintf(buf,"Catchup %s?",ngname);
#endif
    in_char(buf,'C',"yn#h");
#ifdef VERIFY
    printcmd();
#endif
    if ((ch = *buf) == 'h' || ch == 'H') {
	use_one_line = FALSE;
#ifdef VERBOSE
	IF(verbose)
	    fputs("\n\
Type y or SP to mark all articles as read.\n\
Type n to leave articles marked as they are.\n\
The # means enter a number to mark all but the last # articles as read.\n\
Type u to mark everything read and unsubscribe.\n\n\
",stdout) FLUSH;
	ELSE
#endif
#ifdef TERSE
	    fputs("\n\
y or SP to mark all read.\n\
n to forget it.\n\
# means enter a number to leave unread.\n\
u to mark all and unsubscribe.\n\n\
",stdout) FLUSH;
#endif
	termdown(6);
	goto reask_catchup;
    }
    if (ch == 'n' || ch == 'q') {
	if (use_one_line)
	    return 'N';
	newline();
	return 'n';
    }
    if (ch == '#') {
	use_one_line = FALSE;
	in_char("\nEnter approx. number of articles to leave unread: ", 'C', "0");
	if ((ch = *buf) == '0')
	    ch = 'y';
    }
    if (isdigit(ch)) {
	buf[1] = FINISHCMD;
	if (!finish_command(FALSE)) {
	    use_one_line = FALSE;
	    newline();
	    goto reask_catchup;
	}
	else {
	    leave_unread = atoi(buf);
	    ch = 'y';
	}
    }
    if (ch != 'y' && ch != 'u') {
	use_one_line = FALSE;
	printf("\n%s\n", hforhelp) FLUSH;
	termdown(3);
	settle_down();
	goto reask_catchup;
    }
    if (in_ng) {
	article_walk(mark_all_READ, leave_unread);
	if (leave_unread) {
	    count_subjects(CS_NORM);
	    ngptr->toread = (ART_UNREAD)obj_count;
	}
	else {
	    selected_count = selected_subj_cnt = selected_only = 0;
	    ngptr->toread = 0;
	    if (dmcount)
		yankback();
	}
	newline();
    }
    else {
	newline();
	catch_up(ngptr, leave_unread, 1);
    }
    if (ch == 'u') {
	ngptr->subscribechar = NEGCHAR;
	ngptr->rc->flags |= RF_RCCHANGED;
	newsgroup_toread--;
	newline();
	printf(unsubto,ngname);
	printf("(If you meant to hit 'y' instead of 'u', press '-'.)\n") FLUSH;
	termdown(2);
    }
    return ch;
}

static bool
count_unCACHED_article(ptr, arg)
char* ptr;
int arg;
{
    register ARTICLE* ap = (ARTICLE*)ptr;
    if ((ap->flags & (AF_UNREAD|AF_CACHED)) == AF_UNREAD)
	obj_count++;
    return 0;
}

static bool
mark_all_READ(ptr, leave_unread)
char* ptr;
int leave_unread;
{
    register ARTICLE* ap = (ARTICLE*)ptr;
    if (article_num(ap) > lastart - leave_unread)
	return 1;
    ap->flags &= ~(sel_mask|AF_UNREAD);
    return 0;
}

static bool
mark_all_unREAD(ptr, arg)
char* ptr;
int arg;
{
    register ARTICLE* ap = (ARTICLE*)ptr;
    if ((ap->flags & (AF_UNREAD|AF_EXISTS)) == AF_EXISTS) {
	ap->flags |= AF_UNREAD;		/* mark as unread */
	obj_count++;
    }
    return 0;
}

bool
output_subject(ptr, flag)
char* ptr;
int flag;
{
    register ARTICLE* ap;
    register ART_NUM i;
    char tmpbuf[256];
    int len;
    char* s;

    if (int_count)
	return 1;

    if (!subjline) {
	subjline = getval("SUBJLINE",(char*)NULL);
	if (!subjline)
	    subjline = nullstr;
    }

    ap = (ARTICLE*)ptr;
    if (flag && !(ap->flags & flag))
	return 0;
    i = article_num(ap);
    if ((s = fetchsubj(i,FALSE)) != NULL) {
	sprintf(tmpbuf,"%-5ld ", i);
	len = strlen(tmpbuf);
	if (subjline != nullstr) {
	    art = i;
	    interp(tmpbuf + len, sizeof tmpbuf - len, subjline);
	}
	else
	    safecpy(tmpbuf + len, s, sizeof tmpbuf - len);
	if (mode == 'k')
	    page_line = 1;
	if (print_lines(tmpbuf,NOMARKING) != 0)
	    return 1;
    }
    return 0;
}

#ifdef DEBUG
static bool
debug_article_output(ptr, arg)
char* ptr;
int arg;
{
    register ARTICLE* ap = (ARTICLE*)ptr;
    if (int_count)
	return 1;
    if (article_num(ap) >= firstart && ap->subj) {
	printf("%5ld %c %s\n", article_num(ap),
	       (ap->flags & AF_UNREAD)? 'y' : 'n', ap->subj->str) FLUSH;
	termdown(1);
    }
    return 0;
}
#endif

char
ask_memorize(ch)
char_int ch;
{
    bool thread_cmd = (ch == 'T');
    bool use_one_line = (gmode == 's');
    bool global_save = FALSE;
    char* mode_string = (thread_cmd? "thread" : "subject");
    char* mode_phrase = (thread_cmd? "replies to this article" :
				     "this subject and all replies");
    ART_NUM art_hold = art;
    ARTICLE* artp_hold = artp;

    if (!use_one_line)
	newline();
reask_memorize:
    sprintf(cmd_buf,"%sMemorize %s command:", global_save?"Global-" : nullstr,
	    mode_string);
    in_char(cmd_buf, 'm', thread_cmd? "+S.mJK,jcC" : "+S.mJK,jcCfg");
#ifdef VERIFY
    printcmd();
#endif
    ch = *buf;
    if (!thread_cmd && ch == 'f') {
	mode_string = *mode_string == 'a'? "subject" : "author";
	erase_line(0);
	goto reask_memorize;
    }
    if (!thread_cmd && ch == 'g') {
	global_save = !global_save;
	erase_line(0);
	goto reask_memorize;
    }
    if (ch == 'h' || ch == 'H') {
	use_one_line = FALSE;
#ifdef VERBOSE
	IF(verbose) {
	    printf("\n\
Type + or SP to auto-select this %s (i.e. includes future articles).\n\
Type S to auto-select the current subject.\n\
Type . to auto-select %s.\n\
Type m to auto-select the current article.\n\
Type J to auto-kill (junk) this %s.\n\
Type K to auto-kill the current subject.\n\
Type , to auto-kill %s.\n\
Type j to auto-kill the current article.\n\
Type C to clear all selection/killing on %s.\n\
Type c to clear all selection/killing on this %s.\n\
Type q to abort the operation.\n\
",mode_string,mode_phrase,mode_string,mode_phrase,mode_phrase,mode_string) FLUSH;
	    if (!thread_cmd) {
		printf("\
Type f to toggle author (from-line) searching.\n\
Type g to toggle global memorization.\n") FLUSH;
		termdown(2);
	    }
	}
	ELSE
#endif
#ifdef TERSE
	{
	    printf("\n\
+ or SP auto-selects this %s.\n\
S auto-selects the subject.\n\
. auto-selects %s.\n\
m auto-selects this article.\n\
J auto-kills this %s.\n\
K auto-kills the subject.\n\
, auto-kills %s.\n\
j auto-kills the current article.\n\
C clears auto-commands for %s.\n\
c clears auto-commands for this %s.\n\
q aborts.\n\
",mode_string,mode_phrase,mode_string,mode_phrase,mode_phrase,mode_string) FLUSH;
	    if (!thread_cmd) {
		printf("\
f toggles author (from) mode.\n\
g toggles global memorization.\n");
		termdown(2);
	    }
	}
#endif
	newline();
	termdown(9);
	goto reask_memorize;
    }
    if (ch == 'q') {
	if (use_one_line)
	    return 'Q';
	newline();
	return 'q';
    }
    if (!thread_cmd) {
	buf[1] = *mode_string == 'a'? 'f' : 's';
	buf[2] = global_save? 'g' : 'l';
    }
    if (ch == '+') {
	if (!thread_cmd) {
	    (void)art_search(buf, (sizeof buf), TRUE);
	    art = art_hold;
	    artp = artp_hold;
	    ch = '.';
	} else {
	    select_arts_thread(artp, AUTO_SEL_THD);
	    ch = (use_one_line? '+' : '.');
	}
	if (gmode != 's') {
	    printf("\nSelection memorized.\n");
	    termdown(2);
	}
    }
    else if (ch == 'S') {
	select_arts_subject(artp, AUTO_SEL_SBJ);
	ch = (use_one_line? '+' : '.');
	if (gmode != 's') {
	    printf("\nSelection memorized.\n");
	    termdown(2);
	}
    }
    else if (ch == '.') {
	if (!thread_cmd) {
	    (void)art_search(buf, (sizeof buf), TRUE);
	    art = art_hold;
	    artp = artp_hold;
	} else {
	    select_subthread(artp, AUTO_SEL_FOL);
	    ch = (use_one_line? '+' : '.');
	}
	if (gmode != 's') {
	    printf("\nSelection memorized.\n");
	    termdown(2);
	}
    }
    else if (ch == 'm') {
	if (artp) {
	    change_auto_flags(artp, AUTO_SEL_1);
	    ch = (use_one_line? '+' : '.');
	    if (gmode != 's') {
		printf("\nSelection memorized.\n");
		termdown(2);
	    }
	}
    }
    else if (ch == 'J') {
	if (!thread_cmd) {
	    *buf = 'K';
	    (void)art_search(buf, (sizeof buf), TRUE);
	    art = art_hold;
	    artp = artp_hold;
	}
	else
	    kill_thread(artp->subj->thread,AFFECT_ALL|AUTO_KILL_THD);
	if (gmode != 's') {
	    printf("\nKill memorized.\n");
	    termdown(2);
	}
    }
    else if (ch == 'j') {
	if (artp) {
	    mark_as_read(artp);
	    change_auto_flags(artp, AUTO_KILL_1);
	    if (gmode != 's') {
		printf("\nKill memorized.\n");
		termdown(2);
	    }
	}
    }
    else if (ch == 'K') {
	kill_subject(artp->subj,AFFECT_ALL|AUTO_KILL_SBJ);
	if (gmode != 's') {
	    printf("\nKill memorized.\n");
	    termdown(2);
	}
    }
    else if (ch == ',') {
	if (!thread_cmd) {
	    (void)art_search(buf, (sizeof buf), TRUE);
	    art = art_hold;
	    artp = artp_hold;
	}
	else
	    kill_subthread(artp,AFFECT_ALL|AUTO_KILL_FOL);
	if (gmode != 's') {
	    printf("\nKill memorized.\n");
	    termdown(2);
	}
    }
    else if (ch == 'C') {
	if (thread_cmd)
	    clear_thread(artp->subj->thread);
	else
	    clear_subject(artp->subj);
    }
    else if (ch == 'c') {
	clear_subthread(artp);
    }
#if 0
    else if (ch == 's') {
	buf[1] = FINISHCMD;
	finish_command(1);
	(void)art_search(buf, (sizeof buf), TRUE);
	art = art_hold;
	artp = artp_hold;
    }
#endif
    else {
	use_one_line = FALSE;
	printf("\n%s\n", hforhelp) FLUSH;
	termdown(3);
	settle_down();
	goto reask_memorize;
    }
    if (!use_one_line)
	newline();
    return ch;
}
