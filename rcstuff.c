/* rcstuff.c
 */
/* This software is copyrighted as detailed in the LICENSE file. */


#include "EXTERN.h"
#include "common.h"
#include "list.h"
#include "util.h"
#include "util2.h"
#include "hash.h"
#include "cache.h"
#include "bits.h"
#include "ngdata.h"
#include "nntpclient.h"
#include "datasrc.h"
#include "nntp.h"
#include "term.h"
#include "final.h"
#include "trn.h"
#include "env.h"
#include "init.h"
#include "intrp.h"
#include "only.h"
#include "rcln.h"
#include "last.h"
#include "autosub.h"
#include "rt-select.h"
#include "rt-page.h"
#include "INTERN.h"
#include "rcstuff.h"
#include "rcstuff.ih"

static bool foundany;

bool
rcstuff_init()
{
    MULTIRC* mptr = NULL;
    int i;

    multirc_list = new_list(0,0,sizeof(MULTIRC),20,LF_ZERO_MEM|LF_SPARSE,NULL);

    if (trnaccess_mem) {
	NEWSRC* rp;
	char* s;
	char* section;
	char* cond;
	char** vals = prep_ini_words(rcgroups_ini);
	s = trnaccess_mem;
	while ((s = next_ini_section(s,&section,&cond)) != NULL) {
	    if (*cond && !check_ini_cond(cond))
		continue;
	    if (strncaseNE(section, "group ", 6))
		continue;
	    i = atoi(section+6);
	    if (i < 0)
		i = 0;
	    s = parse_ini_section(s, rcgroups_ini);
	    if (!s)
		break;
	    rp = new_newsrc(vals[RI_ID],vals[RI_NEWSRC],vals[RI_ADDGROUPS]);
	    if (rp) {
		MULTIRC* mp;
		NEWSRC* prev_rp;

		mp = multirc_ptr(i);
		prev_rp = mp->first;
		if (!prev_rp)
		    mp->first = rp;
		else {
		    while (prev_rp->next)
			prev_rp = prev_rp->next;
		    prev_rp->next = rp;
		}
		mp->num = i;
		if (!mptr)
		    mptr = mp;
		/*rp->flags |= RF_USED;*/
	    }
	    /*else
		;$$complain?*/
	}
	free((char*)vals);
	free(trnaccess_mem);
    }

    if (UseNewsrcSelector && !checkflag)
	return TRUE;

    foundany = FALSE;
    if (mptr && !use_multirc(mptr))
	use_next_multirc(mptr);
    if (!multirc) {
	mptr = multirc_ptr(0);
	mptr->first = new_newsrc("default",NULL,NULL);
	if (!use_multirc(mptr)) {
	    printf("Couldn't open any newsrc groups.  Is your access file ok?\n");
	    finalize(1);
	}
    }
    if (checkflag)			/* were we just checking? */
	finalize(foundany);		/* tell them what we found */
    return foundany;
}

NEWSRC*
new_newsrc(name,newsrc,add_ok)
char* name;
char* newsrc;
char* add_ok;
{
    char tmpbuf[CBUFLEN];
    NEWSRC* rp;
    DATASRC* dp;

    if (!name || !*name)
	return NULL;

    if (!newsrc || !*newsrc) {
	newsrc = getenv("NEWSRC");
	if (!newsrc)
	    newsrc = RCNAME;
    }

    dp = get_datasrc(name);
    if (!dp)
	return NULL;

    rp = (NEWSRC*)safemalloc(sizeof (NEWSRC));
    bzero((char*)rp, sizeof (NEWSRC));
    rp->datasrc = dp;
    rp->name = savestr(filexp(newsrc));
    sprintf(tmpbuf, RCNAME_OLD, rp->name);
    rp->oldname = savestr(tmpbuf);
    sprintf(tmpbuf, RCNAME_NEW, rp->name);
    rp->newname = savestr(tmpbuf);

    switch (add_ok? *add_ok : 'y') {
    case 'n':
    case 'N':
	break;
    default:
	if (dp->flags & DF_ADD_OK)
	    rp->flags |= RF_ADD_NEWGROUPS;
	/* FALL THROUGH */
    case 'm':
    case 'M':
	rp->flags |= RF_ADD_GROUPS;
	break;
    }
    return rp;
}

bool
use_multirc(mp)
MULTIRC* mp;
{
    NEWSRC* rp;
    bool had_trouble = FALSE;
    bool had_success = FALSE;

    for (rp = mp->first; rp; rp = rp->next) {
	if ((rp->datasrc->flags & DF_UNAVAILABLE) || !lock_newsrc(rp)
	 || !open_datasrc(rp->datasrc) || !open_newsrc(rp)) {
	    unlock_newsrc(rp);
	    had_trouble = TRUE;
	}
	else {
	    rp->datasrc->flags |= DF_ACTIVE;
	    rp->flags |= RF_ACTIVE;
	    had_success = TRUE;
	}
    }
    if (had_trouble)
	get_anything();
    if (!had_success)
	return FALSE;
    multirc = mp;
#ifdef NO_FILELINKS
    if (!write_newsrcs(multirc))
	get_anything();
#endif
    return TRUE;
}

void
unuse_multirc(mptr)
MULTIRC* mptr;
{
    NEWSRC* rp;

    if (!mptr)
	return;

    write_newsrcs(mptr);

    for (rp = mptr->first; rp; rp = rp->next) {
	unlock_newsrc(rp);
	rp->flags &= ~RF_ACTIVE;
	rp->datasrc->flags &= ~DF_ACTIVE;
    }
    if (ngdata_list) {
	close_cache();
	hashdestroy(newsrc_hash);
	walk_list(ngdata_list, clear_ngitem, 0);
	delete_list(ngdata_list);
	ngdata_list = NULL;
	first_ng = NULL;
	last_ng = NULL;
	ngptr = NULL;
	current_ng = NULL;
	recent_ng = NULL;
	starthere = NULL;
	sel_page_np = NULL;
    }
    ngdata_cnt = 0;
    newsgroup_cnt = 0;
    newsgroup_toread = 0;
    multirc = NULL;
}

