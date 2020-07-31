/* sw.c
 */
/* This software is copyrighted as detailed in the LICENSE file. */


#include "EXTERN.h"
#include "common.h"
#include "list.h"
#include "hash.h"
#include "env.h"
#include "util.h"
#include "util2.h"
#include "cache.h"
#include "head.h"
#include "init.h"
#include "opt.h"
#include "only.h"
#include "term.h"
#include "intrp.h"
#include "trn.h"
#include "ngdata.h"
#include "rt-page.h"
#include "charsubst.h"
#include "INTERN.h"
#include "sw.h"

void
sw_file(tcbufptr)
char** tcbufptr;
{
    int initfd = open(*tcbufptr,0);

    if (initfd >= 0) {
	fstat(initfd,&filestat);
	if (filestat.st_size >= TCBUF_SIZE-1)
	    *tcbufptr = saferealloc(*tcbufptr,(MEM_SIZE)filestat.st_size+1);
	if (filestat.st_size) {
	    int len = read(initfd,*tcbufptr,(int)filestat.st_size);
	    (*tcbufptr)[len] = '\0';
	    sw_list(*tcbufptr);
	}
	else
	    **tcbufptr = '\0';
	close(initfd);
    }
}

/* decode a list of space separated switches */

void
sw_list(swlist)
char* swlist;
{
    register char* s;
    register char* p;
    register char inquote = 0;

    s = p = swlist;
    while (*s) {			/* "String, or nothing" */
	if (!inquote && isspace(*s)) {	/* word delimiter? */
	    for (;;) {
		while (isspace(*s)) s++;
		if (*s != '#')
		    break;
		while (*s && *s++ != '\n') ;
	    }
	    if (p != swlist)
		*p++ = '\0';		/* chop here */
	}
	else if (inquote == *s) {
	    s++;			/* delete trailing quote */
	    inquote = 0;		/* no longer quoting */
	}
	else if (!inquote && (*s == '"' || *s == '\'')) {
					/* OK, I know when I am not wanted */
	    inquote = *s++;		/* remember & del single or double */
	}
	else if (*s == '\\') {		/* quoted something? */
	    if (*++s != '\n') {		/* newline? */
		s = interp_backslash(p, s);
		p++;
	    }
	    s++;
	}
	else
	    *p++ = *s++;		/* normal char */
    }
    *p++ = '\0';
    *p = '\0';				/* put an extra null on the end */
    if (inquote) {
	printf("Unmatched %c in switch\n",inquote) FLUSH;
	termdown(1);
    }
    for (p = swlist; *p; /* p += strlen(p)+1 */ ) {
	decode_switch(p);
	while (*p++) ;			/* point at null + 1 */
    }
}

/* decode a single switch */

