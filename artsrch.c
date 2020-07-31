/* artsrch.c
 */
/* This software is copyrighted as detailed in the LICENSE file. */


#include "EXTERN.h"
#include "common.h"
#include "list.h"
#include "hash.h"
#include "ngdata.h"
#include "nntpclient.h"
#include "datasrc.h"
#include "nntp.h"
#include "search.h"
#include "term.h"
#include "util.h"
#include "util2.h"
#include "intrp.h"
#include "cache.h"
#include "bits.h"
#include "kfile.h"
#include "head.h"
#include "final.h"
#include "ng.h"
#include "addng.h"
#include "ngstuff.h"
#include "artio.h"
#include "rthread.h"
#include "rt-util.h"
#include "rt-select.h"
#include "INTERN.h"
#include "artsrch.h"

void
artsrch_init()
{
#ifdef ARTSEARCH
    init_compex(&sub_compex);
    init_compex(&art_compex);
#endif
}

/* search for an article containing some pattern */

#ifdef ARTSEARCH
int
art_search(patbuf,patbufsiz,get_cmd)
char* patbuf;				/* if patbuf != buf, get_cmd must */
int patbufsiz;
int get_cmd;				/*   be set to FALSE!!! */
{
    char* pattern;			/* unparsed pattern */
    register char cmdchr = *patbuf;	/* what kind of search? */
    register char* s;
    bool backward = cmdchr == '?' || cmdchr == Ctl('p');
					/* direction of search */
    COMPEX* compex;			/* which compiled expression */
    char* cmdlst = NULL;		/* list of commands to do */
    int ret = SRCH_NOTFOUND;		/* assume no commands */
    int saltaway = 0;			/* store in KILL file? */
    int howmuch;			/* search scope: subj/from/Hdr/head/art */
    int srchhdr;			/* header to search if Hdr scope */
    bool topstart = 0;
    bool doread;			/* search read articles? */
    bool foldcase = TRUE;		/* fold upper and lower case? */
    int ignorethru = 0;			/* should we ignore the thru line? */
    bool output_level = (!use_threads && gmode != 's');
    ART_NUM srchfirst;

    int_count = 0;
    if (cmdchr == '/' || cmdchr == '?') {	/* normal search? */
	if (get_cmd && buf == patbuf)
	    if (!finish_command(FALSE))	/* get rest of command */
		return SRCH_ABORT;
	compex = &art_compex;
	if (patbuf[1]) {
	    howmuch = ARTSCOPE_SUBJECT;
	    srchhdr = SOME_LINE;
	    doread = FALSE;
	}
	else {
	    howmuch = art_howmuch;
	    srchhdr = art_srchhdr;
	    doread = art_doread;
	}
	s = cpytill(buf,patbuf+1,cmdchr);/* ok to cpy buf+1 to buf */
	pattern = buf;
	if (*pattern) {
	    if (*lastpat)
		free(lastpat);
	    lastpat = savestr(pattern);
	}
	if (*s) {			/* modifiers or commands? */
	    while (*++s) {
		switch (*s) {
		case 'f':		/* scan the From line */
		    howmuch = ARTSCOPE_FROM;
		    break;
		case 'H':		/* scan a specific header */
		    howmuch = ARTSCOPE_ONEHDR;
		    s = cpytill(msg, s+1, ':');
		    srchhdr = get_header_num(msg);
		    goto loop_break;
		case 'h':		/* scan header */
		    howmuch = ARTSCOPE_HEAD;
		    break;
		case 'b':		/* scan body sans signature */
		    howmuch = ARTSCOPE_BODY_NOSIG;
		    break;
		case 'B':		/* scan body */
		    howmuch = ARTSCOPE_BODY;
		    break;
		case 'a':		/* scan article */
		    howmuch = ARTSCOPE_ARTICLE;
		    break;
		case 't':		/* start from the top */
		    topstart = TRUE;
		    break;
		case 'r':		/* scan read articles */
		    doread = TRUE;
		    break;
		case 'K':		/* put into KILL file */
		    saltaway = 1;
		    break;
		case 'c':		/* make search case sensitive */
		    foldcase = FALSE;
		    break;
		case 'I':		/* ignore the killfile thru line */
		    ignorethru = 1;
		    break;
		case 'N':		/* override ignore if -k was used */
		    ignorethru = -1;
		    break;
		default:
		    goto loop_break;
		}
	    }
	  loop_break:;
	}
	while (isspace(*s) || *s == ':') s++;
	if (*s) {
#ifdef OLD_RN_WAY
	    if (*s == 'm' || *s == 'M')
#else
	    if (*s == 'm')
#endif
		doread = TRUE;
	    if (*s == 'k')		/* grandfather clause */
		*s = 'j';
	    cmdlst = savestr(s);
	    ret = SRCH_DONE;
	}
	art_howmuch = howmuch;
	art_srchhdr = srchhdr;
	art_doread = doread;
	if (srchahead)
	    srchahead = -1;
    }
    else {
	register char* h;
	int saltmode = patbuf[2] == 'g'? 2 : 1;
	char *finding_str = patbuf[1] == 'f'? "author" : "subject";

	howmuch = patbuf[1] == 'f'? ARTSCOPE_FROM : ARTSCOPE_SUBJECT;
	srchhdr = SOME_LINE;
	doread = (cmdchr == Ctl('p'));
	if (cmdchr == Ctl('n'))
	    ret = SRCH_SUBJDONE;
	compex = &sub_compex;
	pattern = patbuf+1;
	if (howmuch == ARTSCOPE_SUBJECT) {
	    strcpy(pattern,": *");
	    h = pattern + strlen(pattern);
	    interp(h,patbufsiz - (h-patbuf),"%\\s");  /* fetch current subject */
	}
	else {
	    h = pattern;
	    /*$$ if using thread files, make this "%\\)f" */
	    interp(pattern, patbufsiz - 1, "%\\>f");
	}
	if (cmdchr == 'k' || cmdchr == 'K' || cmdchr == ','
	 || cmdchr == '+' || cmdchr == '.' || cmdchr == 's') {
	    if (cmdchr != 'k')
		saltaway = saltmode;
	    ret = SRCH_DONE;
	    if (cmdchr == '+') {
		cmdlst = savestr("+");
		if (!ignorethru && kill_thru_kludge)
		    ignorethru = 1;
	    }
	    else if (cmdchr == '.') {
		cmdlst = savestr(".");
		if (!ignorethru && kill_thru_kludge)
		    ignorethru = 1;
	    }
	    else if (cmdchr == 's') {
		cmdlst = savestr(patbuf);
		/*ignorethru = 1;*/
	    }
	    else {
		if (cmdchr == ',')
		    cmdlst = savestr(",");
		else
		    cmdlst = savestr("j");
		mark_as_read(article_ptr(art));	/* this article needs to die */
	    }
	    if (!*h) {
#ifdef VERBOSE
		IF(verbose)
		    sprintf(msg, "Current article has no %s.", finding_str);
		ELSE
#endif
#ifdef TERSE
		    sprintf(msg, "Null %s.", finding_str);
#endif
		errormsg(msg);
		ret = SRCH_ABORT;
		goto exit;
	    }
#ifdef VERBOSE
	    if (verbose) {
		if (cmdchr != '+' && cmdchr != '.')
		    printf("\nMarking %s \"%s\" as read.\n",finding_str,h) FLUSH;
		else
		    printf("\nSelecting %s \"%s\".\n",finding_str,h) FLUSH;
		termdown(2);
	    }
#endif
	}
	else if (!srchahead)
	    srchahead = -1;

	{			/* compensate for notesfiles */
	    register int i;
	    for (i = 24; *h && i--; h++)
		if (*h == '\\')
		    h++;
	    *h = '\0';
	}
#ifdef DEBUG
	if (debug) {
	    printf("\npattern = %s\n",pattern) FLUSH;
	    termdown(2);
	}
#endif
    }
    if ((s = compile(compex,pattern,TRUE,foldcase)) != NULL) {
					/* compile regular expression */
	errormsg(s);
	ret = SRCH_ABORT;
	goto exit;
    }
    if (cmdlst && index(cmdlst,'='))
	ret = SRCH_ERROR;		/* listing subjects is an error? */
    if (gmode == 's') {
	if (!cmdlst) {
	    if (sel_mode == SM_ARTICLE)/* set the selector's default command */
		cmdlst = savestr("+");
	    else
		cmdlst = savestr("++");
	}
	ret = SRCH_DONE;
    }
#ifdef KILLFILES
    if (saltaway) {
	char saltbuf[LBUFLEN], *f;

	s = saltbuf;
	f = pattern;
	*s++ = '/';
	while (*f) {
	    if (*f == '/')
		*s++ = '\\';
	    *s++ = *f++;
	}
	*s++ = '/';
	if (doread)
	    *s++ = 'r';
	if (!foldcase)
	    *s++ = 'c';
	if (ignorethru)
	    *s++ = (ignorethru == 1 ? 'I' : 'N');
	if (howmuch != ARTSCOPE_SUBJECT) {
	    *s++ = scopestr[howmuch];
	    if (howmuch == ARTSCOPE_ONEHDR) {
		safecpy(s,htype[srchhdr].name,LBUFLEN-(s-saltbuf));
		s += htype[srchhdr].length;
		if (s - saltbuf > LBUFLEN-2)
		    s = saltbuf+LBUFLEN-2;
	    }
	}
	*s++ = ':';
	if (!cmdlst)
	    cmdlst = savestr("j");
	safecpy(s,cmdlst,LBUFLEN-(s-saltbuf));
	kf_append(saltbuf, saltaway == 2? KF_GLOBAL : KF_LOCAL);
    }
#endif
    if (get_cmd) {
	if (use_threads)
	    newline();
	else {
	    fputs("\nSearching...\n",stdout) FLUSH;
	    termdown(2);
	}
					/* give them something to read */
    }
    if (ignorethru == 0 && kill_thru_kludge && cmdlst
     && (*cmdlst == '+' || *cmdlst == '.'))
	ignorethru = 1;
    srchfirst = doread || sel_rereading? absfirst
		      : (mode != 'k' || ignorethru > 0)? firstart : killfirst;
    if (topstart || art == 0) {
	art = lastart+1;
	topstart = FALSE;
    }
    if (backward) {
	if (cmdlst && art <= lastart)
	    art++;			/* include current article */
    }
    else {
	if (art > lastart)
	    art = srchfirst-1;
	else if (cmdlst && art >= absfirst)
	    art--;			/* include current article */
    }
    if (srchahead > 0) {
	if (!backward)
	    art = srchahead - 1;
	srchahead = -1;
    }
    assert(!cmdlst || *cmdlst);
    perform_status_init(ngptr->toread);
    for (;;) {
	/* check if we're out of articles */
	if (backward? ((art = article_prev(art)) < srchfirst)
		    : ((art = article_next(art)) > lastart))
	    break;
	if (int_count) {
	    int_count = 0;
	    ret = SRCH_INTR;
	    break;
	}
	artp = article_ptr(art);
	if (doread || (!(artp->flags & AF_UNREAD) ^ !sel_rereading)) {
	    if (wanted(compex,art,howmuch)) {
				    /* does the shoe fit? */
		if (!cmdlst)
		    return SRCH_FOUND;
		if (perform(cmdlst,output_level && page_line == 1) < 0) {
		    free(cmdlst);
		    return SRCH_INTR;
		}
	    }
	    else if (output_level && !cmdlst && !(art%50)) {
		printf("...%ld",(long)art);
		fflush(stdout);
	    }
	}
	if (!output_level && page_line == 1)
	    perform_status(ngptr->toread, 60 / (howmuch+1));
    }
exit:
    if (cmdlst)
	free(cmdlst);
    return ret;
}
#endif /* ARTSEARCH */

