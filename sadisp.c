/* This file Copyright 1992 by Clifford A. Adams */
/* sadisp.c
 *
 * display stuff
 */

#include "EXTERN.h"
#include "common.h"
#ifdef SCAN_ART
#include "hash.h"
#include "cache.h"
/* for absfirst declaration */
#include "bits.h"
/* for mailcall */
#include "ng.h"
/* for ngname */
#include "trn.h"
#include "term.h"
#include "util.h"
#include "scanart.h"
#include "samain.h"
#include "sadesc.h"
#include "samisc.h"
#include "sathread.h"	/* for fold mode flag */
#ifdef SCORE
#include "score.h"
#endif
#include "scan.h"
#include "sdisp.h"
#include "color.h"
#include "INTERN.h"
#include "sadisp.h"

void
sa_refresh_top()
{
    color_object(COLOR_SCORE, 1);
    printf("%s |",ngname);
/* # of articles might be optional later */
    printf(" %d",sa_number_arts());

    if (sa_mode_read_elig)
	printf(" unread+read");
    else
	printf(" unread");
    if (sa_mode_zoom)
	printf(" zoom");
    if (sa_mode_fold)
	printf(" Fold");
    if (sa_follow)
	printf(" follow");
    color_pop();	/* of COLOR_SCORE */
    erase_eol();
    printf("\n") FLUSH;
}

void
sa_refresh_bot()
{
    char* s;

    color_object(COLOR_SCORE, 1);
    s_mail_and_place();
    printf("(");
    switch (sa_mode_order) {
      case 1:
	s = "arrival";
	break;
#ifdef SCORE
      case 2:
	if (score_newfirst)
	    s = "score (new>old)";
	else
	    s = "score (old>new)";
	break;
#endif
      default:
	s = "unknown";
	break;
    }
    printf("%s order",s);
#ifdef SCORE
    printf(", %d%% scored",sc_percent_scored());
#endif
    printf(")");
    color_pop();	/* of COLOR_SCORE */
    fflush(stdout);
}

/* set up various screen dimensions */
void
sa_set_screen()
{
    /* One size fits all for now. */
    /* these things here because they may vary by screen size later */
    s_top_lines = 1;
    s_bot_lines = 1;
    s_status_cols = 3;
    s_cursor_cols = 2;

    if (s_itemnum)
	s_itemnum_cols = 3;
    else
	s_itemnum_cols = 0;

    /* (scr_width-1) keeps last character blank. */
    s_desc_cols = (scr_width-1) -s_status_cols -s_cursor_cols -s_itemnum_cols;
}
#endif /* SCAN */
