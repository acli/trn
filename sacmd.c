/* This file Copyright 1992 by Clifford A. Adams */
/* sacmd.c
 *
 * main command loop
 */

#include "EXTERN.h"
#include "common.h"
#ifdef SCAN_ART
#include "list.h"
#include "hash.h"
#include "cache.h"
/* absfirst, lastart, oneless_artnum() */
#include "bits.h"
#include "decode.h"
#include "term.h"	/* for LINES */
#include "head.h"
#include "help.h"	/* online help */
#include "ngdata.h"	/* for ThreadedGroup */
#include "ng.h"
#include "nntpclient.h"
#include "datasrc.h"
#include "addng.h"
#include "ngstuff.h"
#include "respond.h"
#include "intrp.h"
#include "scan.h"
#include "scmd.h"
#include "smisc.h"	/* needed? */
#include "sorder.h" 
#include "spage.h"
#include "sdisp.h"
#include "scanart.h"
#include "samain.h"
#include "samisc.h"
#include "sadisp.h"
#include "sadesc.h"
#include "sathread.h"
#ifdef SCORE
#include "score.h"
#endif
#include "util.h"
#include "util2.h"
#include "INTERN.h"
#include "sacmd.h"

bool sa_extract_start();
/* use this command on an extracted file */
/*$$static char* sa_extracted_use = NULL;*/
static char* sa_extract_dest = NULL;
/* junk articles after extracting them */
static bool sa_extract_junk = FALSE;

/* several basic commands are already done by s_docmd (Scan level) */
/* interprets command in buf, returning 0 to continue looping,
 * a condition code (negative #s) or an art# to read.  Also responsible
 * for setting refresh flags if necessary.
 */
