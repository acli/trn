/* This file Copyright 1993 by Clifford A. Adams */
/* scoresave.c
 *
 * Saving/restoring scores from a file.
 */

#include "EXTERN.h"
#include "common.h"
#ifdef SCORE
#include "list.h"
#include "hash.h"
#include "cache.h"
#include "bits.h"
#include "intrp.h"		/* for filexp */
#include "ng.h"			/* art */
#include "ngdata.h"
#include "util.h"		/* several */
#include "util2.h"
#include "env.h"		/* getval */
#ifdef SCAN
#include "scan.h"
#endif
#ifdef SCAN_ART
#include "scanart.h"
#include "samain.h"
#include "samisc.h"
#endif
#include "score.h"
#include "INTERN.h"
#include "scoresave.h"

static int num_lines = 0;
static int lines_alloc = 0;
static char** lines = NULL;

static char lbuf[LBUFLEN];
static char lbuf2[LBUFLEN];		/* what's another buffer between... */

static int loaded;
static int used;
static int saved;
static ART_NUM last;

void
sc_sv_add(str)
char* str;
{
    if (num_lines == lines_alloc) {
	lines_alloc += 100;
	lines = (char**)saferealloc((char*)lines,lines_alloc * sizeof (char*));
    }
    lines[num_lines] = savestr(str);
    num_lines++;
}

void
sc_sv_delgroup(gname)
char* gname;
{
    char* s;
    int i;
    int start;

    for (i = 0; i < num_lines; i++) {
	s = lines[i];
	if (s && *s == '!' && strEQ(gname,s+1))
	    break;
    }
    if (i == num_lines)
	return;		/* group not found */
    start = i;
    free(lines[i]);
    lines[i] = NULL;
    for (i++; i < num_lines; i++) {
	s = lines[i];
	if (s && *s == '!')
	    break;
	if (s) {
	    free(s);
	    lines[i] = NULL;
	}
    }
    /* copy into the hole (if any) */
    for ( ; i < num_lines; i++)
	lines[start++] = lines[i];
    num_lines -= (i-start);
}

/* get the file containing scores into memory */
void
sc_sv_getfile()
{
    char* s;
    FILE* fp;

    num_lines = lines_alloc = 0;
    lines = NULL;

    s = getval("SAVESCOREFILE","%+/savedscores");
    fp = fopen(filexp(s),"r");
    if (!fp) {
#if 0
	printf("Could not open score save file for reading.\n") FLUSH;
#endif
	return;
    }
    while (fgets(lbuf,LBUFLEN-2,fp)) {
	lbuf[strlen(lbuf)-1] = '\0';	/* strip \n */
	sc_sv_add(lbuf);
    }
    fclose(fp);
}

/* save the memory into the score file */
void
sc_sv_savefile()
{
    char* s;
    FILE* tmpfp;
    char* savename;
    int i;

    if (num_lines == 0)
	return;
    waiting = TRUE;	/* don't interrupt */
    s = getval("SAVESCOREFILE","%+/savedscores");
    savename = savestr(filexp(s));
    strcpy(lbuf,savename);
    strcat(lbuf,".tmp");
    tmpfp = fopen(lbuf,"w");
    if (!tmpfp) {
#if 0
	printf("Could not open score save temp file %s for writing.\n",
	       lbuf) FLUSH;
#endif
	free(savename);
	waiting = FALSE;
	return;
    }
    for (i = 0; i < num_lines; i++) {
	if (lines[i])
	    fprintf(tmpfp,"%s\n",lines[i]);
	if (ferror(tmpfp)) {
	    fclose(tmpfp);
	    free(savename);
	    printf("\nWrite error in temporary save file %s\n",lbuf) FLUSH;
	    printf("(keeping old saved scores)\n");
	    UNLINK(lbuf);
	    waiting = FALSE;
	    return;
	}
    }
    fclose(tmpfp);
    UNLINK(savename);
    RENAME(lbuf,savename);
    waiting = FALSE;
}

