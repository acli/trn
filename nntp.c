/* nntp.c
*/
/* This software is copyrighted as detailed in the LICENSE file. */


#include "EXTERN.h"
#include "common.h"
#include "list.h"
#include "util.h"
#include "util2.h"
#include "init.h"
#include "trn.h"
#include "hash.h"
#include "ngdata.h"
#include "nntpclient.h"
#include "datasrc.h"
#include "INTERN.h"
#include "nntp.h"
#include "nntp.ih"
#include "EXTERN.h"
#include "rcln.h"
#include "cache.h"
#include "bits.h"
#include "head.h"
#include "term.h"
#include "final.h"
#include "artio.h"
#include "rcstuff.h"

#ifdef SUPPORT_NNTP

int
nntp_list(type, arg, len)
char* type;
char* arg;
int len;
{
    int ret;
#ifdef DEBUG /*$$*/
    if (len && (debug & 1) && strcaseEQ(type,"active"))
	return -1;
#endif
    if (len)
	sprintf(ser_line, "LIST %s %.*s", type, len, arg);
    else if (strcaseEQ(type,"active"))
	strcpy(ser_line, "LIST");
    else
	sprintf(ser_line, "LIST %s", type);
    if (nntp_command(ser_line) <= 0)
	return -2;
    if ((ret = nntp_check()) <= 0)
	return ret? ret : -1;
    if (!len)
	return 1;
    if ((ret = nntp_gets(ser_line, sizeof ser_line)) < 0)
	return ret;
#if defined(DEBUG) && defined(FLUSH)
    if (debug & DEB_NNTP)
	printf("<%s\n", ser_line) FLUSH;
#endif
    if (nntp_at_list_end(ser_line))
	return 0;
    return 1;
}

void
nntp_finish_list()
{
    int ret;
    do {
	while ((ret = nntp_gets(ser_line, sizeof ser_line)) == 0) {
	    /* A line w/o a newline is too long to be the end of the
	    ** list, so grab the rest of this line and try again. */
	    while ((ret = nntp_gets(ser_line, sizeof ser_line)) == 0)
		;
	    if (ret < 0)
		return;
	}
    } while (ret > 0 && !nntp_at_list_end(ser_line));
}

/* try to access the specified group */

int
nntp_group(group, gp)
char* group;
NGDATA* gp;
{
    sprintf(ser_line, "GROUP %s", group);
    if (nntp_command(ser_line) <= 0)
	return -2;
    switch (nntp_check()) {
      case -2:
	return -2;
      case -1:
      case 0: {
	int ser_int = atoi(ser_line);
	if (ser_int != NNTP_NOSUCHGROUP_VAL
	 && ser_int != NNTP_SYNTAX_VAL) {
	    if (ser_int != NNTP_AUTH_NEEDED_VAL && ser_int != NNTP_ACCESS_VAL
	     && ser_int != NNTP_AUTH_REJECT_VAL) {
		fprintf(stderr, "\nServer's response to GROUP %s:\n%s\n",
			group, ser_line);
		return -1;
	    }
	}
	return 0;
      }
    }
    if (gp) {
	long count, first, last;

	(void) sscanf(ser_line,"%*d%ld%ld%ld",&count,&first,&last);
	/* NNTP mangles the high/low values when no articles are present. */
	if (!count)
	    gp->abs1st = gp->ngmax+1;
	else {
	    gp->abs1st = (ART_NUM)first;
	    gp->ngmax = (ART_NUM)last;
	}
    }
    return 1;
}

/* check on an article's existence */

int
nntp_stat(artnum)
ART_NUM artnum;
{
    sprintf(ser_line, "STAT %ld", (long)artnum);
    if (nntp_command(ser_line) <= 0)
	return -2;
    return nntp_check();
}

/* check on an article's existence by its message id */

ART_NUM
nntp_stat_id(msgid)
char* msgid;
{
    long artnum;

    sprintf(ser_line, "STAT %s", msgid);
    if (nntp_command(ser_line) <= 0)
	return -2;
    artnum = nntp_check();
    if (artnum > 0 && sscanf(ser_line, "%*d%ld", &artnum) != 1)
	artnum = 0;
    return (ART_NUM)artnum;
}

ART_NUM
nntp_next_art()
{
    long artnum;

    if (nntp_command("NEXT") <= 0)
	return -2;
    artnum = nntp_check();
    if (artnum > 0 && sscanf(ser_line, "%*d %ld", &artnum) != 1)
	artnum = 0;
    return (ART_NUM)artnum;
}

