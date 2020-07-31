/* This file is Copyright 1993 by Clifford A. Adams */
/* scmd.c
 *
 * Scan command loop.
 * Does some simple commands, and passes the rest to context-specific routines.
 */

#include "EXTERN.h"
#include "common.h"
#ifdef SCAN
#include "hash.h"
#include "cache.h"
#include "bits.h"
#include "final.h"
#include "help.h"
#include "ng.h"
#include "nntpclient.h"
#include "datasrc.h"
#include "addng.h"
#include "ngstuff.h"
#include "term.h"
#include "util.h"
#include "scan.h"
#include "smisc.h"
#include "sorder.h"
#include "spage.h"
#include "sdisp.h"
#include "sacmd.h"	/* sa_docmd */
#include "samain.h"
#ifdef SCORE
#include "score.h"
#endif
#include "univ.h"
#include "INTERN.h"
#include "scmd.h"

void s_search();

void
s_go_bot()
{
    s_ref_bot = TRUE;			/* help uses whole screen */
    s_goxy(0,tc_LINES-s_bot_lines);	/* go to bottom bar */
    erase_eol();			/* erase to end of line */
    s_goxy(0,tc_LINES-s_bot_lines);	/* go (back?) to bottom bar */
}

/* finishes a command on the bottom line... */
/* returns TRUE if command entered, FALSE if wiped out... */
int
s_finish_cmd(string)
char* string;
{
    s_go_bot();
    if (string && *string) {
	printf("%s",string);
	fflush(stdout);
    }
    buf[1] = FINISHCMD;
    return finish_command(FALSE);	/* do not echo newline */
}

/* returns an entry # selected, S_QUIT, or S_ERR */
int
s_cmdloop()
{
    int i;

    /* initialization stuff for entry into s_cmdloop */
    s_ref_all = TRUE;
    eat_typeahead();	/* no typeahead before entry */
    while(TRUE) {
	s_refresh();
	s_place_ptr();		/* place article pointer */
	bos_on_stop = TRUE;
	s_lookahead();		/* do something useful while waiting */
	getcmd(buf);
	bos_on_stop = FALSE;
	eat_typeahead();	/* stay in control. */
	/* check for window resizing and refresh */
	/* if window is resized, refill and redraw */
	if (s_resized) {
	    char ch = *buf;
	    i = s_fillpage();
	    if (i == -1 || i == 0)	/* can't fillpage */
	        return S_QUIT;
	    *buf = Ctl('l');
	    (void)s_docmd();
	    *buf = ch;
	    s_resized = FALSE;		/* dealt with */
	}
	i = s_docmd();
	if (i == S_NOTFOUND) {	/* command not in common set */
	    switch (s_cur_type) {
#ifdef SCAN_ART
	      case S_ART:
		i = sa_docmd();
		break;
#endif
	      default:
		i = 0;	/* just keep looping */
		break;
	    }
	}
	if (i != 0)	/* either an entry # or a return code */
	    return i;
	if (s_refill) {
	    i = s_fillpage();
	    if (i == -1 || i == 0)	/* can't fillpage */
	        return S_QUIT;
        }
	/* otherwise just keep on looping... */
    }
}

void
s_lookahead()
{
    switch (s_cur_type) {
#ifdef SCAN_ART
      case S_ART:
	sa_lookahead();
	break;
#endif
      default:
	break;
    }
}

/* Do some simple, common Scan commands for any mode */
/* Interprets command in buf, returning 0 to continue looping or
 * a condition code (negative #s).  Responsible for setting refresh flags
 * if necessary.
 */
