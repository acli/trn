/* rcln.c
 */
/* This software is copyrighted as detailed in the LICENSE file. */


#include "EXTERN.h"
#include "common.h"
#include "util.h"
#include "util2.h"
#include "list.h"
#include "hash.h"
#include "ngdata.h"
#include "nntpclient.h"
#include "datasrc.h"
#include "nntp.h"
#include "rcstuff.h"
#include "term.h"
#include "INTERN.h"
#include "rcln.h"

#define MAX_DIGITS 7

void
rcln_init()
{
    ;
}

#ifdef CATCHUP
void
catch_up(np, leave_count, output_level)
NGDATA* np;
int leave_count;
int output_level;
{
    char tmpbuf[128];
    
    if (leave_count) {
	if (output_level) {
#ifdef VERBOSE
	    IF(verbose)
		printf("\nMarking all but %d articles in %s as read.\n",
		       leave_count,np->rcline) FLUSH;
	    ELSE
#endif
#ifdef TERSE
		printf("\nAll but %d marked as read.\n",leave_count) FLUSH;
#endif
	}
	checkexpired(np, getngsize(np) - leave_count + 1);
	set_toread(np, ST_STRICT);
    }
    else {
	if (output_level) {
#ifdef VERBOSE
	    IF(verbose)
		printf("\nMarking %s as all read.\n",np->rcline) FLUSH;
	    ELSE
#endif
#ifdef TERSE
		fputs("\nMarked read\n",stdout) FLUSH;
#endif
	}
	sprintf(tmpbuf,"%s: 1-%ld", np->rcline,(long)getngsize(np));
	free(np->rcline);
	np->rcline = savestr(tmpbuf);
	*(np->rcline + np->numoffset - 1) = '\0';
	if (ng_min_toread > TR_NONE && np->toread > TR_NONE)
	    newsgroup_toread--;
	np->toread = TR_NONE;
    }
    np->rc->flags |= RF_RCCHANGED;
    if (!write_newsrcs(multirc))
	get_anything();
}
#endif

/* add an article number to a newsgroup, if it isn't already read */