bool
use_next_multirc(mptr)
MULTIRC* mptr;
{
    register MULTIRC* mp = multirc_ptr(mptr->num);

    unuse_multirc(mptr);

    for (;;) {
	mp = multirc_next(mp);
	if (!mp)
	    mp = multirc_low();
	if (mp == mptr) {
	    use_multirc(mptr);
	    return FALSE;
	}
	if (use_multirc(mp))
	    break;
    }
    return TRUE;
}

bool
use_prev_multirc(mptr)
MULTIRC* mptr;
{
    register MULTIRC* mp = multirc_ptr(mptr->num);

    unuse_multirc(mptr);

    for (;;) {
	mp = multirc_prev(mp);
	if (!mp)
	    mp = multirc_high();
	if (mp == mptr) {
	    use_multirc(mptr);
	    return FALSE;
	}
	if (use_multirc(mp))
	    break;
    }
    return TRUE;
}

char*
multirc_name(mp)
register MULTIRC* mp;
{
    char* cp;
    if (mp->first->next)
	return "<each-newsrc>";
    if ((cp = rindex(mp->first->name, '/')) != NULL)
	return cp+1;
    return mp->first->name;
}

static bool
clear_ngitem(cp, arg)
char* cp;
int arg;
{
    NGDATA* ncp = (NGDATA*)cp;

    if (ncp->rcline != NULL) {
	if (!checkflag)
	    free(ncp->rcline);
	ncp->rcline = NULL;
    }
    return 0;
}

/* make sure there is no trn out there reading this newsrc */

static bool
lock_newsrc(rp)
NEWSRC* rp;
{
    long processnum = 0;
    char* runninghost = "(Unknown)";
    char* s;

    if (checkflag)
	return TRUE;

    s = filexp(RCNAME);
    if (strEQ(rp->name, s))
	rp->lockname = savestr(filexp(LOCKNAME));
    else {
	sprintf(buf, RCNAME_INFO, rp->name);
	rp->infoname = savestr(buf);
	sprintf(buf, RCNAME_LOCK, rp->name);
	rp->lockname = savestr(buf);
    }

    tmpfp = fopen(rp->lockname,"r");
    if (tmpfp != NULL) {
	if (fgets(buf,LBUFLEN,tmpfp)) {
	    processnum = atol(buf);
	    if (fgets(buf,LBUFLEN,tmpfp) && *buf
	     && *(s = buf + strlen(buf) - 1) == '\n') {
		*s = '\0';
		runninghost = buf;
	    }
	}
	fclose(tmpfp);
    }
    if (processnum) {
#ifndef MSDOS
#ifdef VERBOSE
	IF(verbose)
	    printf("\nThe requested newsrc is locked by process %ld on host %s.\n",
		   processnum, runninghost) FLUSH;
	ELSE
#endif
#ifdef TERSE
	    printf("\nNewsrc locked by %ld on host %s.\n",processnum,runninghost) FLUSH;
#endif
	termdown(2);
	if (strNE(runninghost,localhost)) {
#ifdef VERBOSE
	    IF(verbose)
		printf("\n\
Since that's not the same host as this one (%s), we must\n\
assume that process still exists.  To override this check, remove\n\
the lock file: %s\n", localhost, rp->lockname) FLUSH;
	    ELSE
#endif
#ifdef TERSE
		printf("\nThis host (%s) doesn't match.\nCan't unlock %s.\n",
		       localhost, rp->lockname) FLUSH;
#endif
	    termdown(2);
	    if (bizarre)
		resetty();
	    finalize(0);
	}
	if (processnum == our_pid) {
#ifdef VERBOSE
	    IF(verbose)
		printf("\n\
Hey, that *my* pid!  Your access file is trying to use the same newsrc\n\
more than once.\n") FLUSH;
	    ELSE
#endif
#ifdef TERSE
		printf("\nAccess file error (our pid detected).\n") FLUSH;
#endif
	    termdown(2);
	    return FALSE;
	}
	if (kill(processnum, 0) != 0) {
	    /* Process is apparently gone */
	    sleep(2);
#ifdef VERBOSE
	    IF(verbose)
		fputs("\n\
That process does not seem to exist anymore.  The count of read articles\n\
may be incorrect in the last newsgroup accessed by that other (defunct)\n\
process.\n\n",stdout) FLUSH;
	    ELSE
#endif
#ifdef TERSE
		fputs("\nProcess crashed.\n",stdout) FLUSH;
#endif
	    if (lastngname) {
#ifdef VERBOSE
		IF(verbose)
		    printf("(The last newsgroup accessed was %s.)\n\n",
			   lastngname) FLUSH;
		ELSE
#endif
#ifdef TERSE
		    printf("(In %s.)\n\n",lastngname) FLUSH;
#endif
	    }
	    termdown(2);
	    get_anything();
	    newline();
	}
	else {
#ifdef VERBOSE
	    IF(verbose)
		printf("\n\
It looks like that process still exists.  To override this, remove\n\
the lock file: %s\n", rp->lockname) FLUSH;
	    ELSE
#endif
#ifdef TERSE
		printf("\nCan't unlock %s.\n", rp->lockname) FLUSH;
#endif
	    termdown(2);
	    if (bizarre)
		resetty();
	    finalize(0);
	}
#endif
    }
    tmpfp = fopen(rp->lockname,"w");
    if (tmpfp == NULL) {
	printf(cantcreate,rp->lockname) FLUSH;
	sig_catcher(0);
    }
    fprintf(tmpfp,"%ld\n%s\n",our_pid,localhost);
    fclose(tmpfp);
    return TRUE;
}

