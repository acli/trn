/* This file Copyright 1992 by Clifford A. Adams */
/* score.c
 *
 */

#include "EXTERN.h"
#include "common.h"
#ifdef SCORE
/* if SCORE is undefined, no code should be compiled */
/* sort the following includes later */
#include "list.h"
#include "hash.h"
#include "cache.h"
#include "bits.h"
#include "artio.h"		/* for openart var.*/
#include "final.h"		/* int_count */
#include "head.h"		/* ? */
#include "intrp.h"		/* for filexp */
#include "ng.h"			/* art */
#include "ngdata.h"
#include "search.h"		/* for regex */
#include "rt-util.h"		/* spinner */
#include "term.h"		/* input_pending() */
#include "trn.h"		/* ngname */
#include "util.h"		/* several */
#ifdef SCAN
#include "scan.h"
#include "sorder.h"
#include "scanart.h"
#ifdef SCAN_ART
#include "samain.h"
#include "samisc.h"
#endif
#endif
#include "INTERN.h"
#include "score.h"
#include "EXTERN.h"
#include "scorefile.h"
#include "scoresave.h"
#include "score-easy.h"		/* interactive menus and such */
#ifdef USE_FILTER
#include "filter.h"
#endif
#include "INTERN.h"		/* not currently needed, but safer */

#if defined(USE_FILTER) && defined(FILTER_DEBUG)
FILE* filter_error_file;
#endif

void
sc_init(pend_wait)
bool_int pend_wait;	/* if true, enter pending mode when scoring... */
{
    int i;
    ART_NUM a;

    if (lastart == 0 || lastart < absfirst) {
#if 0
	printf("No articles exist to be scored.\n") FLUSH;
#endif
	return;
    }
    sc_sf_force_init = TRUE;		/* generally force initialization */
    if (sc_delay)			/* requested delay? */
	return;
    sc_sf_delay = FALSE;

/* Consider the relationships between scoring and article scan mode.
 * Should one be able to initialize the other?  How much can they depend on
 * each other when both are #defined?
 * Consider this especially in a later redesign or porting these systems
 * to other newsreaders.
 */
    kill_thresh_active = FALSE;  /* kill thresholds are generic */
    /* July 24, 1993: changed default of sc_savescores to TRUE */
    sc_savescores = TRUE;

/* CONSIDER: (for sc_init callers) is lastart properly set yet? */
    sc_fill_max = absfirst - 1;
#ifdef SCAN_ART
    if (sa_mode_read_elig || firstart > lastart)
	sc_fill_read = TRUE;
    else
#endif
	sc_fill_read = FALSE;

    if (sf_verbose) {
	printf("\nScoring articles...");
	fflush(stdout);		/* print it *now* */
    }

    sc_initialized = TRUE;	/* little white lie for lookahead */
    /* now is a good time to load a saved score-list which may exist */
    if (!sc_rescoring) {	/* don't load if rescoring */
	sc_load_scores();	/* will be quiet if non-existent */
	i = firstart;
	if (sc_fill_read)
	    i = absfirst;
	if (sc_sf_force_init)
	    i = lastart+1;	/* skip loop */
	for (i = article_first(i); i <= lastart; i = article_next(i)) {
	    if (!SCORED(i) && (sc_fill_read || article_unread(i)))
		break;
	}
	if (i == lastart)	/* all scored */
	    sc_sf_delay = TRUE;
    }
    if (sc_sf_force_init)
	sc_sf_delay = FALSE;

    if (!sc_sf_delay)
	sf_init();	/* initialize the scorefile code */

    sc_do_spin = FALSE;
    for (i = article_last(lastart); i >= absfirst; i = article_prev(i)) {
	if (SCORED(i))
	    break;
    }
    if (i < absfirst) {			/* none scored yet */
	/* score one article, or give up */
	for (a = article_last(lastart); a >= absfirst; a = article_prev(a)) {
	    sc_score_art(a,TRUE);	/* I want it *now* */
	    if (SCORED(a))
		break;
	}
	if (a < absfirst) {		/* no articles scored */
	    if (sf_verbose)
		printf("\nNo articles available for scoring\n");
	    sc_cleanup();
	    return;
	}
    }

    /* if no scoring rules/methods are present, score everything */
    /* XXX will be bad if later methods are added. */
    if (!sf_num_entries) {
	/* score everything really fast */
	for (a = article_last(lastart); a >= absfirst; a = article_prev(a))
	    sc_score_art(a,TRUE);
    }
    if (pend_wait) {
	bool waitflag;		/* if true, use key pause */

	waitflag = TRUE;	/* normal mode: wait for key first */
	if (sf_verbose && waitflag) {
#ifdef PENDING
	    printf("(press key to start reading)");
#else
	    printf("(interrupt to start reading)");
#endif
	    fflush(stdout);
	}
	if (waitflag) {
	    setspin(SPIN_FOREGROUND);
	    sc_do_spin = TRUE;		/* really do it */
	}
	sc_lookahead(TRUE,waitflag);	/* jump in *now* */
	if (waitflag) {
	    sc_do_spin = FALSE;
	    setspin(SPIN_POP);
	}
    }
    if (sf_verbose)
	putchar('\n') FLUSH;

    sc_initialized = TRUE;
}

