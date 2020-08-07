/* This software is copyrighted as detailed in the LICENSE file. */

/*  trn -- threaded readnews program based on rn 4.4
 *
 *  You can request help from:  trn-users@lists.sourceforge.net
 *  Send bugs, suggestions, etc. to:  trn-workers@lists.sourceforge.net
 *
 *  Author/Maintainer of trn: trn@blorf.net (Wayne Davison)
 *  Maintainer of rn: sob@bcm.tmc.edu (Stan Barber)
 *  Original Author: lwall@sdcrdcf.UUCP (Larry Wall)
 *
 *  History:
 *	01/14/83 - rn begun
 *	04/08/83 - rn 1.0
 *	09/01/83 - rn 2.0
 *	05/01/85 - rn 4.3
 *	11/01/89 - rn/rrn integration
 *	11/25/89 - trn begun
 *	07/21/90 - trn 1.0
 *	07/04/91 - rn 4.4
 *	11/25/91 - trn 2.0
 *	07/25/93 - trn 3.0
 *	??/??/?? - trn 4.0
 *
 *  strn -- Scan(-mode)/Scoring TRN
 *
 *  Author/Maintainer of strn: caadams@zynet.com (Clifford A. Adams)
 *
 *  Strn history:
 *      Dec.  90  - "Keyword RN" initial ideas, keyword entry prototype
 *	01/16/91  - "Scoring RN" initial design notes
 *      Late  91  - cleaned up "Semicolon mode" RN patches from Todd Day
 *      Early 92  - major additions to "STRN"
 *	Mid   93  - first strn public release (version 0.8)
 *	Sep.  94  - last beta release (version 0.9.3).
 *	Late  95  - strn code ported to trn 4.0, universal selector started
 *      May   96  - strn 1.0 release
 *
 */

#include "patchlevel.h"
#include "INTERN.h"
#include "common.h"
#include "EXTERN.h"
#include "list.h"
#include "hash.h"
#include "ngdata.h"
#include "nntpclient.h"
#include "datasrc.h"
#include "nntp.h"
#include "INTERN.h"
#include "utf.h"
#ifdef USE_UTF_HACK
#undef PATCHLEVEL
#include "patchlevel2.h"
#endif
#include "trn.h"
#include "EXTERN.h"
#include "term.h"
#include "final.h"
#include "search.h"
#include "addng.h"
#include "ngstuff.h"
#include "rcstuff.h"
#include "env.h"
#include "util.h"
#include "util2.h"
#include "only.h"
#include "ngsrch.h"
#include "help.h"
#include "last.h"
#include "init.h"
#include "intrp.h"
#include "rcln.h"
#include "sw.h"
#include "opt.h"
#include "cache.h"
#include "ng.h"
#include "kfile.h"
#include "rt-select.h"
#ifdef SCAN
#include "scan.h"
#endif /* SCAN */
#include "univ.h"

void
trn_init()
{
    ;
}

static bool restore_old_newsrc = FALSE;
static bool go_forward = TRUE;

int
main(argc,argv)
int argc;
char* argv[];
{
    bool foundany;
    char* s;

    /* Figure out our executable's name. */
#ifdef MSDOS
    strlwr(argv[0]);
    if ((s = rindex(argv[0],'\\')) != NULL)
	*s = '/';
#endif
    if ((s = rindex(argv[0],'/')) == NULL)
	s = argv[0];
    else
	s++;
#if !THREAD_INIT
    /* Default to threaded operation if our name starts with a 't' or 's'. */
    if (*s == 't' || *s == 's')
	use_threads = TRUE;
    else
	UseNewsSelector = -1;
#endif
    if (*s == 's')
	is_strn = TRUE;
#ifdef USE_TK /*XXX remove this?*/
    if (s[1] == 'k')
	UseTk = 1;
    else
	UseTk = 0;
#endif
    foundany = initialize(argc,argv);

    if (UseNewsrcSelector) {
	multirc_selector();
	finalize(0);
    }

#ifdef FINDNEWNG
    if (find_new_groups()) {		/* did we add any new groups? */
	foundany = TRUE;
	starthere = NULL;		/* start ng scan from the top */
    }
#endif

    if (maxngtodo)
	starthere = NULL;
    else if (!foundany) {		/* nothing to do? */
#ifdef VERBOSE
	if (verbose) {
	    fputs("\
No unread news in subscribed-to newsgroups.  To subscribe to a new\n\
newsgroup use the g<newsgroup> command.\n\
",stdout) FLUSH;
	    termdown(2);
	}
#endif
	starthere = last_ng;
    }

    do_multirc();

    finalize(0);
    /* NOT REACHED */
    return 0;
}