static void
unlock_newsrc(rp)
NEWSRC* rp;
{
    safefree0(rp->infoname);
    if (rp->lockname) {
 	UNLINK(rp->lockname);
	free(rp->lockname);
	rp->lockname = NULL;
    }
}

static bool
open_newsrc(rp)
NEWSRC* rp;
{
    register NGDATA* np;
    NGDATA* prev_np;
    char* some_buf;
    long length;
    FILE* rcfp;
    HASHDATUM data;

    /* make sure the .newsrc file exists */

    if ((rcfp = fopen(rp->name,"r")) == NULL) {
	rcfp = fopen(rp->name,"w+");
	if (rcfp == NULL) {
	    printf("\nCan't create %s.\n", rp->name) FLUSH;
	    termdown(2);
	    return FALSE;
	}
	some_buf = SUBSCRIPTIONS;
#ifdef SUPPORT_NNTP
	if ((rp->datasrc->flags & DF_REMOTE)
	 && nntp_list("SUBSCRIPTIONS",nullstr,0) == 1) {
	    do {
		fputs(ser_line,rcfp);
		fputc('\n',rcfp);
		if (nntp_gets(ser_line, sizeof ser_line) < 0)
		    break;
	    } while (!nntp_at_list_end(ser_line));
	}
#endif
	ElseIf (*some_buf && (tmpfp = fopen(filexp(some_buf),"r")) != NULL) {
	    while (fgets(buf,sizeof buf,tmpfp))
		fputs(buf,rcfp);
	    fclose(tmpfp);
	}
	fseek(rcfp, 0L, 0);
    }
    else {
	/* File exists; if zero length and backup isn't, complain */
	if (fstat(fileno(rcfp),&filestat) < 0) {
	    perror(rp->name);
	    return FALSE;
	}
	if (filestat.st_size == 0
	 && stat(rp->oldname,&filestat) >= 0 && filestat.st_size > 0) {
	    printf("Warning: %s is zero length but %s is not.\n",
		   rp->name,rp->oldname);
	    printf("Either recover your newsrc or else remove the backup copy.\n");
	    termdown(2);
	    return FALSE;
	}
	/* unlink backup file name and backup current name */
	UNLINK(rp->oldname);
#ifndef NO_FILELINKS
	safelink(rp->name,rp->oldname);
#endif
    }

    if (!ngdata_list) {
	/* allocate memory for rc file globals */
	ngdata_list = new_list(0, 0, sizeof (NGDATA), 200, 0, init_ngnode);
	newsrc_hash = hashcreate(3001, rcline_cmp);
    }

    if (ngdata_cnt)
	prev_np = ngdata_ptr(ngdata_cnt-1);
    else
	prev_np = NULL;

    /* read in the .newsrc file */

    while ((some_buf = get_a_line(buf,LBUFLEN,FALSE,rcfp)) != NULL) {
	length = len_last_line_got;	/* side effect of get_a_line */
	if (length <= 1)		/* only a newline??? */
	    continue;
	np = ngdata_ptr(ngdata_cnt++);
	if (prev_np)
	    prev_np->next = np;
	else
	    first_ng = np;
	np->prev = prev_np;
	prev_np = np;
	np->rc = rp;
	newsgroup_cnt++;
	if (some_buf[length-1] == '\n')
	    some_buf[--length] = '\0';	/* wipe out newline */
	if (some_buf == buf)
	    np->rcline = savestr(some_buf);  /* make semipermanent copy */
	else {
	    /*NOSTRICT*/
#ifndef lint
	    some_buf = saferealloc(some_buf,(MEM_SIZE)(length+1));
#endif
	    np->rcline = some_buf;
	}
	if (*some_buf == ' ' || *some_buf == '\t'
	 || strnEQ(some_buf,"options",7)) {	/* non-useful line? */
	    np->toread = TR_JUNK;
	    np->subscribechar = ' ';
	    np->numoffset = 0;
	    continue;
	}
	parse_rcline(np);
	data = hashfetch(newsrc_hash, np->rcline, np->numoffset - 1);
	if (data.dat_ptr) {
	    np->toread = TR_IGNORE;
	    continue;
	}
	if (np->subscribechar == NEGCHAR) {
	    np->toread = TR_UNSUB;
	    sethash(np);
	    continue;
	}
	newsgroup_toread++;

	/* now find out how much there is to read */

	if (!inlist(buf) || (suppress_cn && foundany && !paranoid))
	    np->toread = TR_NONE;	/* no need to calculate now */
	else
	    set_toread(np, ST_LAX);
	if (np->toread > TR_NONE) {	/* anything unread? */
	    if (!foundany) {
		starthere = np;
		foundany = TRUE;	/* remember that fact*/
	    }
	    if (suppress_cn) {		/* if no listing desired */
		if (checkflag)		/* if that is all they wanted */
		    finalize(1);	/* then bomb out */
	    }
	    else {
#ifdef VERBOSE
		IF(verbose)
		    printf("Unread news in %-40s %5ld article%s\n",
			np->rcline,(long)np->toread,PLURAL(np->toread)) FLUSH;
		ELSE
#endif
#ifdef TERSE
		    printf("%s: %ld article%s\n",
			np->rcline,(long)np->toread,PLURAL(np->toread)) FLUSH;
#endif
		termdown(1);
		if (int_count) {
		    countdown = 1;
		    int_count = 0;
		}
		if (countdown) {
		    if (!--countdown) {
			fputs("etc.\n",stdout) FLUSH;
			if (checkflag)
			    finalize(1);
			suppress_cn = TRUE;
		    }
		}
	    }
	}
	sethash(np);
    }
    if (prev_np) {
	prev_np->next = NULL;
	last_ng = prev_np;
    }
    fclose(rcfp);			/* close .newsrc */
#ifdef NO_FILELINKS
    UNLINK(rp->oldname);
    RENAME(rp->name,rp->oldname);
    rp->flags |= RF_RCCHANGED;
#endif
    if (rp->infoname) {
	if ((tmpfp = fopen(rp->infoname,"r")) != NULL) {
	    if (fgets(buf,sizeof buf,tmpfp) != NULL) {
		long actnum, descnum;
		char* s;
		buf[strlen(buf)-1] = '\0';
		if ((s = index(buf, ':')) != NULL && s[1] == ' ' && s[2]) {
		    safefree0(lastngname);
		    lastngname = savestr(s+2);
		}
		if (fscanf(tmpfp,"New-Group-State: %ld,%ld,%ld",
			   &lastnewtime,&actnum,&descnum) == 3) {
		    rp->datasrc->act_sf.recent_cnt = actnum;
		    rp->datasrc->desc_sf.recent_cnt = descnum;
		}
	    }
	}
    }
    else {
	readlast();
#ifdef SUPPORT_NNTP
	if (rp->datasrc->flags & DF_REMOTE) {
	    rp->datasrc->act_sf.recent_cnt = lastactsiz;
	    rp->datasrc->desc_sf.recent_cnt = lastextranum;
	}
	else
#endif
	{
	    rp->datasrc->act_sf.recent_cnt = lastextranum;
	    rp->datasrc->desc_sf.recent_cnt = 0;
	}
    }
    rp->datasrc->lastnewgrp = lastnewtime;

    if (paranoid && !checkflag)
	cleanup_newsrc(rp);
    return TRUE;
}