int
s_docmd()
{
    long a;		/* entry pointed to */
    bool flag;		/* misc */

    a = page_ents[s_ptr_page_line].entnum;
    if (*buf == '\f')	/* map form feed to ^l */
	*buf = Ctl('l');
    switch(*buf) {
      case 'j':		/* vi mode */
	if (!s_mode_vi)
	    return S_NOTFOUND;
	/* FALL THROUGH */
      case 'n':		/* next art */
      case ']':
	s_rub_ptr();
	if (s_ptr_page_line < s_bot_ent)	/* more on page... */
	    s_ptr_page_line +=1;
	else {
	    if (!s_next_elig(page_ents[s_bot_ent].entnum)) {
		s_beep();
		s_refill = TRUE;
		break;
	    }
	    s_go_next_page();	/* will jump to top too... */
	}
	break;
      case 'k':	/* vi mode */
	if (!s_mode_vi)
	    return S_NOTFOUND;
	/* FALL THROUGH */
      case 'p':	/* previous art */
      case '[':
	s_rub_ptr();
	if (s_ptr_page_line > 0)	/* more on page... */
	    s_ptr_page_line = s_ptr_page_line - 1;
	else {
	    if (s_prev_elig(page_ents[0].entnum)) {
		s_go_prev_page();
		s_ptr_page_line = s_bot_ent; /* go to page bot. */
	    } else {
		s_refill = TRUE;
		s_beep();
	    }
	}
	break;
      case 't':	/* top of page */
	s_rub_ptr();
	s_go_top_page();
	break;
      case 'b':	/* bottom of page */
	s_rub_ptr();
	s_go_bot_page();
	break;
      case '>':	/* next page */
	s_rub_ptr();
	a = s_next_elig(page_ents[s_bot_ent].entnum);
	if (!a) {		/* at end of articles */
	    s_beep();
	    break;
	}
	s_go_next_page();		/* will beep if no next page */
	break;
      case '<':	/* previous page */
	s_rub_ptr();
	if (!s_prev_elig(page_ents[0].entnum)) {
	    s_beep();
	    break;
	}
	s_go_prev_page();		/* will beep if no prior page */
	break;
      case 'T':		/* top of ents */
      case '^':
	s_rub_ptr();
	flag = s_go_top_ents();
	if (!flag)		/* failure */
	    return S_QUIT;
	break;
      case 'B':	/* bottom of ents */
      case '$':
	s_rub_ptr();
	flag = s_go_bot_ents();
	if (!flag)
	    return S_QUIT;
	break;
      case Ctl('r'):	/* refresh screen */
      case Ctl('l'):
	  s_ref_all = TRUE;
	break;
      case Ctl('f'):	/* refresh (mail) display */
#ifdef MAILCALL
	setmail(TRUE);
#endif
	s_ref_bot = TRUE;
	break;
      case 'h': /* universal help */
	s_go_bot();
	s_ref_all = TRUE;
	univ_help(UHELP_SCANART);
	eat_typeahead();
	break;
      case 'H':	/* help */
	s_go_bot();
	s_ref_all = TRUE;
	/* any commands typed during help are unused. (might change) */
	switch (s_cur_type) {
#ifdef SCAN_ART
	  case S_ART:
	    (void)help_scanart();
	    break;
#endif
	  default:
	    printf("No help available for this mode (yet).\n") FLUSH;
	    printf("Press any key to continue.\n");
	    break;
	}
	(void)get_anything();
	eat_typeahead();
	break;
      case '!':	/* shell command */
	s_go_bot();
	s_ref_all = TRUE;			/* will need refresh */
	if (!escapade())
	    (void)get_anything();
	eat_typeahead();
	break;
#if 0
      case '&':		/* see/set switches... */
/* CAA 05/29/95: The new option stuff makes this potentially recursive.
 * Something similar to the 'H' (extended help) code needs to be done.
 * It may be necessary for this code to do the context saving.
 */
	s_go_bot();
	s_ref_all = TRUE;			/* will need refresh */
	if (!switcheroo())		/* XXX same semantics in trn4? */
	    (void)get_anything();
	eat_typeahead();
	break;
#endif
      case '/':
      case '?':
      case 'g':		/* goto (search for) group */
	s_search();
	break;
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
	s_jumpnum(*buf);
	break;
      case '#':		/* Toggle item numbers */
	if (s_itemnum) {
	    /* turn off item numbers */
	    s_desc_cols += s_itemnum_cols;
	    s_itemnum_cols = 0;
	    s_itemnum = 0;
	} else {
	    /* turn on item numbers */
	    s_itemnum_cols = 3;
	    s_desc_cols -= s_itemnum_cols;
	    s_itemnum = 1;
	}
	s_ref_all = TRUE;
	break;
      default:
	return S_NOTFOUND;		/* not one of the simple commands */
    } /* switch */
    return 0;		/* keep on looping! */
}

static char search_text[LBUFLEN];

static char search_init INIT(FALSE);

bool
s_match_description(ent)
long ent;
{
    int i, lines;
    static char lbuf[LBUFLEN];
    char* s;

    lines = s_ent_lines(ent);
    for (i = 1; i <= lines; i++) {
	strncpy(lbuf,s_get_desc(ent,i,FALSE),LBUFLEN);
	for (s = lbuf; *s; s++)
	    if (isupper(*s))
		*s = tolower(*s);		/* convert to lower case */
	if (STRSTR(lbuf,search_text))
	    return TRUE;
    }
    return FALSE;
}

