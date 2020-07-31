/* This file Copyright 1992 by Clifford A. Adams */
/* sadesc.c
 *
 */

#include "EXTERN.h"
#include "common.h"
#ifdef SCAN_ART
#include "list.h"
#include "hash.h"
#include "cache.h"
#include "bits.h"
#include "head.h"
/* currently used for fast author fetch when group is threaded */
#include "ngdata.h"
#include "term.h"	/* for standout */
#include "rthread.h"
#include "rt-util.h"	/* compress_from() */
#include "scanart.h"
#include "samain.h"
#include "sacmd.h"	/* for sa_wrap_next_author() */
#include "sadisp.h"
#include "sathread.h"
#include "scan.h"
#ifdef SCORE
#include "score.h"
#endif
#include "INTERN.h"
#include "sadesc.h"


/* returns statchars in temp space... */
char*
sa_get_statchars(a,line)
long a;
int line;		/* which status line (1 = first) */
{
    static char char_buf[16];

/* Debug */
#if 0
    printf("entry: sa_get_statchars(%d,%d)\n",(int)a,line) FLUSH;
#endif

#if 0
    /* old 5-column status */
    switch (line) {
      case 1:
	strcpy(char_buf,".....");
	if (sa_marked(a))
	    char_buf[4] = 'x';
	if (sa_selected1(a))
	    char_buf[3] = '+';
	if (was_read(sa_ents[a].artnum))
	    char_buf[0] = '-';
	else
	    char_buf[0] = '+';
	break;
      default:
	strcpy(char_buf,"     ");
	break;
    } /* switch */
#else
    switch (line) {
      case 1:
	strcpy(char_buf,"...");
	if (sa_marked(a))
	    char_buf[2] = 'x';
	if (sa_selected1(a))
	    char_buf[1] = '*';
	if (was_read(sa_ents[a].artnum))
	    char_buf[0] = '-';
	else
	    char_buf[0] = '+';
	break;
      default:
	strcpy(char_buf,"   ");
	break;
    } /* switch */
#endif
    return char_buf;
}

char*
sa_desc_subject(e)
long e;
{
    char* s;
    char* s1;
    static char sa_subj_buf[256];

    /* fetchlines saves its arguments */
    s = fetchlines(sa_ents[e].artnum,SUBJ_LINE);

    if (!s || !*s) {
	if (s)
	    free(s);
	sprintf(sa_subj_buf,"(no subject)");
	return sa_subj_buf;
    }
    strncpy(sa_subj_buf,s,250);
    free(s);
    s1 = sa_subj_buf;
    if (*s1 == 'r' || *s1 == 'R') {
	if (*++s1 == 'e' || *s1 == 'E') {
	    if (*++s1 ==':') {
		*s1 = '>';		/* more cosmetic "Re:" */
		return s1;
	    }
	}
    }
    return sa_subj_buf;
}

