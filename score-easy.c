/* This file Copyright 1993 by Clifford A. Adams */
/* score-easy.c
 *
 * Simple interactive menus for scorefile tasks.
 */

#include "EXTERN.h"
#include "common.h"
#ifdef SCORE
#include "search.h"
#include "term.h"
#include "util.h"
#include "score.h"
#include "scorefile.h"
#include "INTERN.h"
#include "score-easy.h"

/* new line to return to the caller. */
static char sc_e_newline[LBUFLEN];

/* returns new string or NULL to abort. */
char*
sc_easy_append()
{
    char* s;
    bool q_done;	/* if TRUE, we are finished with current question */
    char filechar;
    long score;
    char ch;

    filechar = '\0';	/* GCC warning avoidance */
    s = sc_e_newline;
    printf("\nScorefile easy append mode.\n") FLUSH;
    q_done = FALSE;
    while (!q_done) {
	printf("0) Exit.\n") FLUSH;
	printf("1) List the current scorefile abbreviations.\n");
	printf("2) Add an entry to the global scorefile.\n");
	printf("3) Add an entry to this newsgroup's scorefile.\n");
	printf("4) Add an entry to another scorefile.\n");
	printf("5) Use a temporary scoring rule.\n");
	ch = menu_get_char();
	q_done = TRUE;
	switch (ch) {
	  case '0':
	    return NULL;
	  case '1':
	    strcpy(sc_e_newline,"?");
	    return sc_e_newline;
	  case '2':
	    filechar = '*';
	    break;
	  case '3':
	    filechar = '"';
	    break;
	  case '4':
	    filechar = '\0';
	    break;
	  case '5':
	    filechar = '!';
	    break;
	  case 'h':
	    printf("No help available (yet).\n") FLUSH;
	    q_done = FALSE;
	    break;
	  default:
	    q_done = FALSE;
	    break;
	}
    }
    while (filechar == '\0') {	/* choose one */
	printf("Type the (single character) abbreviation of the scorefile:");
	fflush(stdout);
	eat_typeahead();
	getcmd(buf);
	printf("%c\n",*buf) FLUSH;
	filechar = *buf;
	/* If error checking is done later, then an error should set
	 * filechar to '\0' and continue the while loop.
	 */
    }
    *s++ = filechar;
    *s++ = ' ';
    q_done = FALSE;
    while (!q_done) {
	printf("What type of line do you want to add?\n");
	printf("0) Exit.\n");
	printf("1) A scoring rule line.\n");
	printf("   (for the current article's author/subject)\n");
	printf("2) A command, comment, or other kind of line.\n");
	printf("   (use this for any other kind of line)\n");
	printf("\n[Other line formats will be supported later.]\n");
	ch = menu_get_char();
	q_done = TRUE;
	switch (ch) {
	  case '0':
	    return NULL;
	  case '1':
	    break;
	  case '2':
	    printf("Enter the line below:\n");
	    fflush(stdout);
	    buf[0] = '>';
	    buf[1] = FINISHCMD;
	    if (finish_command(TRUE)) {
		sprintf(s,"%s",buf+1);
		return sc_e_newline;
	    }
	    printf("\n");
	    q_done = FALSE;
	    break;
	  case 'h':
	    printf("No help available (yet).\n") FLUSH;
	    q_done = FALSE;
	    break;
	  default:
	    q_done = FALSE;
	    break;
	}
    }
    q_done = FALSE;
    while (!q_done) {
	printf("Enter a score amount (like 10 or -6):");
	fflush(stdout);
	buf[0] = ' ';
	buf[1] = FINISHCMD;
	if (finish_command(TRUE)) {
	    score = atoi(buf+1);
	    if (score == 0)
		if (buf[1] != '0')
		    continue;	/* the while loop */
	    sprintf(s,"%ld",score);
	    s = sc_e_newline+strlen(sc_e_newline); /* point at terminator  */
	    *s++ = ' ';
	    q_done = TRUE;
	} else
	    printf("\n") FLUSH;
    }
    q_done = FALSE;
    while (!q_done) {
	printf("Do you want to:\n");
	printf("0) Exit.\n");
	printf("1) Give the score to the current subject.\n");
	printf("2) Give the score to the current author.\n");
/* add some more options here later */
/* perhaps fold regular-expression question here? */
	ch = menu_get_char();
	q_done = TRUE;
	switch (ch) {
	  case '0':
	    return NULL;
	  case '1':
	    *s++ = 'S';
	    *s++ = '\0';
	    return sc_e_newline;
	  case '2':
	    *s++ = 'F';
	    *s++ = '\0';
	    return sc_e_newline;
	  case 'h':
	    printf("No help available (yet).\n") FLUSH;
	    q_done = FALSE;
	    break;
	  default:
	    q_done = FALSE;
	    break;
	}
    }
    /* later ask for headers, pattern-matching, etc... */
    return NULL;
}

/* returns new string or NULL to abort. */
char*
sc_easy_command()
{
    char* s;
    bool q_done;	/* if TRUE, we are finished with current question */
    char ch;

    s = sc_e_newline;
    printf("\nScoring easy command mode.\n") FLUSH;
    q_done = FALSE;
    while (!q_done) {
	printf("0) Exit.\n");
	printf("1) Add something to a scorefile.\n");
	printf("2) Rescore the articles in the current newsgroup.\n");
	printf("3) Explain the current article's score.\n");
	printf("   (show the rules that matched this article)\n");
	printf("4) Edit this newsgroup's scoring rule file.\n");
	/* later add an option to edit an arbitrary file */
	printf("5) Continue scoring unscored articles.\n");
	ch = menu_get_char();
	q_done = TRUE;
	switch (ch) {
	  case '0':
	    return NULL;
	  case '1':
	    return "\"";	/* do an append command */
	  case '2':
	    return "r";
	  case '3':
	    return "s";
	  case '4':
	    /* add more later */
	    return "e";
	  case '5':
	    return "f";
	  case 'h':
	    printf("No help available (yet).\n") FLUSH;
	    q_done = FALSE;
	    break;
	  default:
	    q_done = FALSE;
	    break;
	}
    }
    return NULL;
}
#endif /* SCORE */