void
do_multirc()
{
    bool special = FALSE;	/* allow newsgroup with no unread news? */
    char mode_save = mode;
    char gmode_save = gmode;

    if (UseUnivSelector) {
	char ch;
	univ_startup();		/* load startup file */
	ch = universal_selector();
	if (ch != 'Q') {
	    /* section copied from bug_out below */
	    /* now write the newsrc(s) back out */
	    if (!write_newsrcs(multirc))
		restore_old_newsrc = TRUE; /*$$ ask to retry! */
	    if (restore_old_newsrc)
		get_old_newsrcs(multirc);
	    finalize(0);
	}
    }

    if (UseNewsgroupSelector) {
      ng_start_sel:
	switch (newsgroup_selector()) {
	case Ctl('n'):
	    use_next_multirc(multirc);
	    end_only();
	    goto ng_start_sel;
	case Ctl('p'):
	    use_prev_multirc(multirc);
	    end_only();
	    goto ng_start_sel;
	case 'q':
	    goto bug_out;
	}
	starthere = ngptr;
	UseNewsgroupSelector = FALSE;
    }

    /* loop through all unread news */
  restart:
    current_ng = first_ng;
    for (;;) {
	bool retry = FALSE;
	if (findlast > 0) {
	    findlast = -1;
	    starthere = NULL;
	    if (lastngname) {
		if ((ngptr = find_ng(lastngname)) == NULL)
		    ngptr = first_ng;
		else {
		    set_ngname(lastngname);
		    set_toread(ngptr, ST_LAX);
		    if (ngptr->toread <= TR_NONE)
			ngptr = first_ng;
		}
	    }
	} else if (starthere) {
	    ngptr = starthere;
	    starthere = NULL;
	}
	else
	    ngptr = first_ng;
	for (;;) {			/* for each newsgroup */
	    if (ngptr == NULL) {	/* after the last newsgroup? */
		set_mode('r','f');
		if (maxngtodo) {
		    if (retry) {
#ifdef VERBOSE
			IF(verbose)
			    printf("\nRestriction %s%s still in effect.\n",
				   ngtodo[0], maxngtodo > 1 ? ", etc." : nullstr) FLUSH;
		 	ELSE
#endif
#ifdef TERSE
			    fputs("\n(\"Only\" mode.)\n",stdout) FLUSH;
#endif
			termdown(2);
		    } else {
#ifdef VERBOSE
			IF(verbose)
			    fputs("\nNo articles under restriction.", stdout) FLUSH;
			ELSE
#endif
#ifdef TERSE
			    fputs("\nNo \"only\" articles.",stdout) FLUSH;
#endif
			termdown(2);
			end_only();	/* release the restriction */
			printf("\n%s\n", msg) FLUSH;
			termdown(2);
			retry = TRUE;
		    }
		}
	    }
	    else {
		bool shoe_fits;	/* newsgroup matches restriction? */

		set_mode('r','n');
		if (ngptr->toread >= TR_NONE) {	/* recalc toread? */
		    set_ngname(ngptr->rcline);
		    shoe_fits = inlist(ngname);
		    if (shoe_fits)
			set_toread(ngptr, ST_LAX);
		    if (paranoid) {
			recent_ng = current_ng;
			current_ng = ngptr;
			cleanup_newsrc(ngptr->rc); /* this may move newsgroups around */
			set_ng(current_ng);
		    }
		} else
		    shoe_fits = TRUE;
		if (ngptr->toread < (special? TR_NONE : ng_min_toread)
		 || !shoe_fits) {		/* unwanted newsgroup? */
		    if (go_forward)
			ngptr = ngptr->next;
		    else {
			ngptr = ngptr->prev;
			if (ngptr == NULL) {
			    ngptr = first_ng->next;
			    go_forward = 1;
			}
		    }
		    continue;
		}
	    }
	    special = FALSE;	/* go back to normal mode */
	    if (ngptr != current_ng) {
		recent_ng = current_ng;	/* remember previous newsgroup */
		current_ng = ngptr;	/* remember current newsgroup */
	    }
    reask_newsgroup:
	    unflush_output();	/* disable any ^O in effect */
	    if (ngptr == NULL) {
		dfltcmd = (retry ? "npq" : "qnp");
#ifdef VERBOSE
		IF(verbose)
		    printf("\n****** End of newsgroups -- what next? [%s] ",
			   dfltcmd);
		ELSE
#endif
#ifdef TERSE
		    printf("\n**** End -- next? [%s] ", dfltcmd);
#endif
		termdown(1);
	    } else {
		ThreadedGroup = (use_threads && !(ngptr->flags&NF_UNTHREADED));
		dfltcmd = (UseNewsSelector >= 0
		  && ngptr->toread >= (ART_UNREAD)UseNewsSelector? "+ynq":"ynq");
#ifdef VERBOSE
		IF(verbose)
		    printf("\n%s %3ld unread article%s in %s -- read now? [%s] ",
			   ThreadedGroup? "======" : "******",
			   (long)ngptr->toread, PLURAL(ngptr->toread),
			   ngname, dfltcmd);
		ELSE
#endif
#ifdef TERSE
		    printf("\n%s %3ld in %s -- read? [%s] ",
			   ThreadedGroup? "====" : "****",
			   (long)ngptr->toread,ngname,dfltcmd);
#endif
		termdown(1);
	    }
	    fflush(stdout);
    reinp_newsgroup:
	    if (special || (ngptr && ngptr->toread > 0))
		retry = TRUE;
	    switch (input_newsgroup()) {
	    case ING_ASK:
		goto reask_newsgroup;
	    case ING_INPUT:
	    case ING_ERASE:
		goto reinp_newsgroup;
	    case ING_ERROR:
		printf("\n%s",hforhelp) FLUSH;
		termdown(2);
		settle_down();
		goto reask_newsgroup;
	    case ING_QUIT:
		goto bug_out;
	    case ING_BREAK:
		goto loop_break;
	    case ING_RESTART:
		goto restart;
#ifdef SUPPORT_NNTP
	    case ING_NOSERVER:
		if (multirc)
		    goto restart;
		goto bug_out;
#endif
	    case ING_SPECIAL:
		special = TRUE;
		break;
	    case ING_NORM:
		break;
	    case ING_DISPLAY:
		newline();
		break;
	    case ING_MESSAGE:
		printf("\n%s\n", msg) FLUSH;
		termdown(2);
		break;
	    }
	}
    loop_break:;
#ifdef SUPPORT_NNTP
	check_active_refetch(FALSE);
#endif
    }

bug_out:
    /* now write the newsrc(s) back out */
    if (!write_newsrcs(multirc))
	restore_old_newsrc = TRUE; /*$$ ask to retry! */
    if (restore_old_newsrc)
	get_old_newsrcs(multirc);

    set_mode(gmode_save,mode_save);
}