void
decode_switch(s)
register char* s;
{
    while (isspace(*s)) s++;		/* ignore leading spaces */
#ifdef DEBUG
    if (debug) {
	printf("Switch: %s\n",s) FLUSH;
	termdown(1);
    }
#endif
    if (*s != '-' && *s != '+') {	/* newsgroup pattern */
	setngtodo(s);
	if (mode == 'i')
	    ng_min_toread = 0;
    }
    else {				/* normal switch */
	bool upordown = *s == '-' ? TRUE : FALSE;
	char tmpbuf[LBUFLEN];

	switch (*++s) {
#ifdef TERMMOD
	case '=':
	    /*$$ fix this */
	    break;
#endif
#ifdef BAUDMOD
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
	    /*$$ fix this */
	    break;
#endif
	case '/':
	    set_option(OI_AUTO_SAVE_NAME, YESorNO(upordown));
	    break;
	case '+':
	    set_option(OI_USE_ADD_SEL, YESorNO(upordown));
	    set_option(OI_USE_NEWSGROUP_SEL, YESorNO(upordown));
	    if (upordown)
		set_option(OI_INITIAL_GROUP_LIST, YESorNO(0));
	    else
		set_option(OI_USE_NEWSRC_SEL, YESorNO(0));
	    break;
	case 'a':
	    set_option(OI_BKGND_THREADING, YESorNO(!upordown));
	    break;
	case 'A':
	    set_option(OI_AUTO_ARROW_MACROS, YESorNO(upordown));
	    break;
	case 'b':
	    set_option(OI_READ_BREADTH_FIRST, YESorNO(upordown));
	    break;
	case 'B':
	    set_option(OI_BKGND_SPINNER, YESorNO(upordown));
	    break;
	case 'c':
	    checkflag = upordown;
	    break;
	case 'C':
	    if (*++s == '=') s++;
	    set_option(OI_CHECKPOINT_NEWSRC_FREQUENCY, s);
	    break;
	case 'd': {
	    if (*++s == '=') s++;
	    set_option(OI_SAVE_DIR, s);
	    break;
	}
	case 'D':
#ifdef DEBUG
	    if (*++s == '=') s++;
	    if (*s) {
		if (upordown)
		    debug |= atoi(s);
		else
		    debug &= ~atoi(s);
	    }
	    else {
		if (upordown)
		    debug |= 1;
		else
		    debug = 0;
	    }
#else
	    printf("Trn was not compiled with -DDEBUG.\n") FLUSH;
	    termdown(1);
#endif
	    break;
	case 'e':
	    set_option(OI_ERASE_SCREEN, YESorNO(upordown));
	    break;
	case 'E':
	    if (*++s == '=') s++;
	    strcpy(tmpbuf,s);
	    s = index(tmpbuf,'=');
	    if (s) {
		*s++ = '\0';
		s = export(tmpbuf,s) - (s-tmpbuf);
		if (mode == 'i')
		    save_init_environment(s);
	    }
	    else {
		s = export(tmpbuf,nullstr) - strlen(tmpbuf) - 1;
		if (mode == 'i')
		    save_init_environment(s);
	    }
	    break;
	case 'f':
	    set_option(OI_NOVICE_DELAYS, YESorNO(!upordown));
	    break;
	case 'F':
	    set_option(OI_CITED_TEXT_STRING, s+1);
	    break;
	case 'g':
#ifdef INNERSEARCH
	    set_option(OI_GOTO_LINE_NUM, s+1);
#else
	    notincl("-g");
#endif
	    break;
	case 'G':
#ifdef EDIT_DISTANCE
	    set_option(OI_FUZZY_NEWSGROUP_NAMES, YESorNO(upordown));
#else
	    notincl("-G");
#endif        
	    break;
	case 'h':
	    if (!s[1]) {
		/* Free old user_htype list */
		while (user_htype_cnt > 1)
		    free(user_htype[--user_htype_cnt].name);
		bzero((char*)user_htypeix, 26);
	    }
	    /* FALL THROUGH */
	case 'H':
	    if (checkflag)
		break;
	    set_header(s+1,*s == 'h'? HT_HIDE : HT_MAGIC, upordown);
	    break;
	case 'i':
	    if (*++s == '=') s++;
	    set_option(OI_INITIAL_ARTICLE_LINES, s);
	    break;
	case 'I':
	    set_option(OI_APPEND_UNSUBSCRIBED_GROUPS, YESorNO(upordown));
	    break;
	case 'j':
	    set_option(OI_FILTER_CONTROL_CHARACTERS, YESorNO(!upordown));
	    break;
	case 'J':
	    if (*++s == '=') s++;
	    set_option(OI_JOIN_SUBJECT_LINES,
		       upordown && *s? s : YESorNO(upordown));
	    break;
	case 'k':
	    set_option(OI_IGNORE_THRU_ON_SELECT, YESorNO(upordown));
	    break;
	case 'K':
	    set_option(OI_AUTO_GROW_GROUPS, YESorNO(!upordown));
	    break;
	case 'l':
	    set_option(OI_MUCK_UP_CLEAR, YESorNO(upordown));
	    break;
	case 'L':
	    set_option(OI_ERASE_EACH_LINE, YESorNO(upordown));
	    break;
	case 'M':
	    if (upordown)
		set_option(OI_SAVEFILE_TYPE, "mail");
	    break;
	case 'm':
	    set_option(OI_PAGER_LINE_MARKING, s+1);
	    break;
	case 'N':
	    if (upordown)
		set_option(OI_SAVEFILE_TYPE, "norm");
	    break;
	case 'o':
	    if (*++s == '=') s++;
	    set_option(OI_OLD_MTHREADS_DATABASE, s);
	    break;
	case 'O':
	    if (*++s == '=') s++;
	    set_option(OI_NEWS_SEL_MODE, s);
	    if (*++s) {
		char tmpbuf[4];
		sprintf(tmpbuf, "%s%c", isupper(*s)? "r " : nullstr, *s);
		set_option(OI_NEWS_SEL_ORDER, tmpbuf);
	    }
	    break;
	case 'p':
	    if (*++s == '=') s++;
	    if (!upordown)
		s = YESorNO(0);
	    else {
		switch (*s) {
		case '+':
		    s = "thread";
		    break;
		case 'p':
		    s = "parent";
		    break;
		default:
		    s = "subthread";
		    break;
		}
	    }
	    set_option(OI_SELECT_MY_POSTS, s);
	    break;
	case 'q':
	    set_option(OI_NEWGROUP_CHECK, YESorNO(!upordown));
	    break;
        case 'Q':
#ifdef CHARSUBST
	    if (*++s == '=') s++;
	    set_option(OI_CHARSET, s);
#else
	    notincl("-Q");
#endif
	    break;
	case 'r':
	    set_option(OI_RESTART_AT_LAST_GROUP, YESorNO(upordown));
	    break;
	case 's':
	    if (*++s == '=') s++;
	    set_option(OI_INITIAL_GROUP_LIST, isdigit(*s)? s : YESorNO(0));
	    break;
	case 'S':
	    if (*++s == '=') s++;
	    set_option(OI_SCANMODE_COUNT, s);
	    break;
	case 't':
	    set_option(OI_TERSE_OUTPUT, YESorNO(upordown));
	    break;
	case 'T':
	    set_option(OI_EAT_TYPEAHEAD, YESorNO(!upordown));
	    break;
	case 'u':
	    set_option(OI_COMPRESS_SUBJECTS, YESorNO(!upordown));
	    break;
	case 'U':
	    unsafe_rc_saves = upordown;
	    break;
	case 'v':
	    set_option(OI_VERIFY_INPUT, YESorNO(upordown));
	    break;
	case 'V':
	    if (mode == 'i') {
		tc_LINES = 1000;
		tc_COLS = 1000;
		erase_screen = FALSE;
	    }
	    trn_version();
	    newline();
	    if (mode == 'i')
		exit(0);
	    break;
	case 'x':
	    if (*++s == '=') s++;
	    if (isdigit(*s)) {
		set_option(OI_ARTICLE_TREE_LINES, s);
		while (isdigit(*s)) s++;
	    }
	    if (*s)
		set_option(OI_NEWS_SEL_STYLES, s);
	    set_option(OI_USE_THREADS, YESorNO(upordown));
	    break;
	case 'X':
	    if (*++s == '=') s++;
	    if (isdigit(*s)) {
		set_option(OI_USE_NEWS_SEL, s);
		while (isdigit(*s)) s++;
	    }
	    else
		set_option(OI_USE_NEWS_SEL, YESorNO(upordown));
	    if (*s)
		set_option(OI_NEWS_SEL_CMDS, s);
	    break;
	case 'z':
	    if (*++s == '=') s++;
	    set_option(OI_DEFAULT_REFETCH_TIME,
		       upordown && *s? s : YESorNO(upordown));
	    break;
	default:
#ifdef VERBOSE
	    IF(verbose)
		printf("\nIgnoring unrecognized switch: -%c\n", *s) FLUSH;
	    ELSE
#endif
#ifdef TERSE
		printf("\nIgnoring -%c\n", *s) FLUSH;
#endif
	    termdown(2);
	    break;
	}
    }
}

void
save_init_environment(str)
char* str;
{
    if (init_environment_cnt >= init_environment_max) {
	init_environment_max += 32;
	init_environment_strings = (char**)
	  saferealloc((char*)init_environment_strings,
		      init_environment_max * sizeof (char*));
    }
    init_environment_strings[init_environment_cnt++] = str;
}

void
write_init_environment(fp)
FILE* fp;
{
    int i;
    char* s;
    for (i = 0; i < init_environment_cnt; i++) {
	s = index(init_environment_strings[i],'=');
	if (!s)
	    continue;
	*s = '\0';
	fprintf(fp, "%s=%s\n", init_environment_strings[i],quote_string(s+1));
	*s = '=';
    }
    init_environment_cnt = init_environment_max = 0;
    free((char*)init_environment_strings);
    init_environment_strings = NULL;
}
