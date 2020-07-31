/* This file Copyright 1992, 1993 by Clifford A. Adams */
/* scorefile.c
 *
 * A simple "proof of concept" scoring file for headers.
 * (yeah, right. :)
 */

#include "EXTERN.h"
#include "common.h"
#ifdef SCORE
/* if SCORE is undefined, no code should be compiled */
#include "list.h"
#include "hash.h"
#include "cache.h"
#include "bits.h"		/* absfirst */
#include "head.h"
#include "search.h"		/* regex matches */
#include "ngdata.h"
#include "ng.h"
#include "term.h"		/* finish_command() */
#include "util.h"
#include "util2.h"
#include "env.h"		/* getval */
#include "rt-util.h"
#include "mempool.h"
#include "score.h"		/* shared stuff... */
#ifdef SCAN_ART
#include "scanart.h"
#include "samain.h"		/* for sa_authscored macro */
#endif
#include "url.h"
#include "INTERN.h"
#include "scorefile.h"
#include "scorefile.ih"

/* list of score array markers (in htype field of score entry) */
    /* entry is a file marker.  Score is the file level */
#define SF_FILE_MARK_START (-1)
#define SF_FILE_MARK_END (-2)
/* other misc. rules */
#define SF_KILLTHRESHOLD (-3)
#define SF_NEWAUTHOR (-4)
#define SF_REPLY (-5)

static int sf_file_level INIT(0);	/* how deep are we? */

static char sf_buf[LBUFLEN];

static char** sf_extra_headers = NULL;
static int sf_num_extra_headers = 0;

static bool sf_has_extra_headers;

/* Must be called before any other sf_ routine (once for each group) */
void
sf_init()
{
    int i;
    char* s;
    int level;	/* depth of newsgroup score file */

    sf_num_entries = 0;
    level = 0;
    sf_extra_headers = NULL;
    sf_num_extra_headers = 0;

    /* initialize abbreviation list */
    sf_abbr = (char**)safemalloc(256 * sizeof (char*));
    bzero((char*)sf_abbr, 256 * sizeof (char*));

    if (sf_verbose)
	printf("\nReading score files...\n") FLUSH;
    sf_file_level = 0;
    /* find # of levels */
    strcpy(sf_buf,filexp("%C"));
    level = 0;
    for (s = sf_buf; *s; s++)
	if (*s == '.')
	    level++;		/* count dots in group name */
    level++;

    /* the main read-in loop */
    for (i = 0; i <= level; i++)
	if ((s = sf_get_filename(i)) != NULL)
	    sf_do_file(s);

    /* do post-processing (set thresholds and detect extra header usage) */
    sf_has_extra_headers = FALSE;
    /* set thresholds from the sf_entries */
    reply_active = newauthor_active = kill_thresh_active = FALSE;
    for (i = 0; i < sf_num_entries; i++) {
	if (sf_entries[i].head_type >= HEAD_LAST)
	    sf_has_extra_headers = TRUE;
	switch (sf_entries[i].head_type) {
	  case SF_KILLTHRESHOLD:
	    kill_thresh_active = TRUE;
	    kill_thresh = sf_entries[i].score;
	    if (sf_verbose) {
		int j;
		/* rethink? */
		for (j = i+1; j < sf_num_entries; j++)
		    if (sf_entries[j].head_type == SF_KILLTHRESHOLD)
			break;
		if (j == sf_num_entries) /* no later thresholds */
		    printf("killthreshold %d\n",kill_thresh) FLUSH;
	    }
	    break;
	  case SF_NEWAUTHOR:
	    newauthor_active = TRUE;
	    newauthor = sf_entries[i].score;
	    if (sf_verbose) {
		int j;
		/* rethink? */
		for (j = i+1; j < sf_num_entries; j++)
		    if (sf_entries[j].head_type == SF_NEWAUTHOR)
			break;
		if (j == sf_num_entries) /* no later newauthors */
		    printf("New Author score: %d\n",newauthor) FLUSH;
	    }
	    break;
	  case SF_REPLY:
	    reply_active = TRUE;
	    reply_score = sf_entries[i].score;
	    if (sf_verbose) {
		int j;
		/* rethink? */
		for (j = i+1; j < sf_num_entries; j++)
		    if (sf_entries[j].head_type == SF_REPLY)
			break;
		if (j == sf_num_entries) /* no later reply rules */
		    printf("Reply score: %d\n",reply_score) FLUSH;
	    }
	    break;
	}
    }
}

void
sf_clean()
{
    int i;

    for (i = 0; i < sf_num_entries; i++) {
	if (sf_entries[i].compex != NULL) {
	    free_compex(sf_entries[i].compex);
	    free(sf_entries[i].compex);
	}
    }
    mp_free(MP_SCORE1);		/* free memory pool */
    if (sf_abbr) {
	for (i = 0; i < 256; i++)
	    if (sf_abbr[i]) {
		free(sf_abbr[i]);
		sf_abbr[i] = NULL;
	    }
	free(sf_abbr);
    }
    if (sf_entries)
	free(sf_entries);
    sf_entries = NULL;
    for (i = 0; i < sf_num_extra_headers; i++)
	free(sf_extra_headers[i]);
    sf_num_extra_headers = 0;
    sf_extra_headers = NULL;
}

/* rename sf_num_entries (to ?) */
/* use macro instead of all the "sf_entries[sf_num_entries-1]"?
 * call it "sf_recent_entry" or "sf_last_entry"?
 */