/* Initialize the memory for an entire node's worth of article's */
static void
init_ngnode(list, node)
LIST* list;
LISTNODE* node;
{
    register ART_NUM i;
    register NGDATA* np;
    bzero(node->data, list->items_per_node * list->item_size);
    for (i = node->low, np = (NGDATA*)node->data; i <= node->high; i++, np++)
	np->num = i;
}

static void
parse_rcline(np)
register NGDATA* np;
{
    char* s;
    int len;

    for (s=np->rcline; *s && *s!=':' && *s!=NEGCHAR && !isspace(*s); s++) ;
    len = s - np->rcline;
    if ((!*s || isspace(*s)) && !checkflag) {
#ifndef lint
	np->rcline = saferealloc(np->rcline,(MEM_SIZE)len + 3);
#endif
	s = np->rcline + len;
	strcpy(s, ": ");
    }
    if (*s == ':' && s[1] && s[2] == '0') {
	np->flags |= NF_UNTHREADED;
	s[2] = '1';
    }
    np->subscribechar = *s;		/* salt away the : or ! */
    np->numoffset = len + 1;		/* remember where the numbers are */
    *s = '\0';			/* null terminate newsgroup name */
}

void
abandon_ng(np)
NGDATA* np;
{
    char* some_buf = NULL;
    FILE* rcfp;

    /* open newsrc backup copy and try to find the prior value for the group. */
    if ((rcfp = fopen(np->rc->oldname, "r")) != NULL) {
	int length = np->numoffset - 1;

	while ((some_buf = get_a_line(buf,LBUFLEN,FALSE,rcfp)) != NULL) {
	    if (len_last_line_got <= 0)
		continue;
	    some_buf[len_last_line_got-1] = '\0';	/* wipe out newline */
	    if ((some_buf[length] == ':' || some_buf[length] == NEGCHAR)
	     && strnEQ(np->rcline, some_buf, length)) {
		break;
	    }
	    if (some_buf != buf)
		free(some_buf);
	}
	fclose(rcfp);
    } else if (errno != ENOENT) {
	printf("Unable to open %s.\n", np->rc->oldname) FLUSH;
	termdown(1);
	return;
    }
    if (some_buf == NULL) {
	some_buf = np->rcline + np->numoffset;
	if (*some_buf == ' ')
	    some_buf++;
	*some_buf = '\0';
	np->abs1st = 0;		/* force group to be re-calculated */
    }
    else {
	free(np->rcline);
	if (some_buf == buf)
	    np->rcline = savestr(some_buf);
	else {
	    /*NOSTRICT*/
#ifndef lint
	    some_buf = saferealloc(some_buf, (MEM_SIZE)(len_last_line_got));
#endif /* lint */
	    np->rcline = some_buf;
	}
    }
    parse_rcline(np);
    if (np->subscribechar == NEGCHAR)
	np->subscribechar = ':';
    np->rc->flags |= RF_RCCHANGED;
    set_toread(np, ST_LAX);
}

/* try to find or add an explicitly specified newsgroup */
/* returns TRUE if found or added, FALSE if not. */
/* assumes that we are chdir'ed to NEWSSPOOL */

