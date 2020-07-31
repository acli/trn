/* bits.c
 */
/* This software is copyrighted as detailed in the LICENSE file. */


#include "EXTERN.h"
#include "common.h"
#include "hash.h"
#include "cache.h"
#include "list.h"
#include "ngdata.h"
#include "nntpclient.h"
#include "datasrc.h"
#include "nntp.h"
#include "rcstuff.h"
#include "head.h"
#include "term.h"
#include "util.h"
#include "util2.h"
#include "final.h"
#include "trn.h"
#include "ng.h"
#include "artio.h"
#include "intrp.h"
#include "rcln.h"
#include "ndir.h"
#include "kfile.h"
#include "rthread.h"
#include "rt-process.h"
#include "rt-select.h"
#include "rt-util.h"
#include "INTERN.h"
#include "bits.h"
#include "bits.ih"

#ifdef DBM_XREFS
#    ifdef NULL
#	undef NULL
#    endif
#    include <dbm.h>
#endif

static long chase_count = 0;

void
bits_init()
{
    ;
}

void
rc_to_bits()
{
    char* mybuf = buf;			/* place to decode rc line */
    register char* s;
    register char* c;
    register char* h;
    register long i;
    register ART_NUM unread;
    ARTICLE* ap;

    /* modify the article flags to reflect what has already been read */

    for (s = ngptr->rcline + ngptr->numoffset; *s == ' '; s++) ;
					/* find numbers in rc line */
    i = strlen(s);
#ifndef lint
    if (i >= LBUFLEN-2)			/* bigger than buf? */
	mybuf = safemalloc((MEM_SIZE)(i+2));
#endif
    strcpy(mybuf,s);			/* make scratch copy of line */
    if (mybuf[0])
	mybuf[i++] = ',';		/* put extra comma on the end */
    mybuf[i] = '\0';
    s = mybuf;				/* initialize the for loop below */
    if (set_firstart(s)) {
	s = index(s,',') + 1;
	for (i = article_first(absfirst); i < firstart; i = article_next(i))
	    article_ptr(i)->flags &= ~AF_UNREAD;
	firstart = i;
    }
    else
	firstart = article_first(firstart);
    unread = 0;
#ifdef DEBUG
    if (debug & DEB_CTLAREA_BITMAP) {
	printf("\n%s\n",mybuf) FLUSH;
	termdown(2);
	for (i = article_first(absfirst); i < firstart; i = article_next(i)) {
	    if (article_unread(i))
		printf("%ld ",(long)i) FLUSH;
	}
    }
#endif
    i = firstart;
    for ( ; (c = index(s,',')) != NULL; s = ++c) {	/* for each range */
	ART_NUM min, max;
	*c = '\0';			/* do not let index see past comma */
	h = index(s,'-');
	min = atol(s);
	if (min < firstart)		/* make sure range is in range */
	    min = firstart;
	if (min > lastart)
	    min = lastart+1;
	for (; i < min; i = article_next(i)) {
	    ap = article_ptr(i);
	    if (ap->flags & AF_EXISTS) {
		if (ap->autofl & AUTO_KILLS)
		    ap->flags &= ~AF_UNREAD;
		else {
		    ap->flags |= AF_UNREAD;
		    unread++;
		    if (ap->autofl & AUTO_SELS)
			select_article(ap, ap->autofl);
		}
	    }
	}
	if (!h)
	    max = min;
	else if ((max = atol(h+1)) < min)
	    max = min-1;
	if (max > lastart)
	    max = lastart;
	/* mark all arts in range as read */
	for ( ; i <= max; i = article_next(i))
	    article_ptr(i)->flags &= ~AF_UNREAD;
#ifdef DEBUG
	if (debug & DEB_CTLAREA_BITMAP) {
	    printf("\n%s\n",s) FLUSH;
	    termdown(2);
	    for (i = absfirst; i <= lastart; i++) {
		if (!was_read(i))
		    printf("%ld ",(long)i) FLUSH;
	    }
	}
#endif
	i = article_next(max);
    }
    for (; i <= lastart; i = article_next(i)) {
	ap = article_ptr(i);
	if (ap->flags & AF_EXISTS) {
	    if (ap->autofl & AUTO_KILLS)
		ap->flags &= ~AF_UNREAD;
	    else {
		ap->flags |= AF_UNREAD;
		unread++;
		if (ap->autofl & AUTO_SELS)
		    select_article(ap, ap->autofl);
	    }
	}
    }
#ifdef DEBUG
    if (debug & DEB_CTLAREA_BITMAP) {
	fputs("\n(hit CR)",stdout) FLUSH;
	termdown(1);
	fgets(cmd_buf, sizeof cmd_buf, stdin);
    }
#endif
    if (mybuf != buf)
	free(mybuf);
    ngptr->toread = unread;
}