void
sf_grow()
{
    int i;

    sf_num_entries++;
    if (sf_num_entries == 1) {
	sf_entries = (SF_ENTRY*)safemalloc(sizeof (SF_ENTRY));
    } else {
	sf_entries = (SF_ENTRY*)saferealloc((char*)sf_entries,
			sf_num_entries * sizeof (SF_ENTRY));
    }
    i = sf_num_entries-1;
    sf_entries[i].compex = NULL;	/* init */
    sf_entries[i].flags = 0;
    sf_entries[i].str1 = NULL;
    sf_entries[i].str2 = NULL;
}

/* Returns -1 if no matching extra header found, otherwise returns offset
 * into the sf_extra_headers array.
 */
int
sf_check_extra_headers(head)
char* head;		/* header name, (without ':' character) */
{
    int i;
    char* s;
    static char lbuf[LBUFLEN];

    /* convert to lower case */
    safecpy(lbuf,head,sizeof lbuf - 1);
    for (s = lbuf; *s; s++) {
	if (isalpha(*s) && isupper(*s))
	    *s = tolower(*s);		/* convert to lower case */
    }
    for (i = 0; i < sf_num_extra_headers; i++) {
	if (strEQ(sf_extra_headers[i],lbuf))
	    return i;
    }
    return -1;
}

/* adds the header to the list of known extra headers if it is not already
 * known.
 */
void
sf_add_extra_header(head)
char* head;		/* new header name, (without ':' character) */
{
    static char lbuf[LBUFLEN];		/* ick. */
    int len;
    char* colonptr;	/* points to ':' character */
    char* s;
    char* s2;

    /* check to see if it's already known */
    /* first see if it is a known system header */
    safecpy(lbuf,head,sizeof lbuf - 2);
    len = strlen(lbuf);
    lbuf[len] = ':';
    lbuf[len+1] = '\0';
    colonptr = lbuf+len;
    if (set_line_type(lbuf,colonptr) != SOME_LINE)
	return;		/* known types should be interpreted in normal way */
    /* then check to see if it's a known extra header */
    if (sf_check_extra_headers(head) >= 0)
	return;

    sf_num_extra_headers++;
    sf_extra_headers = (char**)saferealloc((char*)sf_extra_headers,
	sf_num_extra_headers * sizeof (char*));
    s = savestr(head);
    for (s2 = s; *s2; s2++) {
	if (isalpha(*s2) && isupper(*s2))
	    *s2 = tolower(*s2);		/* convert to lower case */
    }
    sf_extra_headers[sf_num_extra_headers-1] = s;
}

char*
sf_get_extra_header(art,hnum)
ART_NUM art;		/* article number to check */
int hnum;		/* header number: offset into sf_extra_headers */
{
    char* s;
    char* head;		/* header text */
    int len;		/* length of header */
    static char lbuf[LBUFLEN];

    parseheader(art);	/* fast if already parsed */

    head = sf_extra_headers[hnum];
    len = strlen(head);

    for (s = headbuf; s && *s && *s != '\n'; s++) {
	if (strncaseEQ(head,s,len)) {
	    s = index(s,':');
	    if (!s)
		return nullstr;
	    s++;	/* skip the colon */
	    while (*s == ' ' || *s == '\t') s++;
	    if (!*s)
		return nullstr;
	    head = s;		/* now point to start of new text */
	    s = index(s,'\n');
	    if (!s)
		return nullstr;
	    *s = '\0';
	    safecpy(lbuf,head,sizeof lbuf - 1);
	    *s = '\n';
	    return lbuf;
	}
	s = index(s,'\n');	/* '\n' will be skipped on loop increment */
    }
    return nullstr;
}

/* move to util.c ? */
/* Returns TRUE if text pointed to by s is a text representation of
 * the number 0.  Used for error checking.
 * Note: does not check for trailing garbage ("+00kjsdfk" returns TRUE).
 */
bool
is_text_zero(s)
char* s;
{
    return *s == '0' || ((*s == '+' || *s == '-') && s[1]=='0');
}

/* keep this one outside the functions because it is shared */
static char sf_file[LBUFLEN];

/* filenames of type a/b/c/foo.bar.misc for group foo.bar.misc */
char*
sf_get_filename(level)
int level;
{
    char* s;

    strcpy(sf_file,filexp(getval("SCOREDIR",DEFAULT_SCOREDIR)));
    strcat(sf_file,"/");
#ifdef SHORTSCORENAMES
    strcat(sf_file,filexp("%C"));

    s1 = rindex(sf_file,'/');	/* find last slash in filename */
    if (!level)
	*s1 = '\0';	/* cut off slash for global */
    i = level;
    while (i--) {
	*s1++ = '/';
	while (*s1 != '.' && *s1 != '\0') s1++;
	if (*s1 == '\0' && i > 0)	/* not enough levels exist */
	    return NULL;	/* get_file2 wouldn't work either... */
	*s1 = '\0';
    }
    strcat(sf_file,"/SCORE");
#else /* !SHORTSCORENAMES */
    if (!level) {
	/* allow environment variable later... */
	strcat(sf_file,"global");
    } else {
	strcat(sf_file,filexp("%C"));
	s = rindex(sf_file,'/');
	/* maybe redo this logic later... */
	while (level--) {
	    if (*s == '\0')	/* no more name to match */
		return NULL;
	    while (*s && *s != '.') s++;
	    if (*s && level)
		s++;
	}
	*s = '\0';	/* cut end of score file */
    }
#endif /* SHORTSCORENAMES */
    return sf_file;
}