bool
get_ng(what, flags)
char* what;
int flags;
{
    char* ntoforget;
    char promptbuf[128];
    int autosub;

#ifdef VERBOSE
    IF(verbose)
	ntoforget = "Type n to forget about this newsgroup.\n";
    ELSE
#endif
#ifdef TERSE
	ntoforget = "n to forget it.\n";
#endif
    if (index(what,'/')) {
	dingaling();
	printf("\nBad newsgroup name.\n") FLUSH;
	termdown(2);
      check_fuzzy_match:
#ifdef EDIT_DISTANCE
	if (fuzzyGet && (flags & GNG_FUZZY)) {
	    flags &= ~GNG_FUZZY;
	    if (find_close_match())
		what = ngname;
	    else
		return FALSE;
	} else
#endif
	    return FALSE;
    }
    set_ngname(what);
    ngptr = find_ng(ngname);
    if (ngptr == NULL) {		/* not in .newsrc? */
	NEWSRC* rp;
	for (rp = multirc->first; rp; rp = rp->next) {
	    if (!ALLBITS(rp->flags, RF_ADD_GROUPS | RF_ACTIVE))
		continue;
	    /*$$ this may scan a datasrc multiple times... */
	    if (find_actgrp(rp->datasrc,buf,ngname,ngname_len,(ART_NUM)0))
		break;  /*$$ let them choose which server */
	}
	if (!rp) {
	    dingaling();
#ifdef VERBOSE
	    IF(verbose)
		printf("\nNewsgroup %s does not exist!\n",ngname) FLUSH;
	    ELSE
#endif
#ifdef TERSE
		printf("\nNo %s!\n",ngname) FLUSH;
#endif
	    termdown(2);
	    if (novice_delays)
		sleep(2);
	    goto check_fuzzy_match;
	}
	if (mode != 'i' || !(autosub = auto_subscribe(ngname)))
	    autosub = addnewbydefault;
	if (autosub) {
	    if (append_unsub) {
		printf("(Adding %s to end of your .newsrc %ssubscribed)\n",
		       ngname, (autosub == ADDNEW_SUB)? nullstr : "un") FLUSH;
		termdown(1);
		ngptr = add_newsgroup(rp, ngname, autosub);
	    } else {
		if (autosub == ADDNEW_SUB) {
		    printf("(Subscribing to %s)\n", ngname) FLUSH;
		    termdown(1);
		    ngptr = add_newsgroup(rp, ngname, autosub);
		} else {
		    printf("(Ignoring %s)\n", ngname) FLUSH;
		    termdown(1);
		    return FALSE;
		}
	    }
	    flags &= ~GNG_RELOC;
	} else {
#ifdef VERBOSE
	    IF(verbose)
		sprintf(promptbuf,"\nNewsgroup %s not in .newsrc -- subscribe?",ngname);
	    ELSE
#endif
#ifdef TERSE
		sprintf(promptbuf,"\nSubscribe %s?",ngname);
#endif
reask_add:
	    in_char(promptbuf,'A',"ynYN");
#ifdef VERIFY
	    printcmd();
#endif
	    newline();
	    if (*buf == 'h') {
#ifdef VERBOSE
		IF(verbose) {
		    printf("Type y or SP to subscribe to %s.\n\
Type Y to subscribe to this and all remaining new groups.\n\
Type N to leave all remaining new groups unsubscribed.\n", ngname) FLUSH;
		    termdown(3);
		}
		ELSE
#endif
#ifdef TERSE
		{
		    fputs("\
y or SP to subscribe, Y to subscribe all new groups, N to unsubscribe all\n",
			  stdout) FLUSH;
		    termdown(1);
		}
#endif
		fputs(ntoforget,stdout) FLUSH;
		termdown(1);
		goto reask_add;
	    }
	    else if (*buf == 'n' || *buf == 'q') {
		if (append_unsub)
		    ngptr = add_newsgroup(rp, ngname, NEGCHAR);
		return FALSE;
	    }
	    else if (*buf == 'y') {
		ngptr = add_newsgroup(rp, ngname, ':');
		flags |= GNG_RELOC;
	    }
	    else if (*buf == 'Y') {
		addnewbydefault = ADDNEW_SUB;
		if (append_unsub)
		    printf("(Adding %s to end of your .newsrc subscribed)\n",
			   ngname) FLUSH;
		else
		    printf("(Subscribing to %s)\n", ngname) FLUSH;
		termdown(1);
		ngptr = add_newsgroup(rp, ngname, ':');
		flags &= ~GNG_RELOC;
	    }
	    else if (*buf == 'N') {
		addnewbydefault = ADDNEW_UNSUB;
		if (append_unsub) {
		    printf("(Adding %s to end of your .newsrc unsubscribed)\n",
			   ngname) FLUSH;
		    termdown(1);
		    ngptr = add_newsgroup(rp, ngname, NEGCHAR);
		    flags &= ~GNG_RELOC;
		} else {
		    printf("(Ignoring %s)\n", ngname) FLUSH;
		    termdown(1);
		    return FALSE;
		}
	    }
	    else {
		fputs(hforhelp,stdout) FLUSH;
		termdown(1);
		settle_down();
		goto reask_add;
	    }
	}
    }
    else if (mode == 'i')		/* adding new groups during init? */
	return FALSE;
    else if (ngptr->subscribechar == NEGCHAR) {/* unsubscribed? */
#ifdef VERBOSE
	IF(verbose)
	    sprintf(promptbuf,
"\nNewsgroup %s is unsubscribed -- resubscribe?",ngname)
  FLUSH;
	ELSE
#endif
#ifdef TERSE
	    sprintf(promptbuf,"\nResubscribe %s?",ngname)
	      FLUSH;
#endif
reask_unsub:
	in_char(promptbuf,'R',"yn");
#ifdef VERIFY
	printcmd();
#endif
	newline();
	if (*buf == 'h') {
#ifdef VERBOSE
	    IF(verbose)
		printf("Type y or SP to resubscribe to %s.\n", ngname) FLUSH;
	    ELSE
#endif
#ifdef TERSE
		fputs("y or SP to resubscribe.\n",stdout) FLUSH;
#endif
	    fputs(ntoforget,stdout) FLUSH;
	    termdown(2);
	    goto reask_unsub;
	}
	else if (*buf == 'n' || *buf == 'q') {
	    return FALSE;
	}
	else if (*buf == 'y') {
	    register char* cp;
	    cp = ngptr->rcline + ngptr->numoffset;
	    ngptr->flags = (*cp && cp[1] == '0' ? NF_UNTHREADED : 0);
	    ngptr->subscribechar = ':';
	    ngptr->rc->flags |= RF_RCCHANGED;
	    flags &= ~GNG_RELOC;
	}
	else {
	    fputs(hforhelp,stdout) FLUSH;
	    termdown(1);
	    settle_down();
	    goto reask_unsub;
	}
    }

    /* now calculate how many unread articles in newsgroup */

    set_toread(ngptr, ST_STRICT);
#ifdef RELOCATE
    if (flags & GNG_RELOC) {
	if (!relocate_newsgroup(ngptr,-1))
	    return FALSE;
    }
#endif
    return ngptr->toread >= TR_NONE;
}