/* returns the next article number (after the last one used) */
ART_NUM
sc_sv_use_line(line,a)
char* line;
ART_NUM a;	/* art number to start with */
{
    char* s;
    char* p;
    char c1,c2;
    int score;
    int x;

    score = 0;	/* get rid of warning */
    s = line;
    if (!s)
	return a;
    while (*s) {
	switch(*s) {
	  case 'A': case 'B': case 'C': case 'D': case 'E':
	  case 'F': case 'G': case 'H': case 'I':
	    /* negative starting digit */
	    p = s;
	    c1 = *s;
	    *s = '0' + ('J' - *s);	/* convert to first digit */
	    s++;
	    while (isdigit(*s)) s++;
	    c2 = *s;
	    *s = '\0';
	    score = 0 - atoi(p);
	    *p = c1;
	    *s = c2;
	    loaded++;
	    if (is_available(a) && article_unread(a)) {
		sc_set_score(a,score);
		used++;
	    }
	    a++;
	    break;
	  case 'J': case 'K': case 'L': case 'M': case 'N':
	  case 'O': case 'P': case 'Q': case 'R': case 'S':
	    /* positive starting digit */
	    p = s;
	    c1 = *s;
	    *s = '0' + (*s - 'J');	/* convert to first digit */
	    s++;
	    while (isdigit(*s)) s++;
	    c2 = *s;
	    *s = '\0';
	    score = atoi(p);
	    *p = c1;
	    *s = c2;
	    loaded++;
	    if (is_available(a) && article_unread(a)) {
		sc_set_score(a,score);
		used++;
	    }
	    a++;
	    break;
	  case 'r':	/* repeat */
	    s++;
	    p = s;
	    if (!isdigit(*s)) {
		/* simple case, just "r" */
		x = 1;
	    } else {
		s++;
		while (isdigit(*s)) s++;
		c1 = *s;
		*s = '\0';
		x = atoi(p);
		*s = c1;
	    }
	    for ( ; x; x--) {
		loaded++;
		if (is_available(a) && article_unread(a)) {
		    sc_set_score(a,score);
		    used++;
		}
		a++;
	    }
	    break;
	  case 's':	/* skip */
	    s++;
	    p = s;
	    if (!isdigit(*s)) {
		/* simple case, just "s" */
		a += 1;
	    } else {
		s++;
		while (isdigit(*s)) s++;
		c1 = *s;
		*s = '\0';
		x = atoi(p);
		*s = c1;
		a += x;
	    }
	    break;
	} /* switch */
    } /* while */
    return a;
}

ART_NUM
sc_sv_make_line(a)
ART_NUM a;
{
    char* s;
    bool lastscore_valid = FALSE;
    int num_output = 0;
    int score,lastscore;
    int i;
    bool neg_flag;

    s = lbuf;
    *s++ = '.';
    lastscore = 0;

    for (a = article_first(a); a <= lastart && num_output < 50; a = article_next(a)) {
	if (article_unread(a) && SCORED(a)) {
	    if (last != a-1) {
		if (last == a-2) {
		    *s++ = 's';
		    num_output++;
		} else {
		    sprintf(s,"s%ld",(a-last)-1);
		    s = lbuf + strlen(lbuf);
		    num_output++;
		}
	    }
	    /* print article's score */
	    score = article_ptr(a)->score;
	    /* check for repeating scores */
	    if (score == lastscore && lastscore_valid) {
		a = article_next(a);
		for (i = 1; a <= lastart && article_unread(a) && SCORED(a)
			 && article_ptr(a)->score == score; i++)
		    a = article_next(a);
		a = article_prev(a);	/* prepare for the for loop increment */
		if (i == 1) {
		    *s++ = 'r';		/* repeat one */
		    num_output++;
		} else {
		    sprintf(s,"r%d",i);	/* repeat >one */
		    s = lbuf + strlen(lbuf);
		    num_output++;
		}
		saved += i-1;
	    } else {	/* not a repeat */
		i = score;
		if (i < 0) {
		    neg_flag = TRUE;
		    i = 0 - i;
		} else
		    neg_flag = FALSE;
		sprintf(s,"%d",i);
		i = (*s - '0');
		if (neg_flag)
		    *s++ = 'J' - i;
		else
		    *s++ = 'J' + i;
		s = lbuf + strlen(lbuf);
		num_output++;
		lastscore_valid = TRUE;
	    }
	    lastscore = score;
	    last = a;
	    saved++;
	} /* if */
    } /* for */
    *s = '\0';
    sc_sv_add(lbuf);
    return a;
}