long
s_forward_search(ent)
long ent;
{
    if (ent)
	ent = s_next_elig(ent);
    else
	ent = s_first();
    for ( ; ent; ent = s_next_elig(ent))
	if (s_match_description(ent))
	    break;
    return ent;
}

long
s_backward_search(ent)
long ent;
{
    if (ent)
	ent = s_prev_elig(ent);
    else
	ent = s_last();
    for ( ; ent; ent = s_prev_elig(ent))
	if (s_match_description(ent))
	    break;
    return ent;
}

/* perhaps later have a wraparound search? */
void
s_search()
{
    int i;
    int fill_type;    /* 0: forward, 1: backward */
    long ent;
    char* s;
    char* error_msg;

    if (!search_init) {
	search_init = TRUE;
	search_text[0] = '\0';
    }
    s_rub_ptr();
    buf[1] = '\0';
    if (!s_finish_cmd(NULL))
	return;
    if (buf[1]) {	/* new text */
	s = buf+1;
	/* make leading space skip an option later? */
	/* (it isn't too important because substring matching is used) */
	while (*s == ' ') s++;	/* skip leading spaces */
	strncpy(search_text,s,LBUFLEN);
	for (s = search_text; *s != '\0'; s++)
	    if (isupper(*s))
		*s = tolower(*s);		/* convert to lower case */
    }
    if (!*search_text) {
	s_beep();
	printf("\nNo previous search string.\n") FLUSH;
	(void)get_anything();
	s_ref_all = TRUE;
	return;
    }
    s_go_bot();
    printf("Searching for %s",search_text);
    fflush(stdout);
    ent = page_ents[s_ptr_page_line].entnum;
    switch (*buf) {
      case '/':
	error_msg = "No matches forward from current point.";
	ent = s_forward_search(ent);
	fill_type = 0;		/* forwards fill */
	break;
      case '?':
	error_msg = "No matches backward from current point.";
	ent = s_backward_search(ent);
	fill_type = 1;		/* backwards fill */
	break;
      case 'g':
	ent = s_forward_search(ent);
	if (!ent) {
	    ent = s_forward_search(0);	/* from top */
	    /* did we just loop around? */
	    if (ent == page_ents[s_ptr_page_line].entnum) {
		ent = 0;
		error_msg = "No other entry matches.";
	    } else
		error_msg = "No matches.";
	}
	fill_type = 0;		/* forwards fill */
	break;
      default:
	fill_type = 0;
	error_msg = "Internal error in s_search()";
	break;
    }
    if (!ent) {
	s_beep();
	printf("\n%s\n",error_msg) FLUSH;
	(void)get_anything();
	s_ref_all = TRUE;
	return;
    }
    for (i = 0; i <= s_bot_ent; i++)
	if (page_ents[i].entnum == ent) {	/* entry is on same page */
	    s_ptr_page_line = i;
	    return;
	}
    /* entry is not on page... */
    if (fill_type == 1) {
	(void)s_fillpage_backward(ent);
	s_go_bot_page();
	s_refill = TRUE;
	s_ref_all = TRUE;
    }
    else {
	(void)s_fillpage_forward(ent);
	s_go_top_page();
	s_ref_all = TRUE;
    }
}

void
s_jumpnum(firstchar)
char_int firstchar;
{
    int value;
    bool jump_verbose;

    jump_verbose = TRUE;
    value = firstchar - '0';

    s_rub_ptr();
#ifdef NICEBG
    wait_key_pause(10);
    if (input_pending())
	jump_verbose = FALSE;
#endif
    if (jump_verbose) {
	s_go_bot();
	s_ref_bot = TRUE;
	printf("Jump to item: %c",firstchar);
	fflush(stdout);
    }
    getcmd(buf);
    if (*buf == ERASECH)
	return;
    switch (*buf) {
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
	if (jump_verbose) {
	    printf("%c",*buf);
	    fflush(stdout);
	}
	value = value*10 + (*buf - '0');
	break;
      default:
	pushchar(*buf);
	break;
    }
    if (value == 0 || value > s_bot_ent+1) {
	s_beep();
	return;
    }
    s_ptr_page_line = value-1;
}
#endif /* SCAN */