void
sc_cleanup()
{
    if (!sc_initialized)
	return;

    if (sc_savescores)
	sc_save_scores();
    sc_loaded_count = 0;

    if (sf_verbose) {
	printf("\nCleaning up scoring...");
	fflush(stdout);
    }

    if (!sc_sf_delay)
	sf_clean();	/* let the scorefile do whatever cleaning it needs */
    sc_initialized = FALSE;

    if (sf_verbose)
	printf("Done.\n") FLUSH;
}

void
sc_set_score(a,score)
ART_NUM a;
int score;
{
    register ARTICLE* ap;

    if (is_unavailable(a))	/* newly unavailable */
	return;
    if (kill_thresh_active && score <= kill_thresh && article_unread(a))
	oneless_artnum(a);

    ap = article_ptr(a);
    ap->score = score;	/* update the score */
    ap->scoreflags |= SFLAG_SCORED;
#ifdef SCAN
    s_order_changed = TRUE;	/* resort */
#endif
}

/* Hopefully people will write more scoring routines later */
/* This is where you should add hooks for new scoring methods. */
void
sc_score_art_basic(a)
ART_NUM a;
{
    int score;
#ifdef USE_FILTER
    int sc;
# ifdef FILTER_DEBUG
    ARTICLE* ap;
# endif
#endif

    score = 0;
    score += sf_score(a);	/* get a score */

#ifdef USE_FILTER

# ifdef FILTER_DEBUG
    filter_error_file = fopen("/tmp/score.log", "a");
# endif

    sc = filter(a);
    score += sc;

# ifdef FILTER_DEBUG
    ap = article_find(a);
    if (ap && ap->refs)
	fprintf(filter_error_file, "%s: article %ld got score %ld\n",
		current_ng->rcline, (long)a, (long)sc);
    fclose(filter_error_file);
# endif /* FILTER_DEBUG */
#endif /* USE_FILTER */

    if (sc_do_spin)		/* appropriate to spin */
	spin(20);		/* keep the user amused */
    sc_set_score(a,score);	/* set the score */
}

/* Returns an article's score, scoring it if necessary */
int
sc_score_art(a,now)
ART_NUM a;
bool_int now;	/* if TRUE, sort the scores if necessary... */
{
    if (a < absfirst || a > lastart) {
#if 0
 	printf("\nsc_score_art: illegal article# %d\n",a) FLUSH;
#endif
	return LOWSCORE;		/* definitely unavailable */
    }
    if (is_unavailable(a))
	return LOWSCORE;

    if (sc_initialized == FALSE) {
	sc_delay = FALSE;
	sc_sf_force_init = TRUE;
	sc_init(FALSE);
	sc_sf_force_init = FALSE;
    }

    if (!SCORED(a)) {
	if (sc_sf_delay) {
	    sf_init();
	    sc_sf_delay = FALSE;
	}
	sc_score_art_basic(a);
    }
    if (is_unavailable(a))
	return LOWSCORE;
    return article_ptr(a)->score;
}
	
/* scores articles in a range */
/* CONSIDER: option for scoring only unread articles (obey sc_fill_unread?) */
void
sc_fill_scorelist(first,last)
ART_NUM first,last;
{
    int i;

    for (i = article_first(first); i <= last; i = article_next(i))
	(void)sc_score_art(i,FALSE);	/* will be sorted later... */
}

/* consider having this return a flag (is finished/is not finished) */