bool
set_firstart(s)
char* s;
{
    while (*s == ' ') s++;

    if (strnEQ(s,"1-",2)) {		/* can we save some time here? */
	firstart = atol(s+2)+1;		/* process first range thusly */
	if (firstart < absfirst)
	    firstart = absfirst;
	return TRUE;
    }

    firstart = absfirst;
    return FALSE;
}

/* reconstruct the .newsrc line in a human readable form */

void
bits_to_rc()
{
    register char* s;
    register char* mybuf = buf;
    register ART_NUM i;
    ART_NUM count=0;
    int safelen = LBUFLEN - 32;

    strcpy(buf,ngptr->rcline);		/* start with the newsgroup name */
    s = buf + ngptr->numoffset - 1;	/* use s for buffer pointer */
    *s++ = ngptr->subscribechar;		/* put the requisite : or !*/
    for (i = article_first(absfirst); i <= lastart; i = article_next(i)) {
	if (article_unread(i))
	    break;
    }
    sprintf(s," 1-%ld,",(long)i-1);
    s += strlen(s);
    for (; i<=lastart; i++) {	/* for each article in newsgroup */
	if (s-mybuf > safelen) {	/* running out of room? */
	    safelen *= 2;
	    if (mybuf == buf) {		/* currently static? */
		*s = '\0';
		mybuf = safemalloc((MEM_SIZE)safelen + 32);
		strcpy(mybuf,buf);	/* so we must copy it */
		s = mybuf + (s-buf);
					/* fix the pointer, too */
	    }
	    else {			/* just grow in place, if possible */
		int oldlen = s - mybuf;
		mybuf = saferealloc(mybuf,(MEM_SIZE)safelen + 32);
		s = mybuf + oldlen;
	    }
	}
	if (!was_read(i))		/* still unread? */
	    count++;			/* then count it */
	else {				/* article was read */
	    ART_NUM oldi;

	    sprintf(s,"%ld",(long)i);	/* put out the min of the range */
	    s += strlen(s);		/* keeping house */
	    oldi = i;			/* remember this spot */
	    do i++; while (i <= lastart && was_read(i));
					/* find 1st unread article or end */
	    i--;			/* backup to last read article */
	    if (i > oldi) {		/* range of more than 1? */
		sprintf(s,"-%ld,",(long)i);
					/* then it out as a range */
		s += strlen(s);		/* and housekeep */
	    }
	    else
		*s++ = ',';		/* otherwise, just a comma will do */
	}
    }
    if (*(s-1) == ',')			/* is there a final ','? */
	s--;				/* take it back */
    *s++ = '\0';			/* and terminate string */
#ifdef DEBUG
    if ((debug & DEB_NEWSRC_LINE) && !panic) {
	printf("%s: %s\n",ngptr->rcline,ngptr->rcline+ngptr->numoffset) FLUSH;
	printf("%s\n",mybuf) FLUSH;
	termdown(2);
    }
#endif
    free(ngptr->rcline);		/* return old rc line */
    if (mybuf == buf) {
	ngptr->rcline = safemalloc((MEM_SIZE)(s-buf)+1);
					/* grab a new rc line */
	strcpy(ngptr->rcline, buf);	/* and load it */
    }
    else {
	mybuf = saferealloc(mybuf,(MEM_SIZE)(s-mybuf)+1);
					/* be nice to the heap */
	ngptr->rcline = mybuf;
    }
    *(ngptr->rcline + ngptr->numoffset - 1) = '\0';
    if (ngptr->subscribechar == NEGCHAR)/* did they unsubscribe? */
	ngptr->toread = TR_UNSUB;	/* make line invisible */
    else
	ngptr->toread = (ART_UNREAD)count; /* otherwise, remember the count */
    ngptr->rc->flags |= RF_RCCHANGED;
}