int
sa_docmd()
{
    long a;		/* article pointed to */
    long b;		/* misc. artnum */
    int i;		/* for misc. purposes */
    /*$$bool flag;*/		/* misc */
    ART_NUM artnum;

    a = (long)page_ents[s_ptr_page_line].entnum;
    artnum = sa_ents[a].artnum;

    switch(*buf) {
      case '+':	/* enter thread selector */
	if (!ThreadedGroup) {
	    s_beep();
	    return 0;
	}
	buf[0] = '+';	/* fake up command for return */
	buf[1] = '\0';
	sa_art = artnum; /* give it somewhere to point */
	s_save_context();	/* for possible later changes */
	return SA_FAKE;	/* fake up the command. */
#ifdef SCORE
      case 'K':	/* kill below a threshold */
	*buf = ' ';				/* for finish_cmd() */
	if (!s_finish_cmd("Kill below or equal score:"))
	    break;
	/* make **sure** that there is a number here */
	i = atoi(buf+1);
	if (i == 0) {			/* it might not be a number */
	    char* s;
	    s = buf+1;
	    if (*s != '0' && ((*s != '+' && *s != '-') || s[1] != '0')) {
		/* text was not a numeric 0 */
		s_beep();
		return 0;			/* beep but do nothing */
	    }
	}
	sc_kill_threshold(i);
	s_refill = TRUE;
	s_ref_top = TRUE;	/* refresh # of articles */
	break;
#endif /* SCORE */
      case 'D':	/* kill unmarked "on" page */
	for (i = 0; i <= s_bot_ent; i++)
    /* This is a difficult decision, with no obviously good behavior. */
    /* Do not kill threads with the first article marked, as it is probably
     * not what the user wanted.
     */
	    if (!sa_marked(page_ents[i].entnum) || !sa_mode_fold)
		(void)sa_art_cmd(sa_mode_fold,SA_KILL_UNMARKED,
				 page_ents[i].entnum);
/* consider: should it start reading? */
	b = sa_readmarked_elig();
	if (b) {
	    sa_clearmark(b);
	    return b;
	}
	s_ref_top = TRUE;	/* refresh # of articles */
	s_refill = TRUE;
	break;
      case 'J':	/* kill marked "on" page */
	/* If in "fold" mode, kill threads with the first article marked */
	if (sa_mode_fold) {
	    for (i = 0; i <= s_bot_ent; i++) {
		if (sa_marked(page_ents[i].entnum))
		    (void)sa_art_cmd(TRUE,SA_KILL,page_ents[i].entnum);
	    }
	} else
	    for (i = 0; i <= s_bot_ent; i++)
		(void)sa_art_cmd(FALSE,SA_KILL_MARKED,page_ents[i].entnum);
	s_refill = TRUE;
	s_ref_top = TRUE;	/* refresh # of articles */
	break;
      case 'X':	/* kill unmarked (basic-eligible) in group */
	*buf = '?';				/* for finish_cmd() */
	if (!s_finish_cmd("Junk all unmarked articles"))
	    break;
	if ((buf[1] != 'Y') && (buf[1] != 'y'))
	    break;
	i = s_first();
	if (!i)
	    break;
	if (!sa_basic_elig(i))
	    while ((i = s_next(i)) != 0 && !sa_basic_elig(i)) ;
	/* New action: if in fold mode, will not delete an article which
	   has a thread-prior marked article. */
	for ( ; i; i = s_next(i)) {
	    if (!sa_basic_elig(i))
		continue;
	    if (!sa_marked(i) && !was_read(sa_ents[i].artnum)) {
		if (sa_mode_fold) {		/* new semantics */
		    long j;
		    /* make j == 0 if no prior marked articles in the thread. */
		    for (j = i; j; j = sa_subj_thread_prev(j))
			if (sa_marked(j))
			    break;
		    if (j)	/* there was a marked article */
			continue;	/* article selection loop */
		}
		oneless_artnum(sa_ents[i].artnum);
	    }
	}
	b = sa_readmarked_elig();
	if (b) {
	    sa_clearmark(b);
	    return b;
	}
	s_refill = TRUE;
	s_ref_top = TRUE;	/* refresh # of articles */
	break;
      case 'c':	/* catchup */
	s_go_bot();
	ask_catchup();
	s_refill = TRUE;
	s_ref_all = TRUE;
	break;
#ifdef SCORE
      case 'o':	/* toggle between score and arrival orders */
	s_rub_ptr();
	if (sa_mode_order==1)
	    sa_mode_order = 2;
	else
	    sa_mode_order = 1;
	if (sa_mode_order == 2 && sc_delay) {
	    sc_delay = FALSE;
	    sc_init(TRUE);
	}
	if (sa_mode_order == 2 && !sc_initialized) /* score order */
	    sa_mode_order = 1;	/* nope... (maybe allow later) */
	/* if we go into score mode, make sure score is displayed */
	if (sa_mode_order == 2 && !sa_mode_desc_score)
	    sa_mode_desc_score = TRUE;
	s_sort();
	s_go_top_ents();
	s_refill = TRUE;
	s_ref_bot = TRUE;
	break;
      case 'O':	/* change article sorting order */
	if (sa_mode_order != 2) { /* not in score order */
	    s_beep();
	    break;
	}
	score_newfirst = !score_newfirst;
	s_sort();
	s_go_top_ents();
	s_refill = TRUE;
	break;
      case 'R':	/* rescore articles */
	if (!sc_initialized)
	    break;
	/* clear to end of screen */
	clear_rest();
	s_ref_all = TRUE;	/* refresh everything */
	printf("\nRescoring articles...\n") FLUSH;
	sc_rescore();
	s_sort();
	s_go_top_ents();
	s_refill = TRUE;
	eat_typeahead();	/* stay in control. */
	break;
      case Ctl('e'):		/* edit scorefile for group */
	  /* clear to end of screen */
	  clear_rest();
	s_ref_all = TRUE;	/* refresh everything */
	sc_score_cmd("e");	/* edit scorefile */
	eat_typeahead();	/* stay in control. */
	break;
#endif /* SCORE */
      case '\t':	/* TAB: toggle threadcount display */
	sa_mode_desc_threadcount = !sa_mode_desc_threadcount;
	s_ref_desc = 0;
	break;
      case 'a':	/* toggle author display */
	sa_mode_desc_author = !sa_mode_desc_author;
	s_ref_desc = 0;
	break;
      case '%':	/* toggle article # display */
	sa_mode_desc_artnum = !sa_mode_desc_artnum;
	s_ref_desc = 0;
	break;
      case 'U':	/* toggle unread/unread+read mode */
	sa_mode_read_elig = !sa_mode_read_elig;
/* maybe later use the flag to not do this more than once per newsgroup */
	for (i = 1; i < sa_num_ents; i++)
	    s_order_add(i);		/* duplicates ignored */
	if (sa_eligible(s_first()) || s_next_elig(s_first())) {
#ifdef SCORE
#ifdef PENDING
	    if (sa_mode_read_elig) {
		sc_fill_read = TRUE;
		sc_fill_max = absfirst - 1;
	    }
	    if (!sa_mode_read_elig)
		sc_fill_read = FALSE;
#endif
#endif
	    s_ref_top = TRUE;
	    s_rub_ptr();
	    s_go_top_ents();
	    s_refill = TRUE;
	} else	/* quit out: no articles */
	    return SA_QUIT;
	break;
      case 'F':	/* Fold */
	sa_mode_fold = !sa_mode_fold;
	s_refill = TRUE;
	s_ref_top = TRUE;
	break;
      case 'f':	/* follow */
	sa_follow = !sa_follow;
	s_ref_top = TRUE;
	break;
      case 'Z':	/* Zero (wipe) selections... */
	for (i = 1; i < sa_num_ents; i++)
	    sa_ents[i].sa_flags = (sa_ents[i].sa_flags & 0xfd);
	s_ref_status = 0;
	if (!sa_mode_zoom)
	    break;
	s_ref_all = TRUE;	/* otherwise won't be refreshed */
	/* if in zoom mode, turn it off... */
	/* FALL THROUGH */
      case 'z':	/* zoom mode toggle */
	sa_mode_zoom = !sa_mode_zoom;
	if (sa_unzoomrefold && !sa_mode_zoom)
	    sa_mode_fold = TRUE;
	/* toggle mode again if no elibible articles left */
	if (sa_eligible(s_first()) || s_next_elig(s_first())) {
	    s_ref_top = TRUE;
	    s_go_top_ents();
	    s_refill = TRUE;
	} else {
	    s_beep();
	    sa_mode_zoom = !sa_mode_zoom;	/* toggle it right back */
	}
	break;
      case 'j':	/* junk just this article */
	(void)sa_art_cmd(FALSE,SA_KILL,a);
	/* later refill the page */
	s_refill = TRUE;
	s_ref_top = TRUE;	/* refresh # of articles */
	break;
      case 'n':	/* next art */
      case ']':
	s_rub_ptr();
	if (s_ptr_page_line<(s_bot_ent)) /* more on page... */
	    s_ptr_page_line +=1;
	else {
	    if (!s_next_elig(page_ents[s_bot_ent].entnum)) {
		s_beep();
		return 0;
	    }
	    s_go_next_page();	/* will jump to top too... */
	}
	break;
      case 'k':	/* kill subject thread */
      case ',':	/* might later clone TRN ',' */
	(void)sa_art_cmd(TRUE,SA_KILL,a);
	s_refill = TRUE;
	s_ref_top = TRUE;	/* refresh # of articles */
	break;
      case Ctl('n'):	/* follow subject forward */
	  i = sa_subj_thread(a);
	for (b = s_next_elig(a); b; b = s_next_elig(b))
	    if (i == sa_subj_thread(b))
		break;
	if (!b) {	/* no next w/ same subject */
	    s_beep();
	    return 0;
	}
	for (i = s_ptr_page_line+1; i <= s_bot_ent; i++)
	    if (page_ents[i].entnum == b) {	/* art is on same page */
		s_rub_ptr();
		s_ptr_page_line = i;
		return 0;
	    }
	/* article is not on page... */
	(void)s_fillpage_forward(b);  /* fill forward (*must* work) */
	s_ptr_page_line = 0;
	break;
      case Ctl('a'):	/* next article with same author... */
	/* good for scoring */
	b = sa_wrap_next_author(a);
	/* rest of code copied from next-subject case above */
	if (!b || (a == b)) {	/* no next w/ same subject */
	    s_beep();
	    return 0;
	}
	for (i = 0; i <= s_bot_ent; i++)
	    if (page_ents[i].entnum == b) {	/* art is on same page */
		s_rub_ptr();
		s_ptr_page_line = i;
		return 0;
	    }
	/* article is not on page... */
	(void)s_fillpage_forward(b);  /* fill forward (*must* work) */
	s_ptr_page_line = 0;
	break;
      case Ctl('p'):	/* follow subject backwards */
	  i = sa_subj_thread(a);
	for (b = s_prev_elig(a); b; b = s_prev_elig(b))
	    if (i == sa_subj_thread(b))
		break;
	if (!b) {	/* no next w/ same subject */
	    s_beep();
	    return 0;
	}
	for (i = s_ptr_page_line-1; i >= 0; i--)
	    if (page_ents[i].entnum == b) {	/* art is on same page */
		s_rub_ptr();
		s_ptr_page_line = i;
		return 0;
	    }
	/* article is not on page... */
	(void)s_fillpage_backward(b);  /* fill backward (*must* work) */
	s_ptr_page_line = s_bot_ent;  /* go to bottom of page */
	(void)s_refillpage();	/* make sure page is full */
	s_ref_all = TRUE;		/* make everything redrawn... */
	break;
      case 'G': /* go to article number */
	*buf = ' ';				/* for finish_cmd() */
	if (!s_finish_cmd("Goto article:"))
	    break;
	/* make **sure** that there is a number here */
	i = atoi(buf+1);
	if (i == 0) {			/* it might not be a number */
	    char* s;
	    s = buf+1;
	    if (*s != '0' && ((*s != '+' && *s != '-') || s[1] != '0')) {
		/* text was not a numeric 0 */
		s_beep();
		return 0;			/* beep but do nothing */
	    }
	}
	sa_art = (ART_NUM)i;
	return SA_READ;			/* special code to really return */
      case 'N':	/* next newsgroup */
	return SA_NEXT;
      case 'P':	/* previous newsgroup */
	return SA_PRIOR;
      case '`':
	return SA_QUIT_SEL;
      case 'q':	/* quit newsgroup */
	return SA_QUIT;
      case 'Q':	/* start reading and quit SA mode */
	sa_in = FALSE;
	/* FALL THROUGH */
      case '\n':
      case ' ':
	b = sa_readmarked_elig();
	if (b) {
	    sa_clearmark(b);
	    return b;
	}
	/* FALL THROUGH */
      case 'r':	/* read article... */
	/* everything needs to be refreshed... */
	s_ref_all = TRUE;
	return a;
      case 'm':	/* toggle mark on one article */
      case 'M':	/* toggle mark on thread */
	s_rub_ptr();
	(void)sa_art_cmd((*buf == 'M'),SA_MARK,a);
	if (!sa_mark_stay) {
	    /* go to next art on page or top of page if at bottom */
	    if (s_ptr_page_line < s_bot_ent)	/* more on page */
		s_ptr_page_line +=1;
	    else
		s_go_top_page();	/* wrap around to top */
	}
	break;
      case 's':	/* toggle select1 on one article */
      case 'S':	/* toggle select1 on a thread */
	if (!sa_mode_zoom)
	    s_rub_ptr();
	(void)sa_art_cmd((*buf == 'S'),SA_SELECT,a);
	/* if in zoom mode, selection will remove article(s) from the
	 * page, so that moving the cursor down is unnecessary
	 */
	if (!sa_mark_stay && !sa_mode_zoom) {
	    /* go to next art on page or top of page if at bottom */
	    if (s_ptr_page_line<(s_bot_ent))	/* more on page */
		s_ptr_page_line +=1;
	    else
		s_go_top_page();	/* wrap around to top */
	}
	break;
      case 'e':	/* extract marked articles */
#if 0
	if (!sa_extract_start())
	    break;		/* aborted */
	if (!decode_fp)
	    *decode_dest = '\0';	/* wipe old name */
	a = s_first();
	if (!s_eligible(a))
	    a = s_next_elig(a);
	flag = FALSE;		/* have we found a marked one? */
	for ( ; a; a = s_next_elig(a))
	    if (sa_marked(a)) {
		flag = TRUE;
		(void)sa_art_cmd(FALSE,SA_EXTRACT,a);
	    }
	if (!flag) {			/* none were marked */
	    a = page_ents[s_ptr_page_line].entnum;
	    (void)sa_art_cmd(FALSE,SA_EXTRACT,a);
	}
	s_refill = TRUE;
	s_ref_top = TRUE;	/* refresh # of articles */
	(void)get_anything();
	eat_typeahead();
#endif
	break;
#if 0
      case 'E':	/* end extraction, do command on image */
	s_ref_all = TRUE;
	s_go_bot();
	if (decode_fp) {
	    printf("\nIncomplete file: %s\n",decode_dest) FLUSH;
	    printf("Continue with command? [ny]");
	    fflush(stdout);
	    getcmd(buf);
	    printf("\n") FLUSH;
	    if (*buf == 'n' || *buf == ' ' || *buf == '\n')
		break;
		printf("Remove this file? [ny]");
	    fflush(stdout);
	    getcmd(buf);
	    printf("\n") FLUSH;
	    if (*buf == 'y' || *buf == 'Y') {
		decode_end();	/* will remove file */
		break;
	    }
	    fclose(decode_fp);
	    decode_fp = 0;
	}
	if (!sa_extracted_use) {
	    sa_extracted_use = safemalloc(LBUFLEN);
/* later consider a variable for the default command */
	    *sa_extracted_use = '\0';
	}
	if (!*decode_dest) {
	    printf("\nTrn doesn't remember an extracted file name.\n") FLUSH;
	    *buf = ' ';
	    if (!s_finish_cmd("Please enter a file to use:"))
		break;
	    if (!buf[1])	/* user just typed return */
		break;
	    safecpy(decode_dest,buf+1,MAXFILENAME);
	    printf("\n") FLUSH;
	}
	if (sa_extract_dest == NULL) {
	    sa_extract_dest = (char*)safemalloc(LBUFLEN);
	    safecpy(sa_extract_dest,filexp("%p"),LBUFLEN);
	}
	if (*decode_dest != '/' && *decode_dest != '~'
	 && *decode_dest != '%') {
	    sprintf(buf,"%s/%s",sa_extract_dest,decode_dest);
	    safecpy(decode_dest,buf,MAXFILENAME);
	}
	if (*sa_extracted_use)
	    printf("Use command (default %s):\n",sa_extracted_use) FLUSH;
	else
	    printf("Use command (no default):\n") FLUSH;
	*buf = ':';			/* cosmetic */
	if (!s_finish_cmd(NULL))
	    break;	/* command rubbed out */
	if (buf[1] != '\0')		/* typed in a command */
	    safecpy(sa_extracted_use,buf+1,LBUFLEN);
	if (*sa_extracted_use == '\0')	/* no command */
	    break;
	sprintf(buf,"!%s %s",sa_extracted_use,decode_dest);
	printf("\n%s\n",buf+1) FLUSH;
	(void)escapade();
	(void)get_anything();
	eat_typeahead();
	break;
#endif
#ifdef SCORE
      case '"':			/* append to local SCORE file */
	s_go_bot();
	s_ref_all = TRUE;
	printf("Enter score append command or type RETURN for a menu\n");
	buf[0] = ':';
	buf[1] = FINISHCMD;
	if (!finish_command(FALSE))
	    break;
	printf("\n") FLUSH;
	sa_go_art(artnum);
	sc_append(buf+1);
	(void)get_anything();
	eat_typeahead();
	break;
      case '\'':			/* execute scoring command */
	s_go_bot();
	s_ref_all = TRUE;
	printf("\nEnter scoring command or type RETURN for a menu\n");
	buf[0] = ':';
	buf[1] = FINISHCMD;
	if (!finish_command(FALSE))
	    break;
	printf("\n") FLUSH;
	sa_go_art(artnum);
	sc_score_cmd(buf+1);
	s_ref_all = TRUE;
	(void)get_anything();
	eat_typeahead();
	break;
#endif
      default:
	s_beep();
	return 0;
    } /* switch */
    return 0;
}