int
input_newsgroup()
{
    register char* s;

    ing_state = INGS_DIRTY;
    eat_typeahead();
    getcmd(buf);
    if (errno || *buf == '\f') {
	newline();		/* if return from stop signal */
	return ING_ASK;
    }
    buf[2] = *buf;
    setdef(buf,dfltcmd);
#ifdef VERIFY
    printcmd();
#endif
    if (ngptr != NULL)
	*buf = buf[2];

  do_command:
    go_forward = TRUE;		/* default to forward motion */
    switch (*buf) {
      case 'P':			/* goto previous newsgroup */
      case 'p':			/* find previous unread newsgroup */
	if (!ngptr)
	    ngptr = last_ng;
	else if (ngptr != first_ng)
	    ngptr = ngptr->prev;
	go_forward = FALSE;	/* go backward in the newsrc */
	ing_state = INGS_CLEAN;
	if (*buf == 'P')
	    return ING_SPECIAL;
	break;
      case '-':
	ngptr = recent_ng;		/* recall previous newsgroup */
	if (ngptr) {
	    if (!get_ng(ngptr->rcline,0))
		set_ng(current_ng);
	    addnewbydefault = 0;
	}
	return ING_SPECIAL;
      case 'x':
	newline();
	in_char("Confirm: exit and abandon newsrc changes?", 'A', "yn");
	newline();
	if (*buf != 'y')
	    break;
	printf("\nThe abandoned changes are in %s.new.\n",
	       multirc_name(multirc)) FLUSH;
	termdown(2);
	restore_old_newsrc = TRUE;
	return ING_QUIT;
      case 'q': case 'Q':	/* quit? */
	newline();
	return ING_QUIT;
      case '^':
	if (gmode != 's')
	    newline();
	ngptr = first_ng;
	ing_state = INGS_CLEAN;
	break;
      case 'N':			/* goto next newsgroup */
      case 'n':			/* find next unread newsgroup */
	if (ngptr == NULL) {
	    newline();
	    return ING_BREAK;
	}
	ngptr = ngptr->next;
	ing_state = INGS_CLEAN;
	if (*buf == 'N')
	    return ING_SPECIAL;
	break;
      case '1':			/* goto 1st newsgroup */
	ngptr = first_ng;
	ing_state = INGS_CLEAN;
	return ING_SPECIAL;
      case '$':
	ngptr = NULL;		/* go past last newsgroup */
	ing_state = INGS_CLEAN;
	break;
      case 'L':
	list_newsgroups();
	return ING_ASK;
      case '/': case '?':	/* scan for newsgroup pattern */
#ifdef NGSEARCH
	switch (ng_search(buf,TRUE)) {
	  case NGS_ERROR:
	    set_ng(current_ng);
	    return ING_ASK;
	  case NGS_ABORT:
	    set_ng(current_ng);
	    return ING_INPUT;
	  case NGS_INTR:
#ifdef VERBOSE
	    IF(verbose)
		fputs("\n(Interrupted)\n",stdout) FLUSH;
	    ELSE
#endif
#ifdef TERSE
		fputs("\n(Intr)\n",stdout) FLUSH;
#endif
	    termdown(2);
	    set_ng(current_ng);
	    return ING_ASK;
	  case NGS_FOUND:
	    return ING_SPECIAL;
	  case NGS_NOTFOUND:
#ifdef VERBOSE
	    IF(verbose)
		fputs("\n\nNot found -- use a or g to add newsgroups\n",
		      stdout) FLUSH;
	    ELSE
#endif
#ifdef TERSE
		fputs("\n\nNot found\n",stdout) FLUSH;
#endif
	    termdown(3);
	    return ING_ASK;
	  case NGS_DONE:
	    return ING_ASK;
	}
#else
	notincl("/");
#endif
	break;
      case 'm':
#ifndef RELOCATE
	notincl("m");
	break;
#endif		    
      case 'g':	/* goto named newsgroup */
	if (!finish_command(FALSE))
	    return ING_INPUT;
	for (s = buf+1; *s == ' '; s++) ; /* skip leading spaces */
#ifdef RELOCATE
	if (!*s && *buf == 'm' && ngname && ngptr)
	    strcpy(s,ngname);
#endif
	{
	    char* _s;
	    for (_s=s; isdigit(*_s); _s++) ;
	    if (*_s)
		/* found non-digit before hitting end */
		set_ngname(s);
	    else {
		int rcnum = atoi(s);
		for (ngptr = first_ng; ngptr; ngptr = ngptr->next)
		    if (ngptr->num == rcnum)
			break;
		if (!ngptr) {
		    ngptr = current_ng;
		    printf("\nOnly %d groups. Try again.\n", newsgroup_cnt) FLUSH;
		    termdown(2);
		    return ING_ASK;
		}
		set_ngname(ngptr->rcline);
	    }
	}
	/* try to find newsgroup */
#ifdef RELOCATE
	if (!get_ng(ngname,(*buf=='m'?GNG_RELOC:0) | GNG_FUZZY))
#else
	if (!get_ng(ngname,GNG_FUZZY))
#endif
	    ngptr = current_ng;	/* if not found, go nowhere */
	addnewbydefault = 0;
	return ING_SPECIAL;
#ifdef DEBUG
      case 'D':
	return ING_ASK;
#endif
      case '!':			/* shell escape */
	if (escapade())		/* do command */
	    return ING_INPUT;
	return ING_ASK;
      case Ctl('k'):		/* edit global KILL file */
	edit_kfile();
	return ING_ASK;
      case Ctl('n'):		/* next newsrc list */
	end_only();
	use_next_multirc(multirc);
	goto display_multirc;
      case Ctl('p'):		/* prev newsrc list */
	end_only();
	use_prev_multirc(multirc);
      display_multirc:
      {
	NEWSRC* rp;
	int len;
	for (rp = multirc->first, len = 0; rp && len < 66; rp = rp->next) {
	    if (rp->flags & RF_ACTIVE) {
		sprintf(buf+len, ", %s", rp->datasrc->name);
		len += strlen(buf+len);
	    }
	}
	if (rp)
	    strcpy(buf+len, ", ...");
	printf("\n\nUsing newsrc group #%d: %s.\n",multirc->num,buf+2);
	termdown(3);
	return ING_RESTART;
      }
      case 'c':			/* catch up */
#ifdef CATCHUP
	if (ngptr) {
	    ask_catchup();
	    if (ngptr->toread == TR_NONE)
		ngptr = ngptr->next;
	}
#else
	notincl("c");
#endif
	break;
      case 't':
	if (!use_threads)
	    printf("\n\nNot running in thread mode.\n");
	else if (ngptr && ngptr->toread >= TR_NONE) {
	    bool read_unthreaded = !(ngptr->flags&NF_UNTHREADED);
	    ngptr->flags ^= NF_UNTHREADED;
	    printf("\n\n%s will be read %sthreaded.\n",
		   ngptr->rcline, read_unthreaded? "un" : nullstr) FLUSH;
	    set_toread(ngptr, ST_LAX);
	}
	termdown(3);
	return ING_SPECIAL;
      case 'u':			/* unsubscribe */
	if (ngptr && ngptr->toread >= TR_NONE) {/* unsubscribable? */
	    newline();
	    printf(unsubto,ngptr->rcline) FLUSH;
	    termdown(1);
	    ngptr->subscribechar = NEGCHAR;	/* unsubscribe it */
	    ngptr->toread = TR_UNSUB;		/* and make line invisible */
	    ngptr->rc->flags |= RF_RCCHANGED;
	    ngptr = ngptr->next;		/* do an automatic 'n' */
	    newsgroup_toread--;
	}
	break;
      case 'h':
	univ_help(UHELP_NG);
	return ING_ASK;
      case 'H':			/* help */
	help_ng();
	return ING_ASK;
      case 'A':
	if (!ngptr)
	    break;
reask_abandon:
#ifdef VERBOSE
	IF(verbose)
	    in_char("\nAbandon changes to current newsgroup?", 'B', "yn");
	ELSE
#endif
#ifdef TERSE
	    in_char("\nAbandon?", 'B', "ynh");
#endif
#ifdef VERIFY
	printcmd();
#endif
	newline();
	if (*buf == 'h') {
#ifdef VERBOSE
	    printf("Type y or SP to abandon the changes to this group since you started trn.\n");
	    printf("Type n to leave the group as it is.\n");
#else
	    printf("y or SP to abandon changes to this group.\n");
	    printf("n to forget it.\n");
#endif
	    termdown(2);
	    goto reask_abandon;
	} else if (*buf != 'y' && *buf != 'n' && *buf != 'q') {
	    fputs(hforhelp,stdout) FLUSH;
	    termdown(1);
	    settle_down();
	    goto reask_abandon;
	} else if (*buf == 'y')
	    abandon_ng(ngptr);
	return ING_SPECIAL;
      case 'a':
#ifndef FINDNEWNG
	notincl("a");
	return ING_ASK;
#else
	/* FALL THROUGH */
#endif
      case 'o':
      case 'O': {
#ifdef FINDNEWNG
	bool doscan = (*buf == 'a');
#endif
	if (!finish_command(TRUE)) /* get rest of command */
	    return ING_INPUT;
	*msg = '\0';
	end_only();
	if (buf[1]) {
	    bool minusd = instr(buf+1,"-d", TRUE) != NULL;
	    sw_list(buf+1);
	    if (minusd)
		cwd_check();
#ifdef FINDNEWNG
	    if (doscan && maxngtodo)
		scanactive(TRUE);
#endif
	    ng_min_toread = *buf == empty_only_char && maxngtodo
			  ? TR_NONE : TR_ONE;
	}
	ngptr = first_ng;	/* simulate ^ */
	if (*msg && !maxngtodo)
	    return ING_MESSAGE;
	return ING_DISPLAY;
      }
      case '&':
	if (switcheroo())	/* get rest of command */
	    return ING_INPUT;	/* if rubbed out, try something else */
	return ING_ASK;
      case 'l': {		/* list other newsgroups */
	if (!finish_command(TRUE)) /* get rest of command */
	    return ING_INPUT;	/* if rubbed out, try something else */
	for (s = buf+1; *s == ' '; s++) ; /* skip leading spaces */
	push_only();
	if (*s)
	    sw_list(s);
	page_start();
	scanactive(FALSE);
	pop_only();
	return ING_ASK;
      }
      case '`':
      case '\\':
	if (gmode == 's')
	    return ING_ERASE;
      ng_start_sel:
	UseNewsgroupSelector = TRUE;
	switch (newsgroup_selector()) {
	  case Ctl('n'):
	    end_only();
	    use_next_multirc(multirc);
	    goto ng_start_sel;
	  case Ctl('p'):
	    end_only();
	    use_prev_multirc(multirc);
	    goto ng_start_sel;
	  case 'q':
	     return ING_QUIT;
	}
	UseNewsgroupSelector = FALSE;
	return ING_ASK;
#ifdef SCAN_ART
      case ';':
#endif
      case 'U': case '+':
      case '.': case '=':
      case 'y': case 'Y': case '\t': /* do normal thing */
      case ' ': case '\r': case '\n':
	if (!ngptr) {
	    fputs("\nNot on a newsgroup.",stdout) FLUSH;
	    termdown(1);
	    return ING_ASK;
	}
	/* CAA: *once*, the char* s was set to an illegal value
	 *      (it seemed to miss all the if statements below)
	 *      Just to be safe, make sure it is legal.
	 */
	s = nullstr;
	if (*buf == '.') {		/* start command? */
	    if (!finish_command(FALSE)) /* get rest of command */
		return ING_INPUT;
	    s = savestr(buf+1);		/* do_newsgroup will free it */
	}
	else if (*buf == '+' || *buf == 'U' || *buf == '='
#ifdef SCAN_ART
		|| *buf == ';'
#endif
	) {
	    *buf = lastchar; /* restore 0200 if from a macro */
	    save_typeahead(buf+1, sizeof buf - 1);
	    s = savestr(buf);
	}
	else if (*buf == ' ' || *buf == '\r' || *buf == '\n')
	    s = nullstr;
	else
	    s = NULL;

      redo_newsgroup:
	switch (do_newsgroup(s)) {
	  case NG_NORM:
	  case NG_NEXT:
	  case NG_ERROR:
	    ngptr = ngptr->next;
	    break;
	  case NG_ASK:
	    return ING_ASK;
	  case NG_SELPRIOR:
	    *buf = 'p';
	    goto do_command;
	  case NG_SELNEXT:
	    *buf = 'n';
	    goto do_command;
	  case NG_MINUS:
	    ngptr = recent_ng;	/* recall previous newsgroup */
	    return ING_SPECIAL;
#ifdef SUPPORT_NNTP
	  case NG_NOSERVER:
	    nntp_server_died(ngptr->rc->datasrc);
	    return ING_NOSERVER;
#endif
	  /* CAA extensions */
	  case NG_GO_ARTICLE:
	    ngptr = ng_go_ngptr;
	    s = savestr("y");		/* enter with minimal fuss */
	    goto redo_newsgroup;
	  /* later: possible go-to-newsgroup */
	}
	break;
      case ':':		/* execute command on selected groups */
	if (!ngsel_perform())
	    return ING_INPUT;
	page_line = 1;
	newline();
	set_ng(current_ng);
	return ING_ASK;
      case 'v':
	newline();
	trn_version();
	return ING_ASK;
      default:
	if (*buf == ERASECH || *buf == KILLCH)
	    return ING_ERASE;
	return ING_ERROR;
    }
    return ING_NORM;
}