void
find_existing_articles()
{
    ART_NUM an;
    ARTICLE* ap;

#ifdef SUPPORT_NNTP
    if (datasrc->flags & DF_REMOTE) {
	/* Parse the LISTGROUP output and remember everything we find */
	if (/*nntp_rover() ||*/ nntp_artnums()) {
	    /*char* s;*/

	    for (ap = article_ptr(article_first(absfirst));
		 ap && article_num(ap) <= lastart;
		 ap = article_nextp(ap))
		ap->flags &= ~AF_EXISTS;
	    for (;;) {
		if (nntp_gets(ser_line, sizeof ser_line) < 0)
		    break; /*$$*/
		if (nntp_at_list_end(ser_line))
		    break;
		an = (ART_NUM)atol(ser_line);
		if (an < absfirst)
		    continue;	/* Ignore some wacked-out NNTP servers */
		ap = article_ptr(an);
		if (!(ap->flags2 & AF2_BOGUS))
		    ap->flags |= AF_EXISTS;
#if 0
		s = index(ser_line, ' ');
		if (s)
		    rover_thread(article_ptr(an), s);
#endif
	    }
	}
	else if (first_subject && cached_all_in_range) {
	    if (!datasrc->ov_opened || datasrc->over_dir != NULL) {
		for (ap = article_ptr(article_first(first_cached));
		     ap && article_num(ap) <= last_cached;
		     ap = article_nextp(ap))
		{
		    if (ap->flags & AF_CACHED)
			ap->flags |= AF_EXISTS;
		}
	    }
	    for (an = absfirst; an < first_cached; an++) {
		ap = article_ptr(an);
		if (!(ap->flags2 & AF2_BOGUS))
		    ap->flags |= AF_EXISTS;
	    }
	    for (an = last_cached+1; an <= lastart; an++) {
		ap = article_ptr(an);
		if (!(ap->flags2 & AF2_BOGUS))
		    ap->flags |= AF_EXISTS;
	    }
	}
	else {
	    for (an = absfirst; an <= lastart; an++) {
		ap = article_ptr(an);
		if (!(ap->flags2 & AF2_BOGUS))
		    ap->flags |= AF_EXISTS;
	    }
	}
    }
    else
#endif
    {
	ART_NUM first = lastart+1;
	ART_NUM last = 0;
	DIR* dirp;
	Direntry_t* dp;
	char ch;
	long lnum;

	/* Scan the directory to find which articles are present. */

	if (!(dirp = opendir(".")))
	    return;

	for (ap = article_ptr(article_first(absfirst));
	     ap && article_num(ap) <= lastart;
	     ap = article_nextp(ap))
	    ap->flags &= ~AF_EXISTS;

	while ((dp = readdir(dirp)) != NULL) {
	    if (sscanf(dp->d_name, "%ld%c", &lnum, &ch) == 1) {
		an = (ART_NUM)lnum;
		if (an <= lastart && an >= absfirst) {
		    if (an < first)
			first = an;
		    if (an > last)
			last = an;
		    ap = article_ptr(an);
		    if (!(ap->flags2 & AF2_BOGUS))
			ap->flags |= AF_EXISTS;
		}
	    }
	}
	closedir(dirp);

	ngptr->abs1st = first;
	ngptr->ngmax = last;

	if (first > absfirst) {
	    checkexpired(ngptr,first);
	    for (absfirst = article_first(absfirst);
		 absfirst < first;
		 absfirst = article_next(absfirst))
	    {
		onemissing(article_ptr(absfirst));
	    }
	    absfirst = first;
	}
	lastart = last;
    }

    if (firstart < absfirst)
	firstart = absfirst;
    if (firstart > lastart)
	firstart = lastart + 1;
    if (first_cached < absfirst)
	first_cached = absfirst;
    if (last_cached < absfirst)
	last_cached = absfirst - 1;
}

/* mark an article unread, keeping track of toread[] */

void
onemore(ap)
ARTICLE* ap;
{
    if (!(ap->flags & AF_UNREAD)) {
	register ART_NUM artnum = article_num(ap);
	check_first(artnum);
	ap->flags |= AF_UNREAD;
	ap->flags &= ~AF_DEL;
	ngptr->toread++;
	if (ap->subj) {
	    if (selected_only) {
		if (ap->subj->flags & sel_mask) {
		    ap->flags |= sel_mask;
		    selected_count++;
		}
	    } else
		ap->subj->flags |= SF_VISIT;
	}
    }
}