/* add a newsgroup to the newsrc file (eventually) */

static NGDATA*
add_newsgroup(rp, ngn, c)
NEWSRC* rp;
char* ngn;
char_int c;
{
    register NGDATA* np;

    np = ngdata_ptr(ngdata_cnt++);
    np->prev = last_ng;
    if (last_ng)
	last_ng->next = np;
    else
	first_ng = np;
    np->next = NULL;
    last_ng = np;
    newsgroup_cnt++;

    np->rc = rp;
    np->numoffset = strlen(ngn) + 1;
    np->rcline = safemalloc((MEM_SIZE)(np->numoffset + 2));
    strcpy(np->rcline,ngn);		/* and copy over the name */
    strcpy(np->rcline + np->numoffset, " ");
    np->subscribechar = c;		/* subscribe or unsubscribe */
    if (c != NEGCHAR)
	newsgroup_toread++;
    np->toread = TR_NONE;		/* just for prettiness */
    sethash(np);			/* so we can find it again */
    rp->flags |= RF_RCCHANGED;
    return np;
}

#ifdef RELOCATE
bool
relocate_newsgroup(move_np,newnum)
NGDATA* move_np;
NG_NUM newnum;
{
    NGDATA* np;
    int i;
    char* dflt = (move_np!=current_ng ? "$^.Lq" : "$^Lq");
    int save_sort = sel_sort;

    if (sel_newsgroupsort != SS_NATURAL) {
	if (newnum < 0) {
	    /* ask if they want to keep the current order */
	    in_char("Sort newsrc(s) using current sort order?", 'D', "yn"); /*$$ !'D' */
#ifdef VERIFY
	    printcmd();
#endif
	    newline();
	    if (*buf == 'y')
		set_selector(SM_NEWSGROUP, SS_NATURAL);
	    else {
		sel_sort = SS_NATURAL;
		sel_direction = 1;
		sort_newsgroups();
	    }
	}
	else {
	    sel_sort = SS_NATURAL;
	    sel_direction = 1;
	    sort_newsgroups();
	}
    }

    starthere = NULL;			/* Disable this optimization */
    if (move_np != last_ng) {
	if (move_np->prev)
	    move_np->prev->next = move_np->next;
	else
	    first_ng = move_np->next;
	move_np->next->prev = move_np->prev;

	move_np->prev = last_ng;
	move_np->next = NULL;
	last_ng->next = move_np;
	last_ng = move_np;
    }

    /* Renumber the groups according to current order */
    for (np = first_ng, i = 0; np; np = np->next, i++)
	np->num = i;
    move_np->rc->flags |= RF_RCCHANGED;

    if (newnum < 0) {
      reask_reloc:
	unflush_output();		/* disable any ^O in effect */
#ifdef VERBOSE
	IF(verbose)
	    printf("\nPut newsgroup where? [%s] ", dflt);
	ELSE
#endif
#ifdef TERSE
	    printf("\nPut where? [%s] ", dflt);
#endif
	fflush(stdout);
	termdown(1);
      reinp_reloc:
	eat_typeahead();
	getcmd(buf);
	if (errno || *buf == '\f')	/* if return from stop signal */
	    goto reask_reloc;		/* give them a prompt again */
	setdef(buf,dflt);
#ifdef VERIFY
	printcmd();
#endif
	if (*buf == 'h') {
#ifdef VERBOSE
	    IF(verbose) {
		printf("\n\n\
Type ^ to put the newsgroup first (position 0).\n\
Type $ to put the newsgroup last (position %d).\n", newsgroup_cnt-1);
		printf("\
Type . to put it before the current newsgroup.\n");
		printf("\
Type -newsgroup name to put it before that newsgroup.\n\
Type +newsgroup name to put it after that newsgroup.\n\
Type a number between 0 and %d to put it at that position.\n", newsgroup_cnt-1);
		printf("\
Type L for a listing of newsgroups and their positions.\n\
Type q to abort the current action.\n") FLUSH;
	    }
	    ELSE
#endif
#ifdef TERSE
	    {
		printf("\n\n\
^ to put newsgroup first (pos 0).\n\
$ to put last (pos %d).\n", newsgroup_cnt-1);
		printf("\
. to put before current newsgroup.\n");
		printf("\
-newsgroup to put before newsgroup.\n\
+newsgroup to put after.\n\
number in 0-%d to put at that pos.\n", newsgroup_cnt-1);
		printf("\
L for list of newsrc.\n\
q to abort\n") FLUSH;
	    }
#endif
	    termdown(10);
	    goto reask_reloc;
	}
	else if (*buf == 'q')
	    return FALSE;
	else if (*buf == 'L') {
	    newline();
	    list_newsgroups();
	    goto reask_reloc;
	}
	else if (isdigit(*buf)) {
	    if (!finish_command(TRUE))	/* get rest of command */
		goto reinp_reloc;
	    newnum = atol(buf);
	    if (newnum < 0)
		newnum = 0;
	    if (newnum >= newsgroup_cnt)
		newnum = newsgroup_cnt-1;
	}
	else if (*buf == '^') {
	    newline();
	    newnum = 0;
	}
	else if (*buf == '$') {
	    newnum = newsgroup_cnt-1;
	}
	else if (*buf == '.') {
	    newline();
	    newnum = current_ng->num;
	}
	else if (*buf == '-' || *buf == '+') {
	    if (!finish_command(TRUE))	/* get rest of command */
		goto reinp_reloc;
	    np = find_ng(buf+1);
	    if (np == NULL) {
		fputs("Not found.",stdout) FLUSH;
		goto reask_reloc;
	    }
	    newnum = np->num;
	    if (*buf == '+')
		newnum++;
	}
	else {
	    printf("\n%s",hforhelp) FLUSH;
	    termdown(2);
	    settle_down();
	    goto reask_reloc;
	}
    }
    if (newnum < newsgroup_cnt-1) {
	for (np = first_ng; np; np = np->next)
	    if (np->num >= newnum)
		break;
	if (!np || np == move_np)
	    return FALSE;		/* This can't happen... */

	last_ng = move_np->prev;
	last_ng->next = NULL;

	move_np->prev = np->prev;
	move_np->next = np;

	if (np->prev)
	    np->prev->next = move_np;
	else
	    first_ng = move_np;
	np->prev = move_np;

	move_np->num = newnum++;
	for (; np; np = np->next, newnum++)
	    np->num = newnum;
    }
    if (sel_newsgroupsort != SS_NATURAL) {
	sel_sort = sel_newsgroupsort;
	sort_newsgroups();
	sel_sort = save_sort;
    }
    return TRUE;
}
#endif /* RELOCATE */