/* determine if article fits pattern */
/* returns TRUE if it exists and fits pattern, FALSE otherwise */

#ifdef ARTSEARCH
bool
wanted(compex, artnum, scope)
COMPEX* compex;
ART_NUM artnum;
char_int scope;
{
    ARTICLE* ap = article_find(artnum);

    if (!ap || !(ap->flags & AF_EXISTS))
	return FALSE;

    switch (scope) {
      case ARTSCOPE_SUBJECT:
	strcpy(buf,"Subject: ");
	strncpy(buf+9,fetchsubj(artnum,FALSE),256);
#ifdef DEBUG
	if (debug & DEB_SEARCH_AHEAD)
	    printf("%s\n",buf) FLUSH;
#endif
	break;
      case ARTSCOPE_FROM:
	strcpy(buf, "From: ");
	strncpy(buf+6,fetchfrom(artnum,FALSE),256);
	break;
      case ARTSCOPE_ONEHDR:
	untrim_cache = TRUE;
	sprintf(buf, "%s: %s", htype[art_srchhdr].name,
		prefetchlines(artnum,art_srchhdr,FALSE));
	untrim_cache = FALSE;
	break;
      default: {
	char* s;
	char* nlptr;
	char ch;
	bool success = FALSE, in_sig = FALSE;
	if (scope != ARTSCOPE_BODY && scope != ARTSCOPE_BODY_NOSIG) {
	    if (!parseheader(artnum))
		return FALSE;
	    /* see if it's in the header */
	    if (execute(compex,headbuf))	/* does it match? */
		return TRUE;			/* say, "Eureka!" */
	    if (scope < ARTSCOPE_ARTICLE)
		return FALSE;
	}
	if (parsed_art == artnum) {
	    if (!artopen(artnum,htype[PAST_HEADER].minpos))
		return FALSE;
	}
	else {
	    if (!artopen(artnum,(ART_POS)0))
		return FALSE;
	    if (!parseheader(artnum))
		return FALSE;
	}
	/* loop through each line of the article */
	seekartbuf(htype[PAST_HEADER].minpos);
	while ((s = readartbuf(FALSE)) != NULL) {
	    if (scope == ARTSCOPE_BODY_NOSIG && *s == '-' && s[1] == '-'
	     && (s[2] == '\n' || (s[2] == ' ' && s[3] == '\n'))) {
		if (in_sig && success)
		    return TRUE;
		in_sig = TRUE;
	    }
	    if ((nlptr = index(s,'\n')) != NULL) {
		ch = *++nlptr;
		*nlptr = '\0';
	    }
	    success = success || execute(compex,s) != NULL;
	    if (nlptr)
		*nlptr = ch;
	    if (success && !in_sig)		/* does it match? */
		return TRUE;			/* say, "Eureka!" */
	}
	return FALSE;			/* out of article, so no match */
      }
    }
    return execute(compex,buf) != NULL;
}
#endif /* ARTSEARCH */