/* mark an article read, keeping track of toread[] */

void
oneless(ap)
ARTICLE* ap;
{
    if (ap->flags & AF_UNREAD) {
	ap->flags &= ~AF_UNREAD;
	/* Keep selected_count accurate */
	if (ap->flags & sel_mask) {
	    selected_count--;
	    ap->flags &= ~sel_mask;
	}
	if (ngptr->toread > TR_NONE)
	    ngptr->toread--;
    }
}

void
oneless_artnum(artnum)
ART_NUM artnum;
{
    ARTICLE* ap = article_find(artnum);
    if (ap)
	oneless(ap);
}

void
onemissing(ap)
ARTICLE* ap;
{
    missing_count += (ap->flags & AF_UNREAD) != 0;
    oneless(ap);
    ap->flags = (ap->flags & ~(AF_HAS_RE|AF_YANKBACK|AF_FROMTRUNCED|AF_EXISTS))
	      | AF_CACHED|AF_THREADED;
}

/* mark an article as unread, with possible xref chasing */

void
unmark_as_read(ap)
register ARTICLE* ap;
{
    onemore(ap);
#ifdef MCHASE
    if (ap->xrefs != nullstr && !(ap->flags & AF_MCHASE)) {
	ap->flags |= AF_MCHASE;
	chase_count++;
    }
#endif
}

/* Mark an article as read in this newsgroup and possibly chase xrefs.
** Don't call this on missing articles.
*/
void
set_read(ap)
register ARTICLE* ap;
{
    oneless(ap);
    if (!olden_days && ap->xrefs != nullstr && !(ap->flags & AF_KCHASE)) {
	ap->flags |= AF_KCHASE;
	chase_count++;
    }
}

/* temporarily mark article as read.  When newsgroup is exited, articles */
/* will be marked as unread.  Called via M command */

void
delay_unmark(ap)
ARTICLE* ap;
{
    if (!(ap->flags & AF_YANKBACK)) {
	ap->flags |= AF_YANKBACK;
	dmcount++;
    }
}

/* mark article as read.  If article is cross referenced to other */
/* newsgroups, mark them read there also. */

void
mark_as_read(ap)
register ARTICLE* ap;
{
    oneless(ap);
    if (ap->xrefs != nullstr && !(ap->flags & AF_KCHASE)) {
	ap->flags |= AF_KCHASE;
	chase_count++;
    }
    checkcount++;			/* get more worried about crashes */
}

void
mark_missing_articles()
{
    register ARTICLE* ap;
    for (ap = article_ptr(article_first(absfirst));
	 ap && article_num(ap) <= lastart;
	 ap = article_nextp(ap))
    {
	if (!(ap->flags & AF_EXISTS))
	    onemissing(ap);
    }
}

/* keep firstart pointing at the first unread article */

void
check_first(min)
ART_NUM min;
{
    if (min < absfirst)
	min = absfirst;
    if (min < firstart)
	firstart = min;
}

/* bring back articles marked with M */

void
yankback()
{
    if (dmcount) {			/* delayed unmarks pending? */
	if (panic)
	    ;
	else if (gmode == 's')
	    sprintf(msg, "Returned %ld Marked article%s.",(long)dmcount,
		PLURAL(dmcount));
	else {
#ifdef VERBOSE
	    printf("\nReturning %ld Marked article%s...\n",(long)dmcount,
		PLURAL(dmcount)) FLUSH;
#endif
	    termdown(2);
	}
	article_walk(yank_article, 0);
	dmcount = 0;
    }
}

static bool
yank_article(ptr, arg)
char* ptr;
int arg;
{
    register ARTICLE* ap = (ARTICLE*)ptr;
    if (ap->flags & AF_YANKBACK) {
	unmark_as_read(ap);
	if (selected_only)
	    select_article(ap, 0);
	ap->flags &= ~AF_YANKBACK;
    }
    return 0;
}

int
chase_xrefs(until_key)
bool_int until_key;
{
    if (!chase_count)
	return 1;
    if (until_key)
	setspin(SPIN_BACKGROUND);

    article_walk(check_chase, until_key);
    chase_count = 0;
    return 1;
}