int
addartnum(dp,artnum,ngnam)
DATASRC* dp;
ART_NUM artnum;
char* ngnam;
{
    register NGDATA* np;
    register char* s;
    register char* t;
    register char* maxt = NULL;
    ART_NUM min = 0, max = -1, lastnum = 0;
    char* mbuf;
    bool_int morenum;

    if (!artnum)
	return 0;
    np = find_ng(ngnam);
    if (np == NULL)			/* not found in newsrc? */
	return 0;
    if (dp != np->rc->datasrc) {	/* punt on cross-host xrefs */
#ifdef DEBUG
	if (debug & DEB_XREF_MARKER)
	    printf("Cross-host xref to group %s ignored.\n",ngnam) FLUSH;
#endif
	return 0;
    }
    if (!np->numoffset)
	return 0;
#ifndef ANCIENT_NEWS
    if (!np->abs1st) {
	/* Trim down the list due to expires if we haven't done so yet. */
	set_toread(np, ST_LAX);
    }
#endif
#if 0
    if (artnum > np->ngmax + 200) {	/* allow for incoming articles */
	printf("\nCorrupt Xref line!!!  %ld --> %s(1..%ld)\n",
	    artnum,ngnam,
	    np->ngmax) FLUSH;
	paranoid = TRUE;		/* paranoia reigns supreme */
	return -1;			/* hope this was the first newsgroup */
    }
#endif

    if (np->toread == TR_BOGUS)
	return 0;
    if (artnum > np->ngmax) {
	if (np->toread > TR_NONE)
	    np->toread += artnum - np->ngmax;
	np->ngmax = artnum;
    }
#ifdef DEBUG
    if (debug & DEB_XREF_MARKER) {
	printf("%ld->\n%s%c%s\n",(long)artnum,np->rcline, np->subscribechar,
	  np->rcline + np->numoffset) FLUSH;
    }
#endif
    s = np->rcline + np->numoffset;
    while (*s == ' ') s++;		/* skip spaces */
    t = s;
    while (isdigit(*s) && artnum >= (min = atol(s))) {
					/* while it might have been read */
	for (t = s; isdigit(*t); t++) ;	/* skip number */
	if (*t == '-') {		/* is it a range? */
	    t++;			/* skip to next number */
	    if (artnum <= (max = atol(t)))
		return 0;		/* it is in range => already read */
	    lastnum = max;		/* remember it */
	    maxt = t;			/* remember position in case we */
					/* want to overwrite the max */
	    while (isdigit(*t)) t++;	/* skip second number */
	}
	else {
	    if (artnum == min)		/* explicitly a read article? */
		return 0;
	    lastnum = min;		/* remember what the number was */
	    maxt = NULL;		/* last one was not a range */
	}
	while (*t && !isdigit(*t)) t++;	/* skip comma and any spaces */
	s = t;
    }

    /* we have not read it, so insert the article number before s */

    morenum = isdigit(*s);		/* will it need a comma after? */
    *(np->rcline + np->numoffset - 1) = np->subscribechar;
    mbuf = safemalloc((MEM_SIZE)(strlen(s)+(s - np->rcline)+MAX_DIGITS+2+1));
    strcpy(mbuf,np->rcline);		/* make new rc line */
    if (maxt && lastnum && artnum == lastnum+1)
    					/* can we just extend last range? */
	t = mbuf + (maxt - np->rcline);	/* then overwrite previous max */
    else {
	t = mbuf + (t - np->rcline);	/* point t into new line instead */
	if (lastnum) {			/* have we parsed any line? */
	    if (!morenum)		/* are we adding to the tail? */
		*t++ = ',';		/* supply comma before */
	    if (!maxt && artnum == lastnum+1 && *(t-1) == ',')
					/* adjacent singletons? */
		*(t-1) = '-';		/* turn them into a range */
	}
    }
    if (morenum) {			/* is there more to life? */
	if (min == artnum+1) {		/* can we consolidate further? */
	    bool range_before = (*(t-1) == '-');
	    bool range_after;
	    char* nextmax;

	    for (nextmax = s; isdigit(*nextmax); nextmax++) ;
	    range_after = *nextmax++ == '-';
	    
	    if (range_before)
		*t = '\0';		/* artnum is redundant */
	    else
		sprintf(t,"%ld-",(long)artnum);/* artnum will be new min */
	    
	    if (range_after)
		s = nextmax;		/* *s is redundant */
	/*  else
		s = s */		/* *s is new max */
	}
	else
	    sprintf(t,"%ld,",(long)artnum);	/* put the number and comma */
    }
    else
	sprintf(t,"%ld",(long)artnum);	/* put the number there (wherever) */
    strcat(t,s);			/* copy remainder of line */
#ifdef DEBUG
    if (debug & DEB_XREF_MARKER)
	printf("%s\n",mbuf) FLUSH;
#endif
    free(np->rcline);
    np->rcline = mbuf;		/* pull the switcheroo */
    *(np->rcline + np->numoffset - 1) = '\0';
					/* wipe out : or ! */
    if (np->toread > TR_NONE)	/* lest we turn unsub into bogus */
	np->toread--;
    return 0;
}

/* delete an article number from a newsgroup, if it is there */