/* given a string, if no slashes prepends SCOREDIR env. variable */
char*
sf_cmd_fname(s)
char* s;
{
    static char lbuf[LBUFLEN];
    char* s1;

    s1 = index(s,'/');
    if (s1)
	return s;
    /* no slashes in this filename */
    strcpy(lbuf,getval("SCOREDIR",DEFAULT_SCOREDIR));
    strcat(lbuf,"/");
    strcat(lbuf,s);
    return lbuf;
}

/* returns TRUE if good command, FALSE otherwise */
bool
sf_do_command(cmd,check)
char* cmd;		/* text of command */
bool_int check;		/* if TRUE, just check, don't execute */
{
    char* s;
    int i;
    char ch;

    if (strnEQ(cmd,"killthreshold",13)) {
	/* skip whitespace and = sign */
	for (s = cmd+13; *s && (*s == ' ' || *s == '\t' || *s == '='); s++) ;

	/* make **sure** that there is a number here */
	i = atoi(s);
	if (i == 0)		/* it might not be a number */
	    if (!is_text_zero(s)) {
		printf("\nBad killthreshold: %s",cmd);
		return FALSE;	/* continue looping */
	    }
	if (check)
	    return TRUE;
	sf_grow();
	sf_entries[sf_num_entries-1].head_type = SF_KILLTHRESHOLD;
	sf_entries[sf_num_entries-1].score = i;
	return TRUE;
    }
    if (strnEQ(cmd,"savescores",10)) {
	/* skip whitespace and = sign */
	for (s = cmd+10; *s && (*s == ' ' || *s == '\t' || *s == '='); s++) ; 
	if (strnEQ(s,"off",3)) {
	    if (!check)
		sc_savescores = FALSE;
	    return TRUE;
	}
	if (*s) {	/* there is some argument */
	    if (check)
		return TRUE;
	    sc_savescores = TRUE;
	    return TRUE;
	}
	printf("Bad savescores command: |%s|\n",cmd) FLUSH;
	return FALSE;
    }
    if (strnEQ(cmd,"newauthor",9)) {
	/* skip whitespace and = sign */
	for (s = cmd+9; *s && (*s == ' ' || *s == '\t' || *s == '='); s++) ;

	/* make **sure** that there is a number here */
	i = atoi(s);
	if (i == 0)		/* it might not be a number */
	    if (!is_text_zero(s)) {
		printf("\nBad newauthor: %s",cmd);
		return FALSE;	/* continue looping */
	    }
	if (check)
	    return TRUE;
	sf_grow();
	sf_entries[sf_num_entries-1].head_type = SF_NEWAUTHOR;
	sf_entries[sf_num_entries-1].score = i;
	return TRUE;
    }
    if (strnEQ(cmd,"include",7)) {
	if (check)
	    return TRUE;
	s = cmd+7;
	while (*s == ' ' || *s == '\t') s++;	/* skip whitespace */
	if (!*s) {
	    printf("Bad include command (missing filename)\n");
	    return FALSE;
	}
	sf_do_file(filexp(sf_cmd_fname(s)));
	return TRUE;
    }
    if (strnEQ(cmd,"exclude",7)) {
	if (check)
	    return TRUE;
	s = cmd+7;
	while (*s == ' ' || *s == '\t') s++;	/* skip whitespace */
	if (!*s) {
	    printf("Bad exclude command (missing filename)\n");
	    return FALSE;
	}
	sf_exclude_file(filexp(sf_cmd_fname(s)));
	return TRUE;
    }
    if (strnEQ(cmd,"header",6)) {
	char* s2;

	s = cmd+7;
	while (*s == ' ' || *s == '\t') s++;	/* skip whitespace */
	for (s2 = s; *s2 && *s2 != ':'; s2++) ;
	if (!s2) {
	    printf("\nBad header command (missing :)\n%s\n",cmd) FLUSH;
	    return FALSE;
	}
	if (check)
	    return TRUE;
	*s2 = '\0';
	sf_add_extra_header(s);
	*s2 = ':';
	return TRUE;
    }
    if (strnEQ(cmd,"begin",5)) {
	s = cmd+6;
	while (*s == ' ' || *s == '\t') s++;	/* skip whitespace */
	if (strnEQ(s,"score",5)) {
	    /* do something useful later */
	    return TRUE;
	}
	return TRUE;
    }
    if (strnEQ(cmd,"reply",5)) {
	/* skip whitespace and = sign */
	for (s = cmd+5; *s && (*s == ' ' || *s == '\t' || *s == '='); s++) ;

	/* make **sure** that there is a number here */
	i = atoi(s);
	if (i == 0)		/* it might not be a number */
	    if (!is_text_zero(s)) {
		printf("\nBad reply command: %s\n",cmd);
		return FALSE;	/* continue looping */
	    }
	if (check)
	    return TRUE;
	sf_grow();
	sf_entries[sf_num_entries-1].head_type = SF_REPLY;
	sf_entries[sf_num_entries-1].score = i;
	return TRUE;
    }
    if (strnEQ(cmd,"file",4)) {
	if (check)
	    return TRUE;
	s = cmd+4;
	while (*s == ' ' || *s == '\t') s++;	/* skip whitespace */
	if (!*s) {
	    printf("Bad file command (missing parameters)\n");
	    return FALSE;
	}
	ch = *s++;
	while ((*s == ' ') || (*s == '\t'))
	    s++;			/* skip whitespace */
	if (!*s) {
	    printf("Bad file command (missing parameters)\n");
	    return FALSE;
	}
	if (sf_abbr[(int)ch])
	    free(sf_abbr[(int)ch]);
	sf_abbr[(int)ch] = savestr(sf_cmd_fname(s));
	return TRUE;
    }
    if (strnEQ(cmd,"end",3)) {
	s = cmd+4;
	while (*s == ' ' || *s == '\t') s++;	/* skip whitespace */
	if (strnEQ(s,"score",5)) {
	    /* do something useful later */
	    return TRUE;
	}
	return TRUE;
    }
    if (strnEQ(cmd,"newsclip",8)) {
	printf("Newsclip is no longer supported.\n") FLUSH;
	return FALSE;
    }
    /* no command matched */
    printf("Unknown command: |%s|\n",cmd) FLUSH;
    return FALSE;
}