/* List out the newsrc with annotations */

void
list_newsgroups()
{
    register NGDATA* np;
    register NG_NUM i;
    char tmpbuf[2048];
    static char* status[] = {"(READ)","(UNSUB)","(DUP)","(BOGUS)","(JUNK)"};

    page_start();
    print_lines("  #  Status  Newsgroup\n",STANDOUT);
    for (np = first_ng, i = 0; np && !int_count; np = np->next, i++) {
	if (np->toread >= 0)
	    set_toread(np, ST_LAX);
	*(np->rcline + np->numoffset - 1) = np->subscribechar;
	if (np->toread > 0)
	    sprintf(tmpbuf,"%3d %6ld   ",i,(long)np->toread);
	else
	    sprintf(tmpbuf,"%3d %7s  ",i,status[-np->toread]);
	safecpy(tmpbuf+13, np->rcline, sizeof tmpbuf - 13);
	*(np->rcline + np->numoffset - 1) = '\0';
	if (print_lines(tmpbuf,NOMARKING) != 0)
	    break;
    }
    int_count = 0;
}

/* find a newsgroup in any newsrc */

NGDATA*
find_ng(ngnam)
char* ngnam;
{
    HASHDATUM data;

    data = hashfetch(newsrc_hash, ngnam, strlen(ngnam));
    return (NGDATA*)data.dat_ptr;
}

void
cleanup_newsrc(rp)
NEWSRC* rp;
{
    register NGDATA* np;
    register NG_NUM bogosity = 0;

#ifdef VERBOSE
    IF(verbose)
	printf("Checking out '%s' -- hang on a second...\n",rp->name) FLUSH;
    ELSE
#endif
#ifdef TERSE
	printf("Checking '%s' -- hang on...\n",rp->name) FLUSH;
#endif
    termdown(1);
    for (np = first_ng; np; np = np->next) {
/*#ifdef CHECK_ALL_BOGUS $$ what is this? */
	if (np->toread >= TR_UNSUB)
	    set_toread(np, ST_LAX); /* this may reset the group or declare it bogus */
/*#endif*/
	if (np->toread == TR_BOGUS)
	    bogosity++;
    }
    for (np = last_ng; np && np->toread == TR_BOGUS; np = np->prev)
	bogosity--;			/* discount already moved ones */
    if (newsgroup_cnt > 5 && bogosity > newsgroup_cnt / 2) {
	fputs(
"It looks like the active file is messed up.  Contact your news administrator,\n\
",stdout);
	fputs(
"leave the \"bogus\" groups alone, and they may come back to normal.  Maybe.\n\
",stdout) FLUSH;
	termdown(2);
    }
#ifdef RELOCATE
    else if (bogosity) {
	NGDATA* prev_np;
#ifdef VERBOSE
	IF(verbose)
	    printf("Moving bogus newsgroups to the end of '%s'.\n",rp->name) FLUSH;
	ELSE
#endif
#ifdef TERSE
	    fputs("Moving boguses to the end.\n",stdout) FLUSH;
#endif
	termdown(1);
	while (np) {
	    prev_np = np->prev;
	    if (np->toread == TR_BOGUS)
		relocate_newsgroup(np, newsgroup_cnt-1);
	    np = prev_np;
	}
	rp->flags |= RF_RCCHANGED;
#ifdef DELBOGUS
reask_bogus:
	in_char("Delete bogus newsgroups?", 'D', "ny");
#ifdef VERIFY
	printcmd();
#endif
	newline();
	if (*buf == 'h') {
#ifdef VERBOSE
	    IF(verbose) {
		fputs("\
Type y to delete bogus newsgroups.\n\
Type n or SP to leave them at the end in case they return.\n\
",stdout) FLUSH;
		termdown(2);
	    }
	    ELSE
#endif
#ifdef TERSE
	    {
		fputs("y to delete, n to keep\n",stdout) FLUSH;
		termdown(1);
	    }
#endif
	    goto reask_bogus;
	}
	else if (*buf == 'n' || *buf == 'q')
	    ;
	else if (*buf == 'y') {
	    for (np = last_ng; np && np->toread == TR_BOGUS; np = np->prev) {
		hashdelete(newsrc_hash, np->rcline, np->numoffset - 1);
		clear_ngitem((char*)np,0);
		newsgroup_cnt--;
	    }
	    rp->flags |= RF_RCCHANGED; /*$$ needed? */
	    last_ng = np;
	    if (np)
		np->next = NULL;
	    else
		first_ng = NULL;
	    if (current_ng && !current_ng->rcline)
		current_ng = first_ng;
	    if (recent_ng && !recent_ng->rcline)
		recent_ng = first_ng;
	    if (ngptr && !ngptr->rcline)
		ngptr = first_ng;
	    if (sel_page_np && !sel_page_np->rcline)
		sel_page_np = NULL;
	}
	else {
	    fputs(hforhelp,stdout) FLUSH;
	    termdown(1);
	    settle_down();
	    goto reask_bogus;
	}
#endif /* DELBOGUS */
    }
#else /* !RELOCATE */
#ifdef VERBOSE
    IF(verbose)
	printf("You should edit bogus newsgroups out of '%s'.\n",rp->name) FLUSH;
    ELSE
#endif
#ifdef TERSE
	printf("Edit boguses from '%s'.\n",rp->name) FLUSH;
#endif
    termdown(1);
#endif /* !RELOCATE */
    paranoid = FALSE;
}