#ifdef MCHASE
void
subartnum(dp,artnum,ngnam)
DATASRC* dp;
register ART_NUM artnum;
char* ngnam;
{
    register NGDATA* np;
    register char* s;
    register char* t;
    register ART_NUM min, max;
    char* mbuf;
    int curlen;

    if (!artnum)
	return;
    np = find_ng(ngnam);
    if (np == NULL)			/* not found in newsrc? */
	return;	
    if (dp != np->rc->datasrc)		/* punt on cross-host xrefs */
	return;
    if (!np->numoffset)
	return;
#ifdef DEBUG
    if (debug & DEB_XREF_MARKER) {
	printf("%ld<-\n%s%c%s\n",(long)artnum,np->rcline,np->subscribechar,
	  np->rcline + np->numoffset) FLUSH;
    }
#endif
    s = np->rcline + np->numoffset;
    while (*s == ' ') s++;		/* skip spaces */
    
    /* a little optimization, since it is almost always the last number */
    
    for (t=s; *t; t++) ;		/* find end of string */
    curlen = t - np->rcline;
    for (t--; isdigit(*t); t--) ;	/* find previous delim */
    if (*t == ',' && atol(t+1) == artnum) {
	*t = '\0';
	if (np->toread >= TR_NONE)
	    ++np->toread;
#ifdef DEBUG
	if (debug & DEB_XREF_MARKER)
	    printf("%s%c %s\n",np->rcline,np->subscribechar,s) FLUSH;
#endif
	return;
    }

    /* not the last number, oh well, we may need the length anyway */

    while (isdigit(*s) && artnum >= (min = atol(s))) {
					/* while it might have been read */
	for (t = s; isdigit(*t); t++) ;	/* skip number */
	if (*t == '-') {		/* is it a range? */
	    t++;			/* skip to next number */
	    max = atol(t);
	    while (isdigit(*t)) t++;	/* skip second number */
	    if (artnum <= max) {
					/* it is in range => already read */
		if (artnum == min) {
		    min++;
		    artnum = 0;
		}
		else if (artnum == max) {
		    max--;
		    artnum = 0;
		}
		*(np->rcline + np->numoffset - 1) = np->subscribechar;
		mbuf = safemalloc((MEM_SIZE)(curlen+(artnum?(MAX_DIGITS+1)*2+1:1+1)));
		*s = '\0';
		strcpy(mbuf,np->rcline);	/* make new rc line */
		s = mbuf + (s - np->rcline);
					/* point s into mbuf now */
		if (artnum) {		/* split into two ranges? */
		    prange(s,min,artnum-1);
		    s += strlen(s);
		    *s++ = ',';
		    prange(s,artnum+1,max);
		}
		else			/* only one range */
		    prange(s,min,max);
		strcat(s,t);		/* copy remainder over */
#ifdef DEBUG
		if (debug & DEB_XREF_MARKER) {
		    printf("%s\n",mbuf) FLUSH;
		}
#endif
		free(np->rcline);
		np->rcline = mbuf;	/* pull the switcheroo */
		*(np->rcline + np->numoffset - 1) = '\0';
					/* wipe out : or ! */
		if (np->toread >= TR_NONE)
		    np->toread++;
		return;
	    }
	}
	else {
	    if (artnum == min) {	/* explicitly a read article? */
		if (*t == ',')		/* pick a comma, any comma */
		    t++;
		else if (s[-1] == ',')
		    s--;
		else if (s[-2] == ',')	/* (in case of space) */
		    s -= 2;
		strcpy(s,t);		/* no need to realloc */
		if (np->toread >= TR_NONE)
		    np->toread++;
#ifdef DEBUG
		if (debug & DEB_XREF_MARKER) {
		    printf("%s%c%s\n",np->rcline,np->subscribechar,
		      np->rcline + np->numoffset) FLUSH;
		}
#endif
		return;
	    }
	}
	while (*t && !isdigit(*t)) t++;	/* skip comma and any spaces */
	s = t;
    }
}

void
prange(where,min,max)
char* where;
ART_NUM min,max;
{
    if (min == max)
	sprintf(where,"%ld",(long)min);
    else
	sprintf(where,"%ld-%ld",(long)min,(long)max);
}
#endif

/* calculate the number of unread articles for a newsgroup */