void
sc_load_scores()
{
/* lots of cleanup needed here */
    ART_NUM a = 0;
    char* s;
    char* gname;
    int i;
    int total,scored;
    bool verbose;

    sc_save_new = -1;		/* just in case we exit early */
    loaded = used = 0;
    sc_loaded_count = 0;

    /* verbosity is only really useful for debugging... */
    verbose = 0;

    if (num_lines == 0)
	sc_sv_getfile();

    gname = savestr(filexp("%C"));

    for (i = 0; i < num_lines; i++) {
	s = lines[i];
	if (s && *s == '!' && strEQ(s+1,gname))
	    break;
    }
    if (i == num_lines)
	return;		/* no scores loaded */
    i++;

    if (verbose) {
	printf("\nLoading scores...");
	fflush(stdout);
    }
    while (i < num_lines) {
	s = lines[i++];
	if (!s)
	    continue;
	switch (*s) {
	  case ':':
	    a = atoi(s+1);	/* set the article # */
	    break;
	  case '.':			/* longer score line */
	    a = sc_sv_use_line(s+1,a);
	    break;
	  case '!':			/* group of shared file */
	    i = num_lines;
	    break;
	  case 'v':			/* version number */
	    break;			/* not used now */
	  case '\0':			/* empty string */
	  case '#':			/* comment */
	    break;
	  default:
	    /* don't even try to deal with it */
	    return;
	} /* switch */
    } /* while */

    sc_loaded_count = loaded;
    a = firstart;
#ifdef SCAN_ART
    if (sa_mode_read_elig)
	a = absfirst;
#endif
    total = scored = 0;
    for (a = article_first(a); a <= lastart; a = article_next(a)) {
	if (!article_exists(a))
	    continue;
	if (!article_unread(a)
#ifdef SCAN_ART
	 && !sa_mode_read_elig
#endif
	)
	    continue;
	total++;
	if (SCORED(a))
	    scored++;
    } /* for */

    /* sloppy plurals (:-) */
    if (verbose)
	printf("(%d/%d/%d scores loaded/used/unscored)\n",
	       loaded,used,total-scored) FLUSH;

    sc_save_new = total-scored;
#ifdef SCAN
    if (sa_initialized)
	s_top_ent = -1;	/* reset top of page */
#endif
}

void
sc_save_scores()
{
    ART_NUM a;
    char* gname;

    saved = 0;
    last = 0;

    waiting = TRUE;	/* DON'T interrupt */
    gname = savestr(filexp("%C"));
    /* not being able to open is OK */
    if (num_lines > 0) {
	sc_sv_delgroup(gname);	/* delete old group */
    } else {		/* there was no old file */
	sc_sv_add("#STRN saved score file.");
	sc_sv_add("v1.0");
    }
    sprintf(lbuf2,"!%s",gname);	/* add the header */
    sc_sv_add(lbuf2);

    a = firstart;
    sprintf(lbuf2,":%ld",a);
    sc_sv_add(lbuf2);
    last = a-1;
    while (a <= lastart)
	a = sc_sv_make_line(a);
    waiting = FALSE;
}
#endif /* SCORE */