/* prepare to get the header */

int
nntp_header(artnum)
ART_NUM artnum;
{
    sprintf(ser_line, "HEAD %ld", (long)artnum);
    if (nntp_command(ser_line) <= 0)
	return -2;
    return nntp_check();
}

/* copy the body of an article to a temporary file */

void
nntp_body(artnum)
ART_NUM artnum;
{
    char* artname;

    artname = nntp_artname(artnum, FALSE); /* Is it already in a tmp file? */
    if (artname) {
	if (body_pos >= 0)
	    nntp_finishbody(FB_DISCARD);
	artfp = fopen(artname,"r");
	if (artfp && fstat(fileno(artfp),&filestat) == 0)
	    body_end = filestat.st_size;
	return;
    }

    artname = nntp_artname(artnum, TRUE);   /* Allocate a tmp file */
    if (!(artfp = fopen(artname, "w+"))) {
	fprintf(stderr, "\nUnable to write temporary file: '%s'.\n",
		artname);
	finalize(1); /*$$*/
    }
#ifndef MSDOS
    chmod(artname, 0600);
#endif
    /*artio_setbuf(artfp);$$*/
    if (parsed_art == artnum)
	sprintf(ser_line, "BODY %ld", (long)artnum);
    else
	sprintf(ser_line, "ARTICLE %ld", (long)artnum);
    if (nntp_command(ser_line) <= 0)
	finalize(1); /*$$*/
    switch (nntp_check()) {
      case -2:
      case -1:
	finalize(1); /*$$*/
      case 0:
	fclose(artfp);
	artfp = NULL;
	errno = ENOENT;			/* Simulate file-not-found */
	return;
    }
    body_pos = 0;
    if (parsed_art == artnum) {
	fwrite(headbuf, 1, strlen(headbuf), artfp);
	htype[PAST_HEADER].minpos = body_end = (ART_POS)ftell(artfp);
    }
    else {
	char b[NNTP_STRLEN];
	ART_POS prev_pos = body_end = 0;
	while (nntp_copybody(b, sizeof b, body_end+1) > 0) {
	    if (*b == '\n' && body_end - prev_pos < sizeof b)
		break;
	    prev_pos = body_end;
	}
    }
    fseek(artfp, 0L, 0);
    nntplink.flags &= ~NNTP_NEW_CMD_OK;
}

long
nntp_artsize()
{
    return body_pos < 0 ? body_end : -1;
}

static int
nntp_copybody(s, limit, pos)
char* s;
int limit;
ART_POS pos;
{
    int len;
    bool had_nl = TRUE;
    int found_nl;

    while (pos > body_end || !had_nl) {
	found_nl = nntp_gets(s, limit);
	if (found_nl < 0)
	    strcpy(s,"."); /*$$*/
	if (had_nl) {
	    if (nntp_at_list_end(s)) {
		fseek(artfp, (long)body_pos, 0);
		body_pos = -1;
		return 0;
	    }
	    if (s[0] == '.')
		safecpy(s,s+1,limit);
	}
	len = strlen(s);
	if (found_nl)
	    strcpy(s+len, "\n");
	fputs(s, artfp);
	body_end = ftell(artfp);
	had_nl = found_nl;
    }
    return 1;
}

int
nntp_finishbody(bmode)
int bmode;
{
    char b[NNTP_STRLEN];
    if (body_pos < 0)
	return 0;
    if (bmode == FB_DISCARD) {
	/*printf("Discarding the rest of the article...\n") FLUSH; $$*/
#if 0
	/* Implement this if flushing the data becomes possible */
	nntp_artname(openart, -1); /* Or something... */
	openart = 0;	/* Since we didn't finish the art, forget its number */
#endif
    }
    else
    if (bmode == FB_OUTPUT) {
#ifdef VERBOSE
	IF(verbose)
	    printf("Receiving the rest of the article..."), fflush(stdout);
	ELSE
#endif
#ifdef TERSE
	    printf("Receiving..."), fflush(stdout);
#endif
    }
    if (body_end != body_pos)
	fseek(artfp, (long)body_end, 0);
    if (bmode != FB_BACKGROUND)
	nntp_copybody(b, sizeof b, (ART_POS)0x7fffffffL);
    else {
	while (nntp_copybody(b, sizeof b, body_end+1)) {
	    if (input_pending())
		break;
	}
	if (body_pos >= 0)
	    fseek(artfp, (long)body_pos, 0);
    }
    if (bmode == FB_OUTPUT)
	erase_line(0);	/* erase the prompt */
    return 1;
}