static bool
check_chase(ptr, until_key)
char* ptr;
int until_key;
{
    register ARTICLE* ap = (ARTICLE*)ptr;

    if (ap->flags & AF_KCHASE) {
	chase_xref(article_num(ap),TRUE);
	ap->flags &= ~AF_KCHASE;
	if (!--chase_count)
	    return 1;
    }
#ifdef MCHASE
    if (ap->flags & AF_MCHASE) {
	chase_xref(article_num(ap),TRUE);
	ap->flags &= ~AF_MCHASE;
	if (!--chase_count)
	    return 1;
    }
#endif
    if (until_key && input_pending())
	return 1;
    return 0;
}

/* run down xref list and mark as read or unread */

#ifndef DBM_XREFS
/*=-=-=-=*/
static int
chase_xref(artnum,markread)	/* The Xref-line-using version */
ART_NUM artnum;
int markread;
{
    register char* xartnum;
    register ART_NUM x;
    char* xref_buf, *curxref;
    char tmpbuf[128];

    if (datasrc->flags & DF_NOXREFS)
	return 0;

    if (inbackground())
	spin(10);
    else {
	if (output_chase_phrase) {
# ifdef VERBOSE
	    IF(verbose)
		fputs("\nChasing xrefs", stdout);
	    ELSE
# endif
# ifdef TERSE
		fputs("\nXrefs", stdout);
# endif
	    termdown(1);
	    output_chase_phrase = 0;
	}
	putchar('.');
	fflush(stdout);
    }

    xref_buf = fetchcache(artnum, XREF_LINE, FILL_CACHE);
    if (!xref_buf || !*xref_buf)
	return 0;

    xref_buf = savestr(xref_buf);
# ifdef DEBUG
    if (debug & DEB_XREF_MARKER) {
	printf("Xref: %s\n",xref_buf) FLUSH;
	termdown(1);
    }
# endif
    curxref = cpytill(tmpbuf,xref_buf,' ') + 1;
# ifdef VALIDATE_XREF_SITE
    if (valid_xref_site(artnum,tmpbuf))
# endif
    {
	while (*curxref) {		/* for each newsgroup */
	    curxref = cpytill(tmpbuf,curxref,' ');
	    xartnum = index(tmpbuf,':');
	    if (!xartnum)
		break;
	    *xartnum++ = '\0';
	    if (!(x = atol(xartnum)))
		continue;
	    if (strEQ(tmpbuf,ngname)) {/* is this the current newsgroup? */
		if (x < absfirst || x > lastart)
		    continue;
		if (markread)
		    oneless(article_ptr(x)); /* take care of old C newses */
# ifdef MCHASE
		else
		    onemore(article_ptr(x));
# endif
	    } else {
		if (markread) {
		    if (addartnum(datasrc,x,tmpbuf))
			break;
		}
# ifdef MCHASE
		else
		    subartnum(datasrc,x,tmpbuf);
# endif
	    }
	    while (*curxref && isspace(*curxref))
		curxref++;
	}
    }
    free(xref_buf);
    return 0;
}

/* Make sure the site name on Xref matches what inews thinks the site
 * is.  Check first against last inews_site.  If it matches, fine.
 * If not, fetch inews_site from current Path or Relay-Version line and
 * check again.  This is so that if the new administrator decides
 * to change the system name as known to inews, rn will still do
 * Xrefs correctly--each article need only match itself to be valid.
 */ 
# ifdef VALIDATE_XREF_SITE
static bool
valid_xref_site(artnum, site)
ART_NUM artnum;
char* site;
{
    static char* inews_site = NULL;
    char* sitebuf;
    char* s;

    if (inews_site && strEQ(site,inews_site))
	return TRUE;

    if (inews_site)
	free(inews_site);
#ifndef ANCIENT_NEWS
    /* Grab the site from the first component of the Path line */
    sitebuf = fetchlines(artnum,PATH_LINE);
    if ((s = index(sitebuf, '!')) != NULL) {
	*s = '\0';
	inews_site = savestr(sitebuf);
    }
#else /* ANCIENT_NEWS */
    /* Grab the site from the Posting-Version line */
    sitebuf = fetchlines(artnum,RVER_LINE);
    if ((s = instr(sitebuf,"; site ",TRUE)) != NULL) {
	char* t = index(s+7, '.');
	if (t)
	    *t = '\0';
	inews_site = savestr(s+7);
    }
#endif /* ANCIENT_NEWS */
    else
	inews_site = savestr(nullstr);
    free(sitebuf);

    if (strEQ(site,inews_site))
	return TRUE;

#ifdef DEBUG
    if (debug) {
	printf("Xref not from %s -- ignoring\n",inews_site) FLUSH;
	termdown(1);
    }
#endif
    return FALSE;
}
# endif /* VALIDATE_XREF_SITE */