COMPEX* sf_compex INIT(NULL);

char*
sf_freeform(start1,end1)
char* start1;		/* points to first character of keyword */
char* end1;		/* points to last  character of keyword */
{
    char* s;
    bool error;
    char ch;

    error = FALSE;	/* be optimistic :-) */
    /* cases are # of letters in keyword */
    switch (end1-start1+1) {
      case 7:
	if (strnEQ(start1,"pattern",7)) {
	    sf_pattern_status = TRUE;
	    break;
	}
	error = TRUE;
	break;
      case 4:
#ifdef UNDEF
/* here is an example of a hypothetical freeform key with an argument */
	if (strnEQ(start1,"date",4)) {
	    char* s1;
	    int datenum;
	    /* skip whitespace and = sign */
	    for (s = end1+1; *s && (*s == ' ' || *s == '\t'); s++) ;
	    if (!*s) {	/* ran out of line */
		printf("freeform: date keyword: ran out of input\n");
		return s;
	    }
	    datenum = atoi(s);
	    printf("Date: %d\n",datenum) FLUSH;
	    while (isdigit(*s)) s++;	/* skip datenum */
	    end1 = s;		/* end of key data */
	    break;
	}
#endif
	error = TRUE;
	break;
      default:
	error = TRUE;
	break;
    }
    if (error) {
	s = end1+1;
	ch = *s;
	*s = '\0';
	printf("Scorefile freeform: unknown key: |%s|\n",start1) FLUSH;
	*s = ch;
	return NULL;	/* error indicated */
    }
    /* no error, so skip whitespace at end of key */
    for (s = end1+1; *s && (*s == ' ' || *s == '\t'); s++) ;
    return s;
}

bool
sf_do_line(line,check)
char* line;
bool_int check;		/* if TRUE, just check the line, don't act. */
{
    char ch;
    char* s;
    char* s2;
    int i,j;

    if (!line || !*line)
	return TRUE;		/* very empty line */
    s = line + strlen(line) - 1;
    if (*s == '\n')
	*s = '\0';		/* kill the newline */

    ch = line[0];
    if (ch == '#')		/* comment */
	return TRUE;

    /* reset any per-line bitflags */
    sf_pattern_status = FALSE;

    if (isalpha(ch))		/* command line */
	return sf_do_command(line,check);

    /* skip whitespace */
    for (s = line; *s && (*s == ' ' || *s == '\t'); s++) ;
    if (!*s || *s == '#')
	return TRUE;	/* line was whitespace or comment after whitespace */
    /* convert line to lowercase (make optional later?) */
    for (s2 = s; *s2 != '\0'; s2++) {
	if (isupper(*s2))
	    *s2 = tolower(*s2);		/* convert to lower case */
    }
    i = atoi(s);
    if (i == 0)	{	/* it might not be a number */
	if (!is_text_zero(s)) {
	    printf("\nBad scorefile line:\n|%s|\n",s);
	    return FALSE;
	}
    }
    /* add the line as a scoring entry */
    while (isdigit(*s) || *s == '+' || *s == '-' || *s == ' ' || *s == '\t')
	s++;	/* skip score */
    while (TRUE) {
	for (s2 = s; *s2 && !(*s2 == ' ' || *s2 == '\t'); s2++) ;
	s2--;
	if (*s2 == ':')	/* did header */
	    break;	/* go to set header routine */
	s = sf_freeform(s,s2);
	if (!s || !*s) {	/* used up all the line's text, or error */
	    printf("Scorefile entry error error (freeform parse).  ");
	    printf("Line was:\n|%s|\n",line) FLUSH;
	    return FALSE;	/* error */
	}
	s2 = s;
    } /* while */
    /* s is start of header name, s2 points to the ':' character */
    j = set_line_type(s,s2);
    if (j == SOME_LINE) {
	*s2 = '\0';
	j = sf_check_extra_headers(s);
	*s2 = ':';
	if (j >= 0)
	    j += HEAD_LAST;
	else {
	    printf("Unknown score header type.  Line follows:\n|%s|\n",line);
	    return FALSE;
	}
    }
    /* skip whitespace */
    for (s = ++s2; *s && (*s == ' ' || *s == '\t'); s++) ;
    if (!*s) {	/* no pattern */
	printf("Empty score pattern.  Line follows:\n|%s|\n",line) FLUSH;
	return FALSE;
    }
    if (check)
	return TRUE;		/* limits of check */
    sf_grow();		/* acutally make an entry */
    sf_entries[sf_num_entries-1].head_type = j;
    sf_entries[sf_num_entries-1].score = i;
    if (sf_pattern_status) {	/* in pattern matching mode */
	sf_entries[sf_num_entries-1].flags |= 1;
	sf_entries[sf_num_entries-1].str1 = mp_savestr(s,MP_SCORE1);
	sf_compex = (COMPEX*)safemalloc(sizeof (COMPEX));
	init_compex(sf_compex);
	/* compile arguments: */
	/* 1st is COMPEX to store compiled regex in */
	/* 2nd is search string */
	/* 3rd should be TRUE if the search string is a regex */
	/* 4th is TRUE for case-insensitivity */
	s2 = compile(sf_compex,s,TRUE,TRUE);
	if (s2 != NULL) {
	    printf("Bad pattern : |%s|\n",s) FLUSH;
	    printf("Compex returns: |%s|\n",s2) FLUSH;
	    free_compex(sf_compex);
	    free(sf_compex);
	    sf_entries[sf_num_entries-1].compex = NULL;
	    return FALSE;
	} else
	    sf_entries[sf_num_entries-1].compex = sf_compex;
    }
    else {
	sf_entries[sf_num_entries-1].flags &= 0xfe;
	sf_entries[sf_num_entries-1].str2 = NULL;
	/* Note: consider allowing * wildcard on other header filenames */
	if (j == FROM_LINE) {	/* may have * wildcard */
	    if ((s2 = index(s,'*')) != NULL) {
		sf_entries[sf_num_entries-1].str2 = mp_savestr(s2+1,MP_SCORE1);
		*s2 = '\0';
	    }
	}
	sf_entries[sf_num_entries-1].str1 = mp_savestr(s,MP_SCORE1);
    }
    return TRUE;
}