int
nntp_seekart(pos)
ART_POS pos;
{
    if (body_pos >= 0) {
	if (body_end < pos) {
	    char b[NNTP_STRLEN];
	    fseek(artfp, (long)body_end, 0);
	    nntp_copybody(b, sizeof b, pos);
	    if (body_pos >= 0)
		body_pos = pos;
	}
	else
	    body_pos = pos;
    }
    return fseek(artfp, (long)pos, 0);
}

ART_POS
nntp_tellart()
{
    return body_pos < 0 ? (ART_POS)ftell(artfp) : body_pos;
}

char*
nntp_readart(s, limit)
char* s;
int limit;
{
    if (body_pos >= 0) {
	if (body_pos == body_end) {
	    if (nntp_copybody(s, limit, body_pos+1) <= 0)
		return NULL;
	    if (body_end - body_pos < limit) {
		body_pos = body_end;
		return s;
	    }
	    fseek(artfp, (long)body_pos, 0);
	}
	s = fgets(s, limit, artfp);
	body_pos = ftell(artfp);
	if (body_pos == body_end)
	    fseek(artfp, (long)body_pos, 0);  /* Prepare for coming write */
	return s;
    }
    return fgets(s, limit, artfp);
}

/* This is a 1-relative list */
static int maxdays[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

time_t
nntp_time()
{
    char* s;
    int year, month, day, hh, mm;
    time_t ss;

    if (nntp_command("DATE") <= 0)
	return -2;
    if (nntp_check() <= 0)
	return time((time_t*)NULL);

    s = rindex(ser_line, ' ') + 1;
    month = (s[4] - '0') * 10 + (s[5] - '0');
    day = (s[6] - '0') * 10 + (s[7] - '0');
    hh = (s[8] - '0') * 10 + (s[9] - '0');
    mm = (s[10] - '0') * 10 + (s[11] - '0');
    ss = (s[12] - '0') * 10 + (s[13] - '0');
    s[4] = '\0';
    year = atoi(s);

    /* This simple algorithm will be valid until the year 2100 */
    if (year % 4)
	maxdays[2] = 28;
    else
	maxdays[2] = 29;
    if (month < 1 || month > 12 || day < 1 || day > maxdays[month]
     || hh < 0 || hh > 23 || mm < 0 || mm > 59
     || ss < 0 || ss > 59)
	return time((time_t*)NULL);

    for (month--; month; month--)
	day += maxdays[month];

    ss = ((((year-1970) * 365 + (year-1969)/4 + day - 1) * 24L + hh) * 60
	  + mm) * 60 + ss;

    return ss;
}

int
nntp_newgroups(t)
time_t t;
{
    struct tm *ts;

    ts = gmtime(&t);
    sprintf(ser_line, "NEWGROUPS %02d%02d%02d %02d%02d%02d GMT",
	ts->tm_year % 100, ts->tm_mon+1, ts->tm_mday,
	ts->tm_hour, ts->tm_min, ts->tm_sec);
    if (nntp_command(ser_line) <= 0)
	return -2;
    return nntp_check();
}

int
nntp_artnums()
{
    if (datasrc->flags & DF_NOLISTGROUP)
	return 0;
    if (nntp_command("LISTGROUP") <= 0)
	return -2;
    if (nntp_check() <= 0) {
	datasrc->flags |= DF_NOLISTGROUP;
	return 0;
    }
    return 1;
}

#if 0
int
nntp_rover()
{
    if (datasrc->flags & DF_NOXROVER)
	return 0;
    if (nntp_command("XROVER 1-") <= 0)
	return -2;
    if (nntp_check() <= 0) {
	datasrc->flags |= DF_NOXROVER;
	return 0;
    }
    return 1;
}
#endif

ART_NUM
nntp_find_real_art(after)
ART_NUM after;
{
    ART_NUM an;

    if (last_cached > after || last_cached < absfirst
     || nntp_stat(last_cached) <= 0) {
	if (nntp_stat_id("") > after)
	    return 0;
    }

    while ((an = nntp_next_art()) > 0) {
	if (an > after)
	    return an;
	if (after - an > 10)
	    break;
    }

    return 0;
}

char*
nntp_artname(artnum, allocate)
ART_NUM artnum;
bool_int allocate;
{
    static ART_NUM artnums[MAX_NNTP_ARTICLES];
    static time_t artages[MAX_NNTP_ARTICLES];
    time_t now, lowage;
    int i, j;

    if (!artnum) {
	for (i = 0; i < MAX_NNTP_ARTICLES; i++) {
	    artnums[i] = 0;
	    artages[i] = 0;
	}
	return NULL;
    }

    now = time((time_t*)NULL);

    for (i = j = 0, lowage = now; i < MAX_NNTP_ARTICLES; i++) {
	if (artnums[i] == artnum) {
	    artages[i] = now;
	    return nntp_tmpname(i);
	}
	if (artages[i] <= lowage)
	    lowage = artages[j = i];
    }

    if (allocate) {
	artnums[j] = artnum;
	artages[j] = now;
	return nntp_tmpname(j);
    }

    return NULL;
}

char*
nntp_tmpname(ndx)
int ndx;
{
    static char artname[20];
    sprintf(artname,"rrn.%ld.%d",our_pid,ndx);
    return artname;
}

int
nntp_handle_nested_lists()
{
    if (strcaseEQ(last_command,"quit"))
	return 0; /*$$ flush data needed? */
    if (nntp_finishbody(FB_DISCARD))
	return 1;
    fprintf(stderr,"Programming error! Nested NNTP calls detected.\n");
    return -1;
}

int
nntp_handle_timeout()
{
    static bool handling_timeout = FALSE;
    char last_command_save[NNTP_STRLEN];

    if (strcaseEQ(last_command,"quit"))
	return 0;
    if (handling_timeout)
	return -1;
    handling_timeout = TRUE;
    strcpy(last_command_save, last_command);
    nntp_close(FALSE);
    datasrc->nntplink = nntplink;
    if (nntp_connect(datasrc->newsid, 0) <= 0)
	return -2;
    datasrc->nntplink = nntplink;
    if (in_ng && nntp_group(ngname, (NGDATA*)NULL) <= 0)
	return -2;
    if (nntp_command(last_command_save) <= 0)
	return -1;
    strcpy(last_command, last_command_save); /*$$ Is this really needed? */
    handling_timeout = FALSE;
    return 1;
}

void
nntp_server_died(dp)
DATASRC* dp;
{
    MULTIRC* mp = multirc;
    close_datasrc(dp);
    dp->flags |= DF_UNAVAILABLE;
    unuse_multirc(mp);
    if (!use_multirc(mp))
	multirc = NULL;
    fprintf(stderr,"\n%s\n", ser_line);
    get_anything();
}

/* nntp_readcheck -- get a line of text from the server, interpreting
** it as a status message for a binary command.  Call this once
** before calling nntp_read() for the actual data transfer.
*/
#ifdef SUPPORT_XTHREAD
long
nntp_readcheck()
{
    /* try to get the status line and the status code */
    switch (nntp_check()) {
      case -2:
	return -2;
      case -1:
      case 0:
	return rawbytes = -1;
    }

    /* try to get the number of bytes being transfered */
    if (sscanf(ser_line, "%*d%ld", &rawbytes) != 1)
	return rawbytes = -1;
    return rawbytes;
}
#endif

/* nntp_read -- read data from the server in binary format.  This call must
** be preceeded by an appropriate binary command and an nntp_readcheck call.
*/
#ifdef SUPPORT_XTHREAD
long
nntp_read(buf, n)
char* buf;
long n;
{
    /* if no bytes to read, then just return EOF */
    if (rawbytes < 0)
	return 0;

#ifdef HAS_SIGHOLD
    sighold(SIGINT);
#endif

    /* try to read some data from the server */
    if (rawbytes) {
	n = fread(buf, 1, n > rawbytes ? rawbytes : n, nntplink.rd_fp);
	rawbytes -= n;
    } else
	n = 0;

    /* if no more left, then fetch the end-of-command signature */
    if (!rawbytes) {
	char buf[5];	/* "\r\n.\r\n" */

	fread(buf, 1, 5, nntplink.rd_fp);
	rawbytes = -1;
    }
#ifdef HAS_SIGHOLD
    sigrelse(SIGINT);
#endif
    return n;
}
#endif /* SUPPORT_XTHREAD */

#endif /* SUPPORT_NNTP */