/* flag == TRUE means sort now, FALSE means wait until later (not used)
 * nowait == TRUE means start scoring immediately (group entry)
 * FALSE means use NICEBG if available
 */
void
sc_lookahead(flag, nowait)
bool_int flag;
bool_int nowait;
{
    ART_NUM oldart = openart;
    ART_POS oldartpos;

    if (!sc_initialized)
	return;			/* no looking ahead now */

#ifdef PENDING
    if (input_pending())
	return;			/* delay as little as possible */
#endif
    if (!sc_initialized)
	return;		/* don't score then... */
#ifdef PENDING
#ifdef NICEBG
    if (sc_mode_nicebg && !nowait)
	if (wait_key_pause(10))		/* wait up to 1 second for a key */
	    return;
#endif
#endif
    if (oldart)			/* Was there an article open? */
	oldartpos = tellart();	/* where were we in it? */
#ifndef PENDING
    if (int_count)
	int_count = 0;		/* clear the interrupt count */
#endif
    /* prevent needless looping below */
    if (sc_fill_max < firstart && !sc_fill_read)
	sc_fill_max = article_first(firstart)-1;
    else
	sc_fill_max = article_first(sc_fill_max);
    while (sc_fill_max < lastart
#ifdef PENDING
     && !input_pending()
#endif
    ) { 
#ifndef PENDING
	if (int_count > 0) {
	    int_count = 0;
	    return;	/* user requested break */
	}
#endif
	sc_fill_max = article_next(sc_fill_max);
	/* skip over some articles quickly */
	while (sc_fill_max < lastart
	 && (SCORED(sc_fill_max)
	  || (!sc_fill_read && !article_unread(sc_fill_max))))
	    sc_fill_max = article_next(sc_fill_max);

	if (SCORED(sc_fill_max))
	    continue;
	if (!sc_fill_read)	/* score only unread */
	    if (!article_unread(sc_fill_max))
		continue;
	(void)sc_score_art(sc_fill_max,FALSE);
    }
    if (oldart)			/* copied from cheat.c */
	artopen(oldart,oldartpos);	/* do not screw the pager */
}

int
sc_percent_scored()
{
    int i,total,scored;

    if (!sc_initialized)
	return 0;	/* none scored */
    if (sc_fill_max == lastart)
	return 100;
    i = firstart;
#ifdef SCAN_ART
    if (sa_mode_read_elig)
	i = absfirst;
#endif
    total = scored = 0;
    for (i = article_first(i); i <= lastart; i = article_next(i)) {
	if (!article_exists(i))
	    continue;
	if (!article_unread(i)
#ifdef SCAN_ART
	 && !sa_mode_read_elig
#endif
	)
	    continue;
	total++;
	if (SCORED(i))
	    scored++;
    } /* for */
    if (total == 0)
	return 0;
    return (scored*100) / total;
}

void
sc_rescore_arts()
{
    ART_NUM a;
    bool old_spin;

    if (!sc_initialized) {
	if (sc_delay) {
	    sc_delay = FALSE;
	    sc_sf_force_init = TRUE;
	    sc_init(TRUE);
	    sc_sf_force_init = FALSE;
	}
    } else if (sc_sf_delay) {
	sf_init();
	sc_sf_delay = FALSE;
    }
    if (!sc_initialized) {
	printf("\nScoring is not initialized, aborting command.\n") FLUSH;
	return;
    }
    /* I think sc_do_spin will always be false, but why take chances? */
    old_spin = sc_do_spin;
    setspin(SPIN_FOREGROUND);
    sc_do_spin = TRUE;				/* amuse the user */
    for (a = article_first(absfirst); a <= lastart; a = article_next(a)) {
	if (article_exists(a))
	    sc_score_art_basic(a);		/* rescore it then */
    }
    sc_do_spin = old_spin;
    setspin(SPIN_POP);
#ifdef SCAN_ART
    if (sa_in) {
	s_ref_all = TRUE;
	s_refill = TRUE;
	s_top_ent = 0;		/* make sure the refill starts from top */
    }
#endif
}

