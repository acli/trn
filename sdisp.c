/* This file Copyright 1992 by Clifford A. Adams */
/* sdisp.c
 *
 * display stuff
 */

#include "EXTERN.h"
#include "common.h"
#ifdef SCAN
#include "final.h"	/* assert() */
#include "hash.h"
#include "cache.h"
#include "ng.h"		/* mailcall */
#include "term.h"
#include "scan.h"
#include "sorder.h"
#include "smisc.h"
#ifdef SCAN_ART
#include "scanart.h"
#include "samain.h"
#include "sadisp.h"
#endif
#include "INTERN.h"
#include "sdisp.h"

void
s_goxy(x,y)
int x,y;
{
    char* tgoto();
    tputs(tgoto(tc_CM, x, y), 1, putchr);
}

/* Print a string with the placing of the page and mail status.
 * sample: "(mail)-MIDDLE-"
 * Good for most bottom status bars.
 */
void
s_mail_and_place()
{
    bool previous,next;

#ifdef MAILCALL
    setmail(FALSE);		/* another chance to check mail */
    printf("%s",mailcall);
#endif /* MAILCALL */
    /* print page status wrt all entries */
    previous = (0 != s_prev_elig(page_ents[0].entnum));
    next = (0 != s_next_elig(page_ents[s_bot_ent].entnum));
    if (previous && next)
	printf("-MIDDLE-");		/* middle of entries */
    else if (next && !previous)
	printf("-TOP-");
    else if (previous && !next)
	printf("-BOTTOM-");
    else	/* !previous && !next */
	printf("-ALL-");
}

void
s_refresh_top()
{
    home_cursor();
    switch (s_cur_type) {
#ifdef SCAN_ART
      case S_ART:
	sa_refresh_top();
	break;
#endif
    }
    s_ref_top = FALSE;
}

void
s_refresh_bot()
{
    /* if bottom bar exists, then it is at least one character high... */
    s_goxy(0,tc_LINES-s_bot_lines);
    switch (s_cur_type) {
#ifdef SCAN_ART
      case S_ART:
	sa_refresh_bot();
	break;
#endif
    }
    s_ref_bot = FALSE;
}

/* refresh both status and description */
void
s_refresh_entzone()
{
    int i;
    int start;		/* starting page_arts index to refresh... */

    if (s_ref_status < s_ref_desc) {
	/* refresh status characters up to (not including) desc_line */
	for (i = s_ref_status; i <= s_bot_ent && i < s_ref_desc; i++)
	    s_refresh_description(i);
	start = i;
    } else {
	for (i = s_ref_desc; i <= s_bot_ent && i < s_ref_status; i++)
	    s_refresh_status(i);
	start = i;
    }
    for (i = start; i <= s_bot_ent; i++)
	s_ref_entry(i,i==start);
    /* clear to end of screen */
    clear_rest();
    /* now we need to redraw the bottom status line */
    s_ref_bot = TRUE;
    s_ref_status = s_ref_desc = -1;
}

void
s_place_ptr()
{
    s_goxy(s_status_cols,
	    s_top_lines+page_ents[s_ptr_page_line].start_line);
    putchar('>');
    fflush(stdout);
}

/* refresh the status line for an article on screen page */
/* note: descriptions will not (for now) be individually refreshable */
void
s_refresh_status(line)
int line;
{
    int i,j;
    long ent;

    ent = page_ents[line].entnum;
    assert(line <= s_bot_ent);	/* better be refreshing on-page */
    s_goxy(0,s_top_lines+page_ents[line].start_line);
    j = page_ents[line].lines;
    for (i = 1; i <= j; i++)
	printf("%s\n",s_get_statchars(ent,i));
    fflush(stdout);
}

void
s_refresh_description(line)
int line;
{
    int i,j,startline;
    long ent;

    ent = page_ents[line].entnum;
    assert(line <= s_bot_ent);	/* better be refreshing on-page */
    startline = s_top_lines+page_ents[line].start_line;
    j = page_ents[line].lines;
    for (i = 1; i <= j; i++) {
	s_goxy(s_status_cols+s_cursor_cols,(i-1)+startline);
	/* allow flexible format later? */
	if (s_itemnum_cols) {
	    if (i == 1) {	/* first description line */
		if (line < 99)
		    printf("%2d ",line+1);
		else
		    printf("** ");	/* too big */
	    } else
		printf("   ");
	}
	printf("%s",s_get_desc(ent,i,TRUE));
	erase_eol();
	putchar('\n');
    }
    fflush(stdout);
}

void
s_ref_entry(line,jump)
int line;
int jump;	/* true means that the cursor should be positioned */
{
    int i,j;
    long ent;

    ent = page_ents[line].entnum;
    assert(line <= s_bot_ent);	/* better be refreshing on-page */
    if (jump)
	s_goxy(0,s_top_lines+page_ents[line].start_line);
    j = page_ents[line].lines;
    for (i = 1; i <= j; i++) {
/* later replace middle with variable #spaces routine */
	printf("%s%s",s_get_statchars(ent,i),"  ");
	if (s_itemnum_cols) {
	    if (i == 1) {	/* first description line */
		if (line < 99)
		    printf("%2d ",line+1);
		else
		    printf("** ");	/* too big */
	    } else
		printf("   ");
	}
	printf("%s",s_get_desc(ent,i,TRUE));
	erase_eol();
	putchar('\n');
    }
}

void
s_rub_ptr()
{
    rubout();
}

void
s_refresh()
{
    int i;

    if (s_ref_all) {
	clear();	/* make a clean slate */
	s_ref_desc = s_ref_status = 0;
    }
    if ((s_ref_all || s_ref_top) && s_top_lines>0)
	s_refresh_top();
    if (s_ref_all || ((s_ref_status>=0) && (s_ref_desc>=0)))
	s_refresh_entzone();
    else {
	if (s_ref_status>=0) {
	    for (i = s_ref_status; i <= s_bot_ent; i++)
		s_refresh_status(i);
	}
	if (s_ref_desc >= 0) {
	    for (i = s_ref_desc; i <= s_bot_ent; i++)
		s_refresh_description(i);
	}
    }
    s_ref_status = s_ref_desc = -1;
    if ((s_ref_all || s_ref_bot) && s_bot_lines > 0)
	s_refresh_bot();
    s_ref_all = FALSE;
}

int
s_initscreen()
{
    /* check to see if term is too dumb: if so, return non-zero */

    /* set scr_{height,width} */
    /* return 0 if all went well */

    scr_height = tc_LINES;
    scr_width = tc_COLS;
    if (scr_height > 2 && scr_width > 1)	/* current dependencies */
	return 0;	/* everything is OK. */
    return 1;	/* we can't play with this... */
}

/* screen-refresh the status if on-page */
void
s_ref_status_onpage(ent)
long ent;
{
    int i;
    for (i = 0; i <= s_bot_ent; i++)
	if (page_ents[i].entnum == ent)
	    s_refresh_status(i);
}


void
s_resize_win()
{
#if 0
    int i;

    i = s_initscreen();
    /* later possibly use the return value for an error abort? */
    s_resized = TRUE;
#endif
    ;	/* don't have an empty function */
}
#endif /* SCAN */