#ifdef SUPPORT_NNTP
void
check_active_refetch(force)
bool_int force;
{
    DATASRC* dp;
    time_t now = time((time_t*)NULL);

    for (dp = datasrc_first(); dp && dp->name; dp = datasrc_next(dp)) {
	if (!ALLBITS(dp->flags, DF_OPEN | DF_ACTIVE))
	    continue;
	if (dp->act_sf.fp && dp->act_sf.refetch_secs
	 && (force || now - dp->act_sf.lastfetch > dp->act_sf.refetch_secs))
	    actfile_hash(dp);
    }
}
#endif

void
trn_version()
{
    page_start();
    sprintf(msg,"Trn version:%s.\nConfigured for ",patchlevel);
#ifdef SUPPORT_NNTP
# ifdef HAS_LOCAL_SPOOL
    strcat(msg,"both NNTP and local news access.\n");
# else
    strcat(msg,"NNTP (plus individual local access).\n");
# endif
#else
    strcat(msg,"local news access.\n");
#endif
    print_lines(msg, NOMARKING);

    if (multirc) {
	NEWSRC* rp;
	newline();
	sprintf(msg,"News source group #%d:\n\n", multirc->num);
	print_lines(msg, NOMARKING);
	for (rp = multirc->first; rp; rp = rp->next) {
	    if (!(rp->flags & RF_ACTIVE))
		continue;
	    sprintf(msg,"ID %s:\nNewsrc %s.\n",rp->datasrc->name,rp->name);
	    print_lines(msg, NOMARKING);
#ifdef SUPPORT_NNTP
	    if (rp->datasrc->flags & DF_REMOTE) {
		sprintf(msg,"News from server %s.\n",rp->datasrc->newsid);
		print_lines(msg, NOMARKING);
		if (rp->datasrc->act_sf.fp) {
		    if (rp->datasrc->flags & DF_TMPACTFILE)
			strcpy(msg,"Copy of remote active file");
		    else
			sprintf(msg,"Local active file: %s",
				rp->datasrc->extra_name);
		}
		else
		    strcpy(msg,"Dynamic active file");
		if (rp->datasrc->act_sf.refetch_secs) {
		    char* cp = secs2text(rp->datasrc->act_sf.refetch_secs);
		    if (*cp != 'n')
			sprintf(msg+strlen(msg),
				" (refetch%s %s)",*cp == 'm'? " if" : ":", cp);
		}
		strcat(msg,".\n");
	    }
	    else
#endif
		sprintf(msg,"News from %s.\nLocal active file %s.\n",
			rp->datasrc->spool_dir, rp->datasrc->newsid);
	    print_lines(msg, NOMARKING);
	    if (rp->datasrc->grpdesc) {
#ifdef SUPPORT_NNTP
		if (!rp->datasrc->desc_sf.fp && rp->datasrc->desc_sf.hp)
		    strcpy(msg,"Dynamic group desc. file");
		else if (rp->datasrc->flags & DF_TMPGRPDESC)
		    strcpy(msg,"Copy of remote group desc. file");
		else
#endif
		    sprintf(msg,"Group desc. file: %s",rp->datasrc->grpdesc);
#ifdef SUPPORT_NNTP
		if (rp->datasrc->desc_sf.refetch_secs) {
		    char* cp = secs2text(rp->datasrc->desc_sf.refetch_secs);
		    if (*cp != 'n')
			sprintf(msg+strlen(msg),
				" (refetch%s %s)",*cp == 'm'? " if" : ":", cp);
		}
#endif
		strcat(msg,".\n");
		print_lines(msg, NOMARKING);
	    }
	    if (rp->datasrc->flags & DF_TRY_OVERVIEW) {
		sprintf(msg,"Overview files from %s.\n",
			rp->datasrc->over_dir? rp->datasrc->over_dir
					     : "the server");
		print_lines(msg, NOMARKING);
	    }
	    if (rp->datasrc->flags & DF_TRY_THREAD) {
		sprintf(msg,"Thread files from %s.\n",
			rp->datasrc->thread_dir? rp->datasrc->thread_dir
					       : "the server");
		print_lines(msg, NOMARKING);
	    }
	    print_lines("\n", NOMARKING);
	}
    }

    print_lines("\
You can request help from:  trn-users@lists.sourceforge.net\n\
Send bug reports, suggestions, etc. to:  trn-workers@lists.sourceforge.net\n",
		NOMARKING);
}

void
set_ngname(what)
char* what;
{
    if (ngname != what) {
	ngname_len = strlen(what);
	growstr(&ngname,&ngnlen,ngname_len+1);
	strcpy(ngname,what);
    }
    growstr(&ngdir,&ngdlen,ngname_len+1);
    strcpy(ngdir,getngdir(ngname));
}

static char* myngdir;
static int ngdirlen = 0;

char*
getngdir(ngnam)
char* ngnam;
{
    register char* s;

    growstr(&myngdir,&ngdirlen,strlen(ngnam)+1);
    strcpy(myngdir,ngnam);
    for (s = myngdir; *s; s++)
	if (*s == '.')
	    *s = '/';
    return myngdir;
}