void
set_toread(np, lax_high_check)
register NGDATA* np;
bool_int lax_high_check;
{
    register char* s;
    register char* c;
    register char* h;
    char tmpbuf[64];
    char* mybuf = tmpbuf;
    char* nums;
    int length;
    bool virgin_ng = (!np->abs1st);
    ART_NUM ngsize = getngsize(np);
    ART_NUM unread = ngsize;
    ART_NUM newmax;

    if (ngsize == TR_BOGUS) {
	if (!toread_quiet) {
	    printf("\nInvalid (bogus) newsgroup found: %s\n",np->rcline)
	      FLUSH;
	}
	paranoid = TRUE;
	if (virgin_ng || np->toread >= ng_min_toread) {
	    newsgroup_toread--;
	    missing_count++;
	}
	np->toread = TR_BOGUS;
	return;
    }
    if (virgin_ng) {
	sprintf(tmpbuf," 1-%ld",(long)ngsize);
	if (strNE(tmpbuf,np->rcline+np->numoffset))
	    checkexpired(np,np->abs1st);	/* this might realloc rcline */
    }
    nums = np->rcline + np->numoffset;
    length = strlen(nums);
    if (length+MAX_DIGITS+1 > sizeof tmpbuf)
	mybuf = safemalloc((MEM_SIZE)(length+MAX_DIGITS+1));
    strcpy(mybuf,nums);
    mybuf[length++] = ',';
    mybuf[length] = '\0';
    for (s = mybuf; isspace(*s); s++)
	    ;
    for ( ; (c = index(s,',')) != NULL ; s = ++c) {  /* for each range */
	*c = '\0';			/* keep index from running off */
	if ((h = index(s,'-')) != NULL)	/* find - in range, if any */
	    unread -= (newmax = atol(h+1)) - atol(s) + 1;
	else if ((newmax = atol(s)) != 0)
	    unread--;		/* recalculate length */
	if (newmax > ngsize) {	/* paranoia check */
	    if (!lax_high_check && newmax > ngsize) {
		unread = -1;
		break;
	    } else {
		unread += newmax - ngsize;
		np->ngmax = ngsize = newmax;
	    }
	}
    }
    if (unread < 0) {			/* SOMEONE RESET THE NEWSGROUP!!! */
	unread = (ART_UNREAD)ngsize;	/* assume nothing carried over */
	if (!toread_quiet) {
	    printf("\nSomebody reset %s -- assuming nothing read.\n",
		   np->rcline) FLUSH;
	}
	*(np->rcline + np->numoffset) = '\0';
	paranoid = TRUE;		/* enough to make a guy paranoid */
	np->rc->flags |= RF_RCCHANGED;
    }
    if (np->subscribechar == NEGCHAR)
	unread = TR_UNSUB;

    if (unread >= ng_min_toread) {
	if (!virgin_ng && np->toread < ng_min_toread)
	    newsgroup_toread++;
    }
    else if (unread <= 0) {
	if (np->toread > ng_min_toread) {
	    newsgroup_toread--;
	    if (virgin_ng)
		missing_count++;
	}
    }
    np->toread = (ART_UNREAD)unread;	/* remember how many are left */

    if (mybuf != tmpbuf)
	free(mybuf);
}

/* make sure expired articles are marked as read */