bool
sa_extract_start()
{
    if (sa_extract_dest == NULL) {
	sa_extract_dest = (char*)safemalloc(LBUFLEN);
	safecpy(sa_extract_dest,filexp("%p"),LBUFLEN);
    }
    s_go_bot();
    printf("To directory (default %s)\n",sa_extract_dest) FLUSH;
    *buf = ':';			/* cosmetic */
    if (!s_finish_cmd(NULL))
	return FALSE;		/* command rubbed out */
    s_ref_all = TRUE;
    /* if the user typed something, copy it to the destination */
    if (buf[1] != '\0')
	safecpy(sa_extract_dest,filexp(buf+1),LBUFLEN);
/* set a mode for this later? */
    printf("\nMark extracted articles as read? [yn]");
    fflush(stdout);
    getcmd(buf);
    printf("\n") FLUSH;
    if (*buf == 'y' || *buf == ' ' || *buf == '\n')
	sa_extract_junk = TRUE;
    else
	sa_extract_junk = FALSE;
    return TRUE;
}


/* sa_art_cmd primitive: actually does work on an article */
void
sa_art_cmd_prim(cmd,a)
int cmd;
long a;
{
    ART_NUM artnum;

    artnum = sa_ents[a].artnum;
/* do more onpage status refreshes when in unread+read mode? */
    switch (cmd) {
      case SA_KILL_MARKED:
	if (sa_marked(a)) {
	    sa_clearmark(a);
	    oneless_artnum(artnum);
	}
	break;
      case SA_KILL_UNMARKED:
	if (sa_marked(a))
	    break;		/* end case early */
	oneless_artnum(artnum);
	break;
      case SA_KILL:		/* junk this article */
	sa_clearmark(a);	/* clearing should be fast */
	oneless_artnum(artnum);
	break;
      case SA_MARK:		/* mark this article */
	if (sa_marked(a))
	    sa_clearmark(a);
	else
	    sa_mark(a);
	s_ref_status_onpage(a);
	break;
      case SA_SELECT:		/* select this article */
	if (sa_selected1(a)) {
	    sa_clearselect1(a);
	    if (sa_mode_zoom)
		s_refill = TRUE;	/* article is now ineligible */
	} else
	    sa_select1(a);
	s_ref_status_onpage(a);
	break;
      case SA_EXTRACT:
	sa_clearmark(a);
	art = artnum;
	*buf = 'e';		/* fake up the extract command */
	safecpy(buf+1,sa_extract_dest,LBUFLEN);
	(void)save_article();
	if (sa_extract_junk)
	    oneless_artnum(artnum);
	break;
    } /* switch */
}