/* Wrapper to isolate scorefile functions from the rest of the world */
/* corrupted (:-) 11/12/92 by CAA for online rescoring */
void
sc_append(line)
char* line;
{
    char filechar;
    if (!line)		/* empty line */
	return;

    if (!sc_initialized) {
	if (sc_delay) {
	    sc_delay = FALSE;
	    sc_sf_force_init = TRUE;
	    sc_init(TRUE);
	    sc_sf_force_init = FALSE;
	}
    } else if (sc_sf_delay) {
	sf_init();
	sc_sf_delay = FALSE;
    }
    if (!sc_initialized) {
	printf("\nScoring is not initialized, aborting command.\n") FLUSH;
	return;
    }
    if (!*line) {
	line = sc_easy_append();
	if (!line)
	    return;		/* do nothing with empty string */
    }
    filechar = *line;			/* first char */
    sf_append(line);
    if (filechar == '!') {
	printf("\nRescoring articles...");
	fflush(stdout);
	sc_rescore_arts();
	printf("Done.\n") FLUSH;
#ifdef SCAN_ART
	if (sa_initialized)
	    s_top_ent = -1;		/* reset top of page */
#endif
    }
}

void
sc_rescore()
{
    sc_rescoring = TRUE;	/* in case routines need to know */
    sc_cleanup();	/* get rid of the old */
    sc_init(TRUE);	/* enter the new... (wait for rescore) */
#ifdef SCAN_ART
    if (sa_initialized) {
	s_top_ent = -1;	/* reset top of page */
	s_refill = TRUE;	/* make sure a refill is done */
    }
#endif
    sc_rescoring = FALSE;
}

/* May have a very different interface in the user versions */
void
sc_score_cmd(line)
char* line;
{
    long i, j;
    char* s;

    if (!sc_initialized) {
	if (sc_delay) {
	    sc_delay = FALSE;
	    sc_sf_force_init = TRUE;
	    sc_init(TRUE);
	    sc_sf_force_init = FALSE;
	}
    } else if (sc_sf_delay) {
	sf_init();
	sc_sf_delay = FALSE;
    }
    if (!sc_initialized) {
	printf("\nScoring is not initialized, aborting command.\n") FLUSH;
	return;
    }
    if (!*line) {
	line = sc_easy_command();
	if (!line)
	    return;		/* do nothing with empty string */
	if (*line == '\"') {
	    buf[0] = '\0';
	    sc_append(buf);
	    return;
	}
    }
    switch (*line) {
      case 'f':	/* fill (useful when PENDING is unavailable) */
	printf("Scoring more articles...");
	fflush(stdout);	/* print it now */
	setspin(SPIN_FOREGROUND);
	sc_do_spin = TRUE;
	sc_lookahead(TRUE,FALSE);
	sc_do_spin = FALSE;
	setspin(SPIN_POP);
	/* consider a "done" message later,
	 * *if* lookahead did all the arts */
	putchar('\n') FLUSH;
	break;
      case 'r':	/* rescore */
	printf("Rescoring articles...\n") FLUSH;
	sc_rescore();
	break;
      case 's':	/* verbose score for this article */
	/* XXX CONSIDER: A VERBOSE-SCORE ROUTINE (instead of this hack) */
	i = 0;	/* total score */
	sf_score_verbose = TRUE;
	j = sf_score(art);
	sf_score_verbose = FALSE;
	printf("Scorefile total score: %ld\n",j);
	i += j;
	j = sc_score_art(art,TRUE);
	if (i != j) {
	    /* Consider resubmitting article to filter? */
	    printf("Other scoring total: %ld\n", j - i) FLUSH;
	}
	printf("Total score is %ld\n",i) FLUSH;
	break;
      case 'e':	/* edit scorefile or other file */
	for (s = line+1; *s == ' ' || *s == '\t'; s++) ;
	if (!*s)	/* empty name for scorefile */
	    sf_edit_file("\"");	/* edit local scorefile */
	else
	    sf_edit_file(s);
	break;
      default:
	printf("Unknown scoring command |%s|\n",line);
    } /* switch */
}

void
sc_kill_threshold(thresh)
int thresh;		/* kill all articles with this score or lower */
{
    ART_NUM a;

    for (a = article_first(firstart); a <= lastart; a = article_next(a)) {
	if (article_ptr(a)->score <= thresh && article_unread(a)
#ifdef SCAN_ART
	    /* CAA 6/19/93: this is needed for zoom mode */
	 && sa_basic_elig(sa_artnum_to_ent(a))
#endif
	)
	    oneless_artnum(a);
    }
}
#endif /* SCORE */