void
checkexpired(np,a1st)
register NGDATA* np;
register ART_NUM a1st;
{
    register char* s;
    register ART_NUM num, lastnum = 0;
    char* mbuf;
    char* cp;
    int len;

    if (a1st<=1)
	return;
#ifdef DEBUG
    if (debug & DEB_XREF_MARKER) {
	printf("1-%ld->\n%s%c%s\n",(long)(a1st-1),np->rcline,np->subscribechar,
	  np->rcline + np->numoffset) FLUSH;
    }
#endif
    for (s = np->rcline + np->numoffset; isspace(*s); s++) ;
    while (*s && (num = atol(s)) <= a1st) {
	while (isdigit(*s)) s++;
	while (*s && !isdigit(*s)) s++;
	lastnum = num;
    }
    len = strlen(s);
    if (len && s[-1] == '-') {			/* landed in a range? */
	if (lastnum != 1) {
	    if (3+len <= (int)strlen(np->rcline+np->numoffset))
		mbuf = np->rcline;
	    else {
		mbuf = safemalloc((MEM_SIZE)(np->numoffset+3+len+1));
		strcpy(mbuf, np->rcline);
	    }
	    cp = mbuf + np->numoffset;
	    *cp++ = ' '; *cp++ = '1'; *cp++ = '-';
	    safecpy(cp, s, len+1);
	    if (np->rcline != mbuf) {
		free(np->rcline);
		np->rcline = mbuf;
	    }
	    np->rc->flags |= RF_RCCHANGED;
	}
    }
    else {
	/* s now points to what should follow the first range */
	char numbuf[32];
	int nlen;

	sprintf(numbuf," 1-%ld",(long)(a1st - (lastnum != a1st)));
	nlen = strlen(numbuf) + (len != 0);

	if (s - np->rcline >= np->numoffset + nlen)
	    mbuf = np->rcline;
	else {
	    mbuf = safemalloc((MEM_SIZE)(np->numoffset+nlen+len+1));
	    strcpy(mbuf,np->rcline);
	}

	cp = mbuf + np->numoffset;
	strcpy(cp, numbuf);
	cp += nlen;

	if (len) {
	    cp[-1] = ',';
	    if (cp != s)
		safecpy(cp,s,len+1);
	}

	if (!checkflag && np->rcline == mbuf)
	    np->rcline = saferealloc(np->rcline, (MEM_SIZE)(cp-mbuf+len+1));
	else {
	    if (!checkflag)
		free(np->rcline);
	    np->rcline = mbuf;
	}
	np->rc->flags |= RF_RCCHANGED;
    }

#ifdef DEBUG
    if (debug & DEB_XREF_MARKER) {
	printf("%s%c%s\n",np->rcline,np->subscribechar,
	  np->rcline + np->numoffset) FLUSH;
    }
#endif
}

/* Returns TRUE if article is marked as read or does not exist */
/* could use a better name */
bool
was_read_group(dp,artnum,ngnam)
DATASRC* dp;
ART_NUM artnum;
char* ngnam;
{
    register NGDATA* np;
    register char* s;
    register char* t;
    register char* maxt = NULL;
    ART_NUM min = 0, max = -1, lastnum = 0;

    if (!artnum)
	return TRUE;
    np = find_ng(ngnam);
    if (np == NULL)			/* not found in newsrc? */
	return TRUE;
    if (!np->numoffset)		/* no numbers on line */
	return FALSE;
#if 0
    /* consider this code later */
    if (!np->abs1st) {
	/* Trim down the list due to expires if we haven't done so yet. */
	set_toread(np, ST_LAX);
    }
#endif

    if (np->toread == TR_BOGUS)
	return TRUE;
    if (artnum > np->ngmax) {
        return FALSE;		/* probably doesn't exist, however */
    }
    s = np->rcline + np->numoffset;
    while (*s == ' ') s++;		/* skip spaces */
    t = s;
    while (isdigit(*s) && artnum >= (min = atol(s))) {
					/* while it might have been read */
	for (t = s; isdigit(*t); t++) ;	/* skip number */
	if (*t == '-') {		/* is it a range? */
	    t++;			/* skip to next number */
	    if (artnum <= (max = atol(t)))
		return TRUE;		/* it is in range => already read */
	    lastnum = max;		/* remember it */
	    maxt = t;			/* remember position in case we */
					/* want to overwrite the max */
	    while (isdigit(*t)) t++;	/* skip second number */
	}
	else {
	    if (artnum == min)		/* explicitly a read article? */
		return TRUE;
	    lastnum = min;		/* remember what the number was */
	    maxt = NULL;		/* last one was not a range */
	}
	while (*t && !isdigit(*t)) t++;	/* skip comma and any spaces */
	s = t;
    }

    /* we have not read it, so return FALSE */
    return FALSE;
}