/* make an entry in the hash table for the current newsgroup */

void
sethash(np)
NGDATA* np;
{
    HASHDATUM data;
    data.dat_ptr = (char*)np;
    data.dat_len = np->numoffset - 1;
    hashstore(newsrc_hash, np->rcline, data.dat_len, data);
}

static int
rcline_cmp(key, keylen, data)
char* key;
int keylen;
HASHDATUM data;
{
    /* We already know that the lengths are equal, just compare the strings */
    return bcmp(key, ((NGDATA*)data.dat_ptr)->rcline, keylen);
}

/* checkpoint the newsrc(s) */

void
checkpoint_newsrcs()
{
#ifdef DEBUG
    if (debug & DEB_CHECKPOINTING) {
	fputs("(ckpt)",stdout);
	fflush(stdout);
    }
#endif
    if (doing_ng)
	bits_to_rc();			/* do not restore M articles */
    if (!write_newsrcs(multirc))
	get_anything();
#ifdef DEBUG
    if (debug & DEB_CHECKPOINTING) {
	fputs("(done)",stdout);
	fflush(stdout);
    }
#endif
}

/* write out the (presumably) revised newsrc(s) */

bool
write_newsrcs(mptr)
MULTIRC* mptr;
{
    NEWSRC* rp;
    register NGDATA* np;
    int save_sort = sel_sort;
    FILE* rcfp;
    bool total_success = TRUE;

    if (!mptr)
	return TRUE;

    if (sel_newsgroupsort != SS_NATURAL) {
	sel_sort = SS_NATURAL;
	sel_direction = 1;
	sort_newsgroups();
    }

    for (rp = mptr->first; rp; rp = rp->next) {
	if (!(rp->flags & RF_ACTIVE))
	    continue;

	if (rp->infoname) {
	    if ((tmpfp = fopen(rp->infoname, "w")) != NULL) {
		fprintf(tmpfp,"Last-Group: %s\nNew-Group-State: %ld,%ld,%ld\n",
			ngname? ngname : nullstr,rp->datasrc->lastnewgrp,
			rp->datasrc->act_sf.recent_cnt,
			rp->datasrc->desc_sf.recent_cnt);
		fclose(tmpfp);
	    }
	}
	else {
	    readlast();
#ifdef SUPPORT_NNTP
	    if (rp->datasrc->flags & DF_REMOTE) {
		lastactsiz = rp->datasrc->act_sf.recent_cnt;
		lastextranum = rp->datasrc->desc_sf.recent_cnt;
	    }
	    else
#endif
		lastextranum = rp->datasrc->act_sf.recent_cnt;
	    lastnewtime = rp->datasrc->lastnewgrp;
	    writelast();
	}

	if (!(rp->flags & RF_RCCHANGED))
	    continue;

	rcfp = fopen(rp->newname, "w");
	if (rcfp == NULL) {
	    printf(cantrecreate,rp->name) FLUSH;
	    total_success = FALSE;
	    continue;
	}
#ifndef MSDOS
	if (stat(rp->name,&filestat)>=0) { /* preserve permissions */
	    chmod(rp->newname,filestat.st_mode&0666);
	    chown(rp->newname,filestat.st_uid,filestat.st_gid);
	}
#endif
	/* write out each line*/

	for (np = first_ng; np; np = np->next) {
	    register char* delim;
	    if (np->rc != rp)
		continue;
	    if (np->numoffset) {
		delim = np->rcline + np->numoffset - 1;
		*delim = np->subscribechar;
		if ((np->flags & NF_UNTHREADED) && delim[2] == '1')
		    delim[2] = '0';
	    }
	    else
		delim = NULL;
#ifdef DEBUG
	    if (debug & DEB_NEWSRC_LINE) {
		printf("%s\n",np->rcline) FLUSH;
		termdown(1);
	    }
#endif
	    if (fprintf(rcfp,"%s\n",np->rcline) < 0) {
		fclose(rcfp);		/* close new newsrc */
		goto write_error;
	    }
	    if (delim) {
		*delim = '\0';		/* might still need this line */
		if ((np->flags & NF_UNTHREADED) && delim[2] == '0')
		    delim[2] = '1';
	    }
	}
	fflush(rcfp);
	/* fclose is the only sure test for full disks via NFS */
	if (ferror(rcfp)) {
	    fclose(rcfp);
	    goto write_error;
	}
	if (fclose(rcfp) == EOF) {
	  write_error:
	    printf(cantrecreate,rp->name) FLUSH;
	    UNLINK(rp->newname);
	    total_success = FALSE;
	    continue;
	}
	rp->flags &= ~RF_RCCHANGED;

	UNLINK(rp->name);
	RENAME(rp->newname,rp->name);
    }

    if (sel_newsgroupsort != SS_NATURAL) {
	sel_sort = sel_newsgroupsort;
	sort_newsgroups();
	sel_sort = save_sort;
    }
    return total_success;
}

void
get_old_newsrcs(mptr)
MULTIRC* mptr;
{
    NEWSRC* rp;

    if (mptr) {
	for (rp = mptr->first; rp; rp = rp->next) {
	    if (rp->flags & RF_ACTIVE) {
		UNLINK(rp->newname);
		RENAME(rp->name,rp->newname);
		RENAME(rp->oldname,rp->name);
	    }
	}
    }
}