#else /* DBM_XREFS */

static int
chase_xref(artnum,markread)		/* The DBM version */
ART_NUM artnum;
int markread;
{
    datum lhs, rhs;
    datum fetch();
    register char* idp;
    char* ident_buf;
    static FILE* hist_file = NULL;
    long pos;
    register char* xartnum;
    register ART_NUM x;
    char* xref_buf;
    char* curxref;
    char tmpbuf[128];

    if (datasrc->flags & DF_NOXREFS)
	return;

    if (inbackground())
	spin(10);
    else {
	if (output_chase_phrase) {
# ifdef VERBOSE
	    IF(verbose)
		fputs("\nChasing xrefs", stdout);
	    ELSE
# endif
# ifdef TERSE
		fputs("\nXrefs", stdout);
# endif
	    termdown(1);
	    output_chase_phrase = 0;
	}
	putchar('.');
	fflush(stdout);
    }

    xref_buf = fetchcache(artnum, NGS_LINE, FILL_CACHE);
    if (!xref_buf || !*xref_buf)
	return 0;

    xref_buf = safemalloc((MEM_SIZE)BUFSIZ);
    if (hist_file == NULL) {	/* Init. file accesses */
# ifdef DEBUG
	if (debug) {
	    printf("chase_xref: opening files\n");
	    termdown(1);
	}
# endif
	dbminit(filexp(ARTFILE));
	if ((hist_file = fopen(filexp(ARTFILE), "r")) == NULL)
	    return 0;
    }
    ident_buf = fetchlines(artnum,MSGID_LINE);	/* get Message-ID */
# ifdef DEBUG
    if (debug) {
	printf ("chase_xref: Message-ID: %s\n", ident_buf);
	termdown(1);
    }
# endif
    
    if ((idp = index(ident_buf, '@')) != NULL) {
	while (*++idp)			/* make message-id case insensitive */
	    if (isupper(*idp))
		*idp = tolower(*idp);
    }
    lhs.dptr = ident_buf;		/* look up article by id */
    lhs.dsize = strlen(lhs.dptr) + 1;
    rhs = fetch(lhs);			/* fetch the record */
    if (rhs.dptr == NULL)		/* if NULL, nothing there */
	goto wild_goose;
    bcopy(rhs.dptr,(char*)&pos, 4);
    fseek(hist_file, pos, 0);	/* datum returned is position in hist file */
    fgets(xref_buf, BUFSIZ, hist_file);
# ifdef DEBUG
    if (debug) {
	printf ("Xref from history: %s\n", xref_buf);
	termdown(1);
    }
# endif
    curxref = cpytill(tmpbuf, xref_buf, '\t') + 1;
    curxref = cpytill(tmpbuf, curxref, '\t') + 1;
# ifdef DEBUG
    if (debug) {
	printf ("chase_xref: curxref: %s\n", curxref);
	termdown(1);
    }
# endif
    while (*curxref) {			/* for each newsgroup */
	curxref = cpytill(tmpbuf,curxref,' ');
	xartnum = index(tmpbuf,'/');
	if (!xartnum)			/* probably an old-style Xref */
	    break;
	*xartnum++ = '\0';
	if (!(x = atol(xartnum)))
	    continue;
	if (strNE(tmpbuf,ngname)) {	/* not the current newsgroup? */
	    if (markread) {
		if (addartnum(datasrc,x,tmpbuf))
		    goto wild_goose;
	    }
# ifdef MCHASE
	    else
		subartnum(datasrc,x,tmpbuf);
# endif
	}
	while (*curxref && isspace(*curxref))
	    curxref++;
    }
  wild_goose:
    free(xref_buf);
    free(ident_buf);
    return 0;
}
#endif /* DBM_XREFS */