/* return value is unused for now, but may be later... */
/* note: refilling after a kill is the caller's responsibility */
int
sa_art_cmd(multiple,cmd,a)
int multiple;		/* follow the thread? */
int cmd;		/* what to do */
long a;		/* article # to affect or start with */
{
    long b;

    sa_art_cmd_prim(cmd,a);	/* do the first article */
    if (!multiple)
	return 0;		/* no problem... */
    b = a;
    while ((b = sa_subj_thread_next(b)) != 0)
	/* if this is basically eligible and the same subject thread# */
	sa_art_cmd_prim(cmd,b);
    return 0;
}

/* XXX this needs a good long thinking session before re-activating */
long
sa_wrap_next_author(a)
long a;
{
#ifdef UNDEF
    long b;
    char* s;
    char* s2;
    
    s = (char*)sa_desc_author(a,20);	/* 20 characters should be enough */
    for (b = s_next_elig(a); b; b = s_next_elig(b))
	if (STRSTR(get_from_line(b),s))
	    break;	/* out of the for loop */
    if (b)	/* found it */
	return b;
    /* search from first article (maybe return original art) */
    b = s_first();
    if (!sa_eligible(b))
	b = s_next_elig(b);
    for ( ; b; b = s_next_elig(b))
	if (STRSTR(get_from_line(b),s))
	    break;	/* out of the for loop */
    return b;
#else
    return a;		/* feature is disabled */
#endif
}
#endif /* SCAN */