void
sf_do_file(fname)
char* fname;
{
    char* s;
    int sf_fp;
    int i;
    char* safefilename;

#ifdef SCOREFILE_CACHE
    sf_fp = sf_open_file(fname);
    if (sf_fp < 0)
	return;
#else
    fp = fopen(fname,"r");
    if (!fp)
	return;
#endif
    sf_file_level++;
    if (sf_verbose) {
	for (i = 1; i < sf_file_level; i++)
	    printf(".");		/* maybe later putchar... */
	printf("Score file: %s\n",fname) FLUSH;
    }
    safefilename = savestr(fname);
    /* add end marker to scoring array */
    sf_grow();
    sf_entries[sf_num_entries-1].head_type = SF_FILE_MARK_START;
    /* file_level is 1 to n */
    sf_entries[sf_num_entries-1].score = sf_file_level;
    sf_entries[sf_num_entries-1].str2 = NULL;
    sf_entries[sf_num_entries-1].str1 = savestr(safefilename);

#ifdef SCOREFILE_CACHE
    while ((s = sf_file_getline(sf_fp)) != NULL) {
	strcpy(sf_buf,s);
	s = sf_buf;
#else
    while ((s = fgets(sf_buf,1020,fp)) != NULL) { /* consider buffer size */
#endif
	(void)sf_do_line(s,FALSE);
    }
#ifndef SCOREFILE_CACHE
    fclose(fp);
#endif
    /* add end marker to scoring array */
    sf_grow();
    sf_entries[sf_num_entries-1].head_type = SF_FILE_MARK_END;
    /* file_level is 1 to n */
    sf_entries[sf_num_entries-1].score = sf_file_level;
    sf_entries[sf_num_entries-1].str2 = NULL;
    sf_entries[sf_num_entries-1].str1 = savestr(safefilename);
    free(safefilename);
    sf_file_level--;
}

int
score_match(str,ind)
char* str;		/* string to match on */
int ind;		/* index into sf_entries */
{
    char* s1;
    char* s2;
    char* s3;

    s1 = sf_entries[ind].str1;
    s2 = sf_entries[ind].str2;

    if (sf_entries[ind].flags & 1) {	/* pattern style match */
	if (sf_entries[ind].compex != NULL) {
	/* we have a good pattern */
	    s2 = execute(sf_entries[ind].compex,str);
	    if (s2 != NULL)
		return TRUE;
	}
	return FALSE;
    }
    /* default case */
    if ((s3 = STRSTR(str,s1)) != NULL && (!s2 || STRSTR(s3+strlen(s1),s2)))
	return TRUE;
    return FALSE;
}

int
sf_score(a)
ART_NUM a;
{
    int sum,i,j;
    int h;		/* header type */
    char* s;		/* misc */
    bool old_untrim;	/* old value of untrim_cache */

    if (is_unavailable(a))
	return LOWSCORE;	/* unavailable arts get low negative score. */

    /* if there are no score entries, then the answer is real easy and quick */
    if (sf_num_entries == 0)
	return 0;
    old_untrim = untrim_cache;
    untrim_cache = TRUE;
    sc_scoring = TRUE;		/* loop prevention */
    sum = 0;

    /* parse the header now if there are extra headers */
    /* (This could save disk accesses.) */
    if (sf_has_extra_headers)
	parseheader(a);

    for (i = 0; i < sf_num_entries; i++) {
	h = sf_entries[i].head_type;
	if (h <= 0)	/* don't use command headers for scoring */
	    continue;	/* the outer for loop */
	/* if this head_type has been done before, this entry
	   has already been done */
	if (sf_entries[i].flags & 2) {	/* rule has been applied */
	    sf_entries[i].flags &= 0xfd; /* turn off flag */
	    continue;			/* ...with the next rule */
	}

	/* sf_get_line will return ptr to buffer (already lowercased string) */
	s = sf_get_line(a,h);
	if (!s || !*s)	/* no such line for the article */
	    continue;	/* with the sf_entries. */

	/* do the matches for this header */
	for (j = i; j < sf_num_entries; j++) {
	    /* see if there is a match */
	    if (h == sf_entries[j].head_type) {
		if (j != i) {		/* set flag only for future rules */
		    sf_entries[j].flags |= 2; /* rule has been applied. */
		}
		if (score_match(s,j)) {
		    sum = sum + sf_entries[j].score;
		    if (h == FROM_LINE)
			article_ptr(a)->scoreflags |= SFLAG_AUTHOR;
		    if (sf_score_verbose)
			sf_print_match(j);
		}
	    }
	}
    }
    if (newauthor_active && !(article_ptr(a)->scoreflags & SFLAG_AUTHOR)) {
	sum = sum+newauthor;	/* add new author bonus */
	if (sf_score_verbose) {
	    printf("New Author: %d\n",newauthor) FLUSH;
	    /* consider: print which file the bonus came from */
	}
    }
    if (reply_active) {
	/* should be in cache if a rule above used the subject */
	s = fetchcache(a, SUBJ_LINE, TRUE);
	/* later: consider other possible reply forms (threading?) */
	if (s && subject_has_Re(s,(char**)NULL)) {
	    sum = sum+reply_score;
	    if (sf_score_verbose) {
		printf("Reply: %d\n",reply_score);
		/* consider: print which file the bonus came from */
	    }
	}
    }
    untrim_cache = old_untrim;
    sc_scoring = FALSE;
    return sum;
}

/* returns changed score line or NULL if no changes */
char*
sf_missing_score(line)
char* line;
{
    static char lbuf[LBUFLEN];
    int i;
    char* s;

    /* save line since it is probably pointing at (the TRN-global) buf */
    s = savestr(line);
    printf("Possibly missing score.\n\
Type a score now or delete the colon to abort this entry:\n") FLUSH;
    buf[0] = ':';
    buf[1] = FINISHCMD;
    i = finish_command(TRUE);	/* print the CR */
    if (!i) { /* there was no score */
	free(s);
	return NULL;
    }
    strcpy(lbuf,buf+1);
    i = strlen(lbuf);
    lbuf[i] = ' ';
    lbuf[i+1] = '\0';
    strcat(lbuf,s);
    free(s);
    return lbuf;
}

/* Interprets the '\"' command for creating new score entries online */
/* consider using some external buffer rather than the 2 internal ones */
void
sf_append(line)
char* line;
{
    char* scoreline;	/* full line to add to scorefile */
    char* scoretext;	/* text after the score# */
    char filechar;	/* filename character from line */
    char* filename;	/* expanded filename */
    static char filebuf[LBUFLEN];
    FILE* fp;
    char ch;		/* misc */
    char* s;

    if (!line)
	return;		/* do nothing with empty string */

    filechar = *line;		/* ch is file abbreviation */

    if (filechar == '?') {	/* list known file abbreviations */
	int i;

	printf("List of abbreviation/file pairs\n") ;
	for (i = 0; i < 256; i++)
	    if (sf_abbr[i])
		printf("%c %s\n",(char)i,sf_abbr[i]) FLUSH;
	printf("\" [The current newsgroup's score file]\n") FLUSH;
	printf("* [The global score file]\n") FLUSH;
	return;
    }

    /* skip whitespace after filechar */
    scoreline = line+1;
    while (*scoreline == ' ' || *scoreline == '\t') scoreline++;

    ch = *scoreline;	/* first non-whitespace after filechar */
    /* If the scorefile line does not begin with a number,
       and is not a valid command, request a score */
    if (!isdigit(ch) && ch != '+' && ch != '-' && ch != ':' && ch != '!'
     && ch != '#') {
	if (!sf_do_line(scoreline,TRUE)) {  /* just checking */
	    scoreline = sf_missing_score(scoreline);
	    if (!scoreline) {	/* no score typed */
		printf("Score entry aborted.\n") FLUSH;
		return;
	    }
	}
    }

    /* scoretext = first non-whitespace after score# */
    for (scoretext = scoreline;
	 isdigit(*scoretext) || *scoretext == '+' || *scoretext == '-'
			     || *scoretext == ' ' || *scoretext == '\t';
	 scoretext++) {
	;
    }
    /* special one-character shortcuts */
    if (*scoretext && scoretext[1] == '\0') {
	static char lbuf[LBUFLEN];
	switch(*scoretext) {
	  case 'F':	/* domain-shortened FROM line */
	    strcpy(lbuf,scoreline);
	    lbuf[strlen(lbuf)-1] = '\0';
	    strcat(lbuf,filexp("from: %y"));
	    scoreline = lbuf;
	    break;
	  case 'S':	/* current subject */
	    strcpy(lbuf,scoreline);
	    s = fetchcache(art,SUBJ_LINE,TRUE);
	    if (!s || !*s) {
		printf("No subject: score entry aborted.\n");
		return;
	    }
	    if (s[0] == 'R' && s[1] == 'e' && s[2] == ':' && s[3] == ' ')
		s += 4;
	    /* change this next line if LBUFLEN changes */
	    sprintf(lbuf+(strlen(lbuf)-1),"subject: %.900s",s);
	    scoreline = lbuf;
	    break;
	  default:
	    printf("\nBad scorefile line: |%s| (not added)\n", line) FLUSH;
	    return;
	}
	printf("%s\n",scoreline) FLUSH;
    }

    /* test the scoring line unless filechar is '!' (meaning do it now) */
    if (!sf_do_line(scoreline,filechar!='!')) {
	printf("Bad score line (ignored)\n") FLUSH;
	return;
    }	
    if (filechar == '!')
	return;		/* don't actually append to file */
    if (filechar == '"') {	/* do local group */
/* Note: should probably be changed to use sf_ file functions */
	strcpy(filebuf,getval("SCOREDIR",DEFAULT_SCOREDIR));
#ifdef SHORTSCORENAMES
	strcat(filebuf,"/%c/SCORE");
#else
	strcat(filebuf,"/%C");
#endif
	filename = filebuf;
    }
    else if (filechar == '*') {	/* do global scorefile */
/* Note: should probably be changed to use sf_ file functions */
	strcpy(filebuf,getval("SCOREDIR",DEFAULT_SCOREDIR));
	strcat(filebuf,"/global");
	filename = filebuf;
    }
    else if (!(filename = sf_abbr[(int)filechar])) {
	printf("\nBad file abbreviation: %c\n",filechar) FLUSH;
	return;
    }
    filename = filexp(sf_cmd_fname(filename));	/* allow shortcuts */
    /* make sure directory exists... */
    makedir(filename,MD_FILE);
#ifdef SCOREFILE_CACHE
    sf_file_clear();
#endif
    if ((fp = fopen(filename,"a")) != NULL) { /* open (or create) for append */
	fprintf(fp,"%s\n",scoreline);
	fclose(fp);
    }
    else				/* unsuccessful in opening file */
	printf("\nCould not open (for append) file %s\n",filename);
    return;
}

/* returns a lowercased copy of the header line type h in private buffer */
char*
sf_get_line(a,h)
ART_NUM a;
int h;
{
    static char sf_getline[LBUFLEN];
    char* s;

    if (h <= SOME_LINE) {
	printf("sf_get_line(%d,%d): bad header type\n",(int)a,h) FLUSH;
	printf("(Internal error: header number too low)\n") FLUSH;
	*sf_getline = '\0';
	return sf_getline;
    }
    if (h >= HEAD_LAST) {
	if (h-HEAD_LAST < sf_num_extra_headers)
	    s = sf_get_extra_header(a,h-HEAD_LAST);
	else {
	    printf("sf_get_line(%d,%d): bad header type\n",(int)a,h) FLUSH;
	    printf("(Internal error: header number too high)\n") FLUSH;
	    *sf_getline = '\0';
	    return sf_getline;
	}
    } else if (h == SUBJ_LINE)
	s = fetchcache(a,h,TRUE);	/* get compressed copy */
    else
	s = prefetchlines(a,h,FALSE);	/* don't make a copy */
    if (!s)
	*sf_getline = '\0';
    else
	safecpy(sf_getline,s,sizeof sf_getline - 1);

    for (s = sf_getline; *s; s++)
	if (isupper(*s))
	    *s = tolower(*s);
	*s = tolower(*s);
    return sf_getline;
}

/* given an index into sf_entries, print information about that index */
void
sf_print_match(indx)
int indx;
{
    int i,j,k;
    int level,tmplevel;		/* level is initialized iff used */
    char* head_name;
    char* pattern;

    for (i = indx; i >= 0; i--) {
	j = sf_entries[i].head_type;
	if (j == SF_FILE_MARK_START)  /* found immediate inclusion. */
	    break;
	if (j == SF_FILE_MARK_END) {	/* found included file, skip */
	    tmplevel = sf_entries[i].score;
	    for (k = i; k >= 0; k--) {
		if (sf_entries[k].head_type == SF_FILE_MARK_START
		 && sf_entries[k].score == tmplevel)
		    break;	/* inner for loop */
	    }
	    i = k;	/* will be decremented again */
	}
    }
    if (i >= 0)
	level = sf_entries[i].score;
    /* print the file markers. */
    for ( ; i >= 0; i--) {
	if (sf_entries[i].head_type == SF_FILE_MARK_START
	 && sf_entries[i].score <= level) {
	    level--;	/* go out... */
	    for (k = 0; k < level; k++)
		printf(".");		/* make putchar later? */
	    printf("From file: %s\n",sf_entries[i].str1);
	    if (level == 0)		/* top level */
		break;		/* out of the big for loop */
	}
    }
    if (sf_entries[indx].flags & 1)	/* regex type */
	pattern = "pattern ";
    else
	pattern = "";

    if (sf_entries[indx].head_type >= HEAD_LAST)
	head_name = sf_extra_headers[sf_entries[indx].head_type-HEAD_LAST];
    else
	head_name = htype[sf_entries[indx].head_type].name;
    printf("%d %s%s: %s", sf_entries[indx].score,pattern,head_name,
	   sf_entries[indx].str1);
    if (sf_entries[indx].str2)
	printf("*%s",sf_entries[indx].str2);
    printf("\n");
}

void
sf_exclude_file(fname)
char* fname;
{
    int start,end;
    int newnum;
    SF_ENTRY* tmp_entries;

    for (start = 0; start < sf_num_entries; start++)
	if (sf_entries[start].head_type == SF_FILE_MARK_START
	 && strEQ(sf_entries[start].str1,fname))
	    break;
    if (start == sf_num_entries) {
	printf("Exclude: file |%s| was not included\n",fname) FLUSH;
	return;
    }
    for (end = start+1; end < sf_num_entries; end++)
	if (sf_entries[end].head_type==SF_FILE_MARK_END
	 && strEQ(sf_entries[end].str1,fname))
	    break;
    if (end == sf_num_entries) {
	printf("Exclude: file |%s| is incomplete at exclusion command\n",
		fname) FLUSH;
	/* insert more explanation later? */
	return;
    }

    newnum = sf_num_entries-(end-start)-1;
#ifdef UNDEF
    /* Deal with exclusion of all scorefile entries.
     * This cannot happen since the exclusion command has to be within a
     * file.  Code kept in case online exclusions allowed later.
     */
    if (newnum==0) {
	sf_num_entries = 0;
	free(sf_entries);
	sf_entries = NULL;
	return;
    }
#endif
    tmp_entries = (SF_ENTRY*)safemalloc(newnum*sizeof(SF_ENTRY));
    /* copy the parts into tmp_entries */
    if (start > 0)
	bcopy((char*)sf_entries,(char*)tmp_entries,start * sizeof (SF_ENTRY));
    if (end < sf_num_entries-1)
	bcopy((char*)(sf_entries+end+1), (char*)(tmp_entries+start),
		(sf_num_entries-end-1) * sizeof (SF_ENTRY));
    free(sf_entries);
    sf_entries = tmp_entries;
    sf_num_entries = newnum;
    if (sf_verbose)
	printf("Excluded file: %s\n",fname) FLUSH;
}

void
sf_edit_file(filespec)
char* filespec;		/* file abbrev. or name */
{
    char filebuf[LBUFLEN];	/* clean up buffers */
    char filechar;		/* which file to do? */
    char* fname_noexpand;	/* non-expanded filename */

    if (!filespec || !*filespec)
	return;		/* empty, do nothing (error later?) */
    filechar = *filespec;
    /* if more than one character use as filename */
    if (filespec[1])
	strcpy(filebuf,filespec);
    else if (filechar == '"') {	/* edit local group */
/* Note: should probably be changed to use sf_ file functions */
	strcpy(filebuf,getval("SCOREDIR",DEFAULT_SCOREDIR));
#ifdef SHORTSCORENAMES
	strcat(filebuf,"/%c/SCORE");
#else
	strcat(filebuf,"/%C");
#endif
    }
    else if (filechar == '*') {	/* edit global scorefile */
/* Note: should probably be changed to use sf_ file functions */
	strcpy(filebuf,getval("SCOREDIR",DEFAULT_SCOREDIR));
	strcat(filebuf,"/global");
    }
    else {	/* abbreviation */
	if (!sf_abbr[(int)filechar]) {
	    printf("\nBad file abbreviation: %c\n",filechar) FLUSH;
	    return;
	}
	strcpy(filebuf,sf_abbr[(int)filechar]);
    }
    fname_noexpand = sf_cmd_fname(filebuf);
    strcpy(filebuf,filexp(fname_noexpand));
    /* make sure directory exists... */
    if (makedir(filebuf,MD_FILE) == 0) {
	(void)edit_file(fname_noexpand);
#ifdef SCOREFILE_CACHE
	sf_file_clear();
#endif
    }
    else
	printf("Can't make %s\n",filebuf) FLUSH;
}

/* returns file number */
/* if file number is negative, the file does not exist or cannot be opened */
#ifdef SCOREFILE_CACHE
static int
sf_open_file(name)
char* name;
{
    FILE* fp;
    char* temp_name;
    char* s;
    int i;

    if (!name || !*name)
	return 0;	/* unable to open */
    for (i = 0; i < sf_num_files; i++)
	if (strEQ(sf_files[i].fname,name)) {
	    if (sf_files[i].num_lines < 0)	/* nonexistent */
		return -1;	/* no such file */
	    sf_files[i].line_on = 0;
	    return i;
	}
    sf_num_files++;
    sf_files = (SF_FILE*)saferealloc((char*)sf_files,
	sf_num_files * sizeof (SF_FILE));
    sf_files[i].fname = savestr(name);
    sf_files[i].num_lines = 0;
    sf_files[i].num_alloc = 0;
    sf_files[i].line_on = 0;
    sf_files[i].lines = NULL;

    temp_name = NULL;
    if (strncaseEQ(name,"URL:",4)) {
#ifdef USEURL
	char lbuf[1024];
	safecpy(lbuf,name,sizeof lbuf - 4);
	name = lbuf;
	temp_name = temp_filename();
	if (!url_get(name+4,temp_name))
	    name = NULL;
	else
	    name = temp_name;
#else
	printf("\nThis copy of strn does not have URL support.\n") FLUSH;
	name = NULL;
#endif
    }
    if (!name) {
	sf_files[i].num_lines = -1;
	return -1;
    }
    fp = fopen(name,"r");
    if (!fp) {
	sf_files[i].num_lines = -1;
	return -1;
    }
    while ((s = fgets(sf_buf,LBUFLEN-4,fp)) != NULL) {
	if (sf_files[i].num_lines >= sf_files[i].num_alloc) {
	    sf_files[i].num_alloc += 100;
	    sf_files[i].lines = (char**)saferealloc((char*)sf_files[i].lines,
		sf_files[i].num_alloc*sizeof(char**));
	}
	/* CAA: I kind of like the next line in a twisted sort of way. */
	sf_files[i].lines[sf_files[i].num_lines++] = mp_savestr(s,MP_SCORE2);
    }
    fclose(fp);
    if (temp_name)
	UNLINK(temp_name);
    return i;
}
#endif /* SCOREFILE_CACHE */

#ifdef SCOREFILE_CACHE
static void
sf_file_clear()
{
    int i;

    for (i = 0; i < sf_num_files; i++) {
	if (sf_files[i].fname)
	    free(sf_files[i].fname);
	if (sf_files[i].num_lines > 0) {
	    /* memory pool takes care of freeing line contents */
	    free(sf_files[i].lines);
	}
    }
    mp_free(MP_SCORE2);
    if (sf_files)
	free(sf_files);
    sf_files = (SF_FILE*)NULL;
    sf_num_files = 0;
}
#endif /* SCOREFILE_CACHE */

#ifdef SCOREFILE_CACHE
static char*
sf_file_getline(fnum)
int fnum;
{
    if (fnum < 0 || fnum >= sf_num_files)
	return NULL;
    if (sf_files[fnum].line_on >= sf_files[fnum].num_lines)
	return NULL;		/* past end of file, or empty file */
    /* below: one of the more twisted lines of my career  (:-) */
    return sf_files[fnum].lines[sf_files[fnum].line_on++];
}
#endif /* SCOREFILE_CACHE */
#endif /* SCORE */