/* NOTE: should redesign later for the "menu" style... */
char*
sa_get_desc(e,line,trunc)
long e;		/* entry number */
int line;
bool_int trunc;		/* should it be truncated? */
{
    static char desc_buf[1024];
    char* s;
    bool use_standout;	/* if TRUE, use stdout on line */
    ART_NUM artnum;

    artnum = sa_ents[e].artnum;
    use_standout = FALSE;
    switch (line) {
      case 1:
	desc_buf[0] = '\0';	/* initialize the buffer */
	if (sa_mode_desc_artnum) {
	    sprintf(sa_buf,"%6d ",(int)artnum);
	    strcat(desc_buf,sa_buf);
	}
#ifdef SCORE
	if (sc_initialized && sa_mode_desc_score) {
	    /* we'd like the score now */
	    sprintf(sa_buf,"[%4d] ",sc_score_art(artnum,TRUE));
	    strcat(desc_buf,sa_buf);
	}
#endif /* SCORE */
	if (sa_mode_desc_threadcount) {
	    sprintf(sa_buf,"(%3d) ",sa_subj_thread_count(e));
	    strcat(desc_buf,sa_buf);
	}
	if (sa_mode_desc_author) {
#if 0
	    if (trunc)
		sprintf(sa_buf,"%s ",padspaces(sa_desc_author(e,16),16));
	    else
		sprintf(sa_buf,"%s ",sa_desc_author(e,40));
	    strcat(desc_buf,sa_buf);
#endif
	    if (trunc)
		strcat(desc_buf,compress_from(article_ptr(artnum)->from,16));
	    else
		strcat(desc_buf,compress_from(article_ptr(artnum)->from,200));
	    strcat(desc_buf," ");
	}
	if (sa_mode_desc_subject) {
	    sprintf(sa_buf,"%s",sa_desc_subject(e));
	    strcat(desc_buf,sa_buf);
	}
	break;
      case 2:	/* summary line (test) */
	s = fetchlines(artnum,SUMRY_LINE);
	if (s && *s) { /* we really have one */
	    int i;		/* number of spaces to indent */
	    char* s2;	/* for indenting */

/* include the following line to use standout mode */
#if 0
	    use_standout = TRUE;
#endif
	    i = 0;
	    /* if variable widths used later, use them */
	    if (sa_mode_desc_artnum)
		i += 7;
#ifdef SCORE
	    if (sc_initialized && sa_mode_desc_score)
		i += 7;
#endif
	    if (sa_mode_desc_threadcount)
		i += 6;
	    s2 = desc_buf;
	    while (i--) *s2++ = ' ';
#ifdef HAS_TERMLIB
	    if (use_standout)
		sprintf(s2,"Summary: %s%s",tc_SO,s);
	    else
#endif
		sprintf(s2,"Summary: %s",s);
	    break;
	}
	/* otherwise, we might have had a keyword */
	/* FALL THROUGH */
      case 3:	/* Keywords (test) */
	s = fetchlines(artnum,KEYW_LINE);
	if (s && *s) { /* we really have one */
	    int i;		/* number of spaces to indent */
	    char* s2;	/* for indenting */
/* include the following line to use standout mode */
#if 0
	    use_standout = TRUE;
#endif
	    i = 0;
	    /* if variable widths used later, use them */
	    if (sa_mode_desc_artnum)
		i += 7;
#ifdef SCORE
	    if (sc_initialized && sa_mode_desc_score)
		i += 7;
#endif
	    if (sa_mode_desc_threadcount)
		i += 6;
	    s2 = desc_buf;
	    while (i--) *s2++ = ' ';
#ifdef HAS_TERMLIB
	    if (use_standout)
		sprintf(s2,"Keys: %s%s",tc_SO,s);
	    else
#endif
		sprintf(s2,"Keys: %s",s);
	    break;
	}
	/* FALL THROUGH */
      default:	/* no line I know of */
	/* later return NULL */
	sprintf(desc_buf,"Entry %ld: Nonimplemented Description LINE",e);
	break;
    } /* switch (line) */
    if (trunc)
	desc_buf[s_desc_cols] = '\0';	/* make sure it's not too long */
#ifdef HAS_TERMLIB
    if (use_standout)
	strcat(desc_buf,tc_SE);	/* end standout mode */
#endif
    /* take out bad characters (replace with one space) */
    for (s = desc_buf; *s; s++)
	switch (*s) {
	  case Ctl('h'):
	  case '\t':
	  case '\n':
	  case '\r':
	    *s = ' ';
	}
    return desc_buf;
}

/* returns # of lines the article occupies in total... */
int
sa_ent_lines(e)
long e;			/* the entry number */
{
    char* s;
    ART_NUM artnum;
    int num = 1;

    artnum = sa_ents[e].artnum;
    if (sa_mode_desc_summary) {
	s = fetchlines(artnum,SUMRY_LINE);
	if (s && *s)
	    num++;	/* just a test */
	if (s)
	    free(s);
    }
    if (sa_mode_desc_keyw) {
	s = fetchlines(artnum,KEYW_LINE);
	if (s && *s)
	    num++;	/* just a test */
	if (s)
	    free(s);
    }
    return num;
}

#endif /* SCAN */
