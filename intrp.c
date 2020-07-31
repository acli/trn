/* intrp.c
 */
/* This software is copyrighted as detailed in the LICENSE file. */


#include "EXTERN.h"
#include "common.h"
#include "list.h"
#include "env.h"
#include "util.h"
#include "util2.h"
#include "search.h"
#include "hash.h"
#include "cache.h"
#include "bits.h"
#include "head.h"
#include "trn.h"
#include "ngdata.h"
#include "nntpclient.h"
#include "datasrc.h"
#include "nntp.h"
#include "artsrch.h"
#include "ng.h"
#include "respond.h"
#include "rcstuff.h"
#include "artio.h"
#include "init.h"
#include "term.h"
#include "final.h"
#include "rthread.h"
#include "rt-select.h"
#include "rt-util.h"
#include "INTERN.h"
#include "intrp.h"
#include "intrp.ih"
#include <netdb.h>

static char* regexp_specials = "^$.*[\\/?%";

char orgname[] = ORGNAME;

#ifdef HAS_UNAME
#include <sys/utsname.h>
struct utsname utsn;
#endif

COMPEX cond_compex;

void
intrp_init(tcbuf, tcbuf_len)
char* tcbuf;
int tcbuf_len;
{
#if HOSTBITS != 0
    int i;
#endif

    init_compex(&cond_compex);
    
    /* get environmental stuff */

#ifdef NEWS_ADMIN
    {
#ifdef HAS_GETPWENT
	struct passwd* pwd = getpwnam(NEWS_ADMIN);

	if (pwd != NULL)
	    newsuid = pwd->pw_uid;
#else
#ifdef TILDENAME
	char tildenews[2+sizeof NEWS_ADMIN];
	strcpy(tildenews, "~");
	strcat(tildenews, NEWS_ADMIN);
	(void) filexp(tildenews);
#else
	... "Define either HAS_GETPWENT or TILDENAME to get NEWS_ADMIN"
#endif  /* TILDENAME */
#endif	/* HAS_GETPWENT */
    }

    /* if this is the news admin then load his UID into newsuid */

    if (strEQ(loginName,NEWS_ADMIN))
	newsuid = getuid();
#endif

    if (checkflag)			/* that getwd below takes ~1/3 sec. */
	return;				/* and we do not need it for -c */
    trn_getwd(tcbuf, tcbuf_len);	/* find working directory name */
    origdir = savestr(tcbuf);		/* and remember it */

    /* name of header file (%h) */

    headname = savestr(filexp(HEADNAME));

    /* the hostname to use in local-article comparisons */
#if HOSTBITS != 0
    i = (HOSTBITS < 2? 2 : HOSTBITS);
    hostname = phostname+strlen(phostname)-1;
    while (i && hostname != phostname) {
	if (*--hostname == '.')
	    i--;
    }
    if (*hostname == '.')
	hostname++;
#else
    hostname = phostname;
#endif
}

/* skip interpolations */

static char*
skipinterp(pattern,stoppers)
register char* pattern;
char* stoppers;
{
#ifdef DEBUG
    if (debug & DEB_INTRP)
	printf("skipinterp %s (till %s)\n",pattern,stoppers?stoppers:"");
#endif

    while (*pattern && (!stoppers || !index(stoppers,*pattern))) {
	if (*pattern == '%' && pattern[1]) {
	switch_again:
	    switch (*++pattern) {
	    case '^':
	    case '_':
	    case '\\':
	    case '\'':
	    case '>':
	    case ')':
		goto switch_again;
	    case ':':
		pattern++;
		while (*pattern
		 && (*pattern=='.' || *pattern=='-' || isdigit(*pattern))) {
		    pattern++;
		}
		pattern--;
		goto switch_again;
	    case '{':
		for (pattern++; *pattern && *pattern != '}'; pattern++)
		    if (*pattern == '\\')
			pattern++;
		break;
	    case '[':
		for (pattern++; *pattern && *pattern != ']'; pattern++)
		    if (*pattern == '\\')
			pattern++;
		break;
	    case '(': {
		pattern = skipinterp(pattern+1,"!=");
		if (!*pattern)
		    goto getout;
		for (pattern++; *pattern && *pattern != '?'; pattern++)
		    if (*pattern == '\\')
			pattern++;
		if (!*pattern)
		    goto getout;
		pattern = skipinterp(pattern+1,":)");
		if (*pattern == ':')
		    pattern = skipinterp(pattern+1,")");
		break;
	    }
#ifdef BACKTICK
	    case '`': {
		pattern = skipinterp(pattern+1,"`");
		break;
	    }
#endif
#ifdef PROMPTTTY
	    case '"':
		pattern = skipinterp(pattern+1,"\"");
		break;
#endif
	    default:
		break;
	    }
	    pattern++;
	}
	else {
	    if (*pattern == '^'
	     && ((Uchar)pattern[1]>='?' || pattern[1]=='(' || pattern[1]==')'))
		pattern += 2;
	    else if (*pattern == '\\' && pattern[1])
		pattern += 2;
	    else
		pattern++;
	}
    }
getout:
    return pattern;			/* where we left off */
}

/* interpret interpolations */

char*
dointerp(dest,destsize,pattern,stoppers,cmd)
register char* dest;
register int destsize;
register char* pattern;
char* stoppers;
char* cmd;
{
    char* subj_buf = NULL;
    char* ngs_buf = NULL;
    char* refs_buf = NULL;
    char* artid_buf = NULL;
    char* reply_buf = NULL;
    char* from_buf = NULL;
    char* path_buf = NULL;
    char* follow_buf = NULL;
    char* dist_buf = NULL;
    char* line_buf = NULL;
    char* line_split = NULL;
    char* orig_dest = dest;
    register char* s;
    register char* h;
    register int i;
    char scrbuf[8192];
    char spfbuf[512];
    static char* input_str = NULL;
    static int input_siz = 0;
    bool upper = FALSE;
    bool lastcomp = FALSE;
    bool re_quote = FALSE;
    int tick_quote = 0;
    bool address_parse = FALSE;
    bool comment_parse = FALSE;
    bool proc_sprintf = FALSE;
    int metabit = 0;

#ifdef DEBUG
    if (debug & DEB_INTRP)
	printf(">dointerp: %s (till %s)\n",pattern,stoppers?stoppers:"");
#endif

    while (*pattern && (!stoppers || !index(stoppers,*pattern))) {
	if (*pattern == '%' && pattern[1]) {
	    upper = FALSE;
	    lastcomp = FALSE;
	    re_quote = FALSE;
	    tick_quote = 0;
	    address_parse = FALSE;
	    comment_parse = FALSE;
	    proc_sprintf = FALSE;
	    for (s=NULL; !s; ) {
		switch (*++pattern) {
		case '^':
		    upper = TRUE;
		    break;
		case '_':
		    lastcomp = TRUE;
		    break;
		case '\\':
		    re_quote = TRUE;
		    break;
		case '\'':
		    tick_quote++;
		    break;
		case '>':
		    address_parse = TRUE;
		    break;
		case ')':
		    comment_parse = TRUE;
		    break;
		case ':':
		    proc_sprintf = TRUE;
		    h = spfbuf;
		    *h++ = '%';
		    pattern++;	/* Skip over ':' */
		    while (*pattern
		     && (*pattern=='.' || *pattern=='-' || isdigit(*pattern))) {
			*h++ = *pattern++;
		    }
		    *h++ = 's';
		    *h++ = '\0';
		    pattern--;
		    break;
		case '/':
#ifdef ARTSEARCH
		    s = scrbuf;
		    if (!cmd || !index("/?g",*cmd))
			*s++ = '/';
		    strcpy(s,lastpat);
		    s += strlen(s);
		    if (!cmd || *cmd != 'g') {
			if (cmd && index("/?",*cmd))
			    *s++ = *cmd;
			else
			    *s++ = '/';
			if (art_doread)
			    *s++ = 'r';
			if (art_howmuch != ARTSCOPE_SUBJECT) {
			    *s++ = scopestr[art_howmuch];
			    if (art_howmuch == ARTSCOPE_ONEHDR) {
				safecpy(s,htype[art_srchhdr].name,
					(sizeof scrbuf) - (s-scrbuf));
				if (!(s = index(s,':')))
				    s = scrbuf+(sizeof scrbuf)-1;
				else
				    s++;
			    }
			}
		    }
		    *s = '\0';
		    s = scrbuf;
#else
		    s = nullstr;
#endif
		    break;
		case '{':
		    pattern = cpytill(scrbuf,pattern+1,'}');
		    if ((s = index(scrbuf,'-')) != NULL)
			*s++ = '\0';
		    else
			s = nullstr;
		    s = getval(scrbuf,s);
		    break;
		case '<':
		    pattern = cpytill(scrbuf,pattern+1,'>');
		    if ((s = index(scrbuf,'-')) != NULL)
			*s++ = '\0';
		    else
			s = nullstr;
		    interp(scrbuf, 8192, getval(scrbuf,s));
		    s = scrbuf;
		    break;
		case '[':
		    if (in_ng) {
			pattern = cpytill(scrbuf,pattern+1,']');
			if (*scrbuf
			 && (i = get_header_num(scrbuf)) != SOME_LINE) {
			    safefree(line_buf);
			    s = line_buf = fetchlines(art,i);
			}
			else
			    s = nullstr;
		    }
		    else
			s = nullstr;
		    break;
		case '(': {
		    COMPEX *oldbra_compex = bra_compex;
		    char rch;
		    bool matched;
		    
		    pattern = dointerp(dest,destsize,pattern+1,"!=",cmd);
		    rch = *pattern;
		    if (rch == '!')
			pattern++;
		    if (*pattern != '=')
			goto getout;
		    pattern = cpytill(scrbuf,pattern+1,'?');
		    if (!*pattern)
			goto getout;
		    s = scrbuf;
		    h = spfbuf;
		    proc_sprintf = FALSE;
		    do {
			switch (*s) {
			case '^':
			    *h++ = '\\';
			    break;
			case '\\':
			    *h++ = '\\';
			    *h++ = '\\';
			    break;
			case '%':
			    proc_sprintf = TRUE;
			    break;
			}
			*h++ = *s;
		    } while (*s++);
		    if (proc_sprintf) {
			dointerp(scrbuf,sizeof scrbuf,spfbuf,(char*)NULL,cmd);
			proc_sprintf = FALSE;
		    }
		    if ((s = compile(&cond_compex,scrbuf,TRUE,TRUE)) != NULL) {
			printf("%s: %s\n",scrbuf,s) FLUSH;
			pattern += strlen(pattern);
			free_compex(&cond_compex);
			goto getout;
		    }
		    matched = (execute(&cond_compex,dest) != NULL);
		    if (getbracket(&cond_compex, 0)) /* were there brackets? */
			bra_compex = &cond_compex;
		    if (matched==(rch == '=')) {
			pattern = dointerp(dest,destsize,pattern+1,":)",cmd);
			if (*pattern == ':')
			    pattern = skipinterp(pattern+1,")");
		    }
		    else {
			pattern = skipinterp(pattern+1,":)");
			if (*pattern == ':')
			    pattern++;
			pattern = dointerp(dest,destsize,pattern,")",cmd);
		    }
		    s = dest;
		    bra_compex = oldbra_compex;
		    free_compex(&cond_compex);
		    break;
		}
#ifdef BACKTICK
		case '`': {
		    FILE* popen();
		    FILE* pipefp;

		    pattern = dointerp(scrbuf,(sizeof scrbuf),pattern+1,"`",cmd);
		    pipefp = popen(scrbuf,"r");
		    if (pipefp != NULL) {
			int len;

			len = fread(scrbuf,sizeof(char),(sizeof scrbuf)-1,
			    pipefp);
			scrbuf[len] = '\0';
			pclose(pipefp);
		    }
		    else {
			printf("\nCan't run %s\n",scrbuf);
			*scrbuf = '\0';
		    }
		    for (s=scrbuf; *s; s++) {
			if (*s == '\n') {
			    if (s[1])
				*s = ' ';
			    else
				*s = '\0';
			}
		    }
		    s = scrbuf;
		    break;
		}
#endif
#ifdef PROMPTTTY
		case '"':
		    pattern = dointerp(scrbuf,(sizeof scrbuf),pattern+1,"\"",cmd);
		    fputs(scrbuf,stdout) FLUSH;
		    resetty();
		    fgets(scrbuf, sizeof scrbuf, stdin);
		    noecho();
		    crmode();
		    i = strlen(scrbuf);
		    if (scrbuf[i-1] == '\n') {
			scrbuf[--i] = '\0';
		    }
		    growstr(&input_str, &input_siz, i+1);
		    safecpy(input_str, scrbuf, i+1);
		    s = input_str;
		    break;
#endif
		case '~':
		    s = homedir;
		    break;
		case '.':
		    s = dotdir;
		    break;
		case '+':
		    s = trndir;
		    break;
		case '$':
		    s = scrbuf;
		    sprintf(s,"%ld",our_pid);
		    break;
		case '#':
		    s = scrbuf;
		    if (upper) {
			static int counter = 0;
			sprintf(s,"%d",++counter);
		    }
		    else
			sprintf(s,"%d",perform_cnt);
		    break;
		case '?':
		    s = " ";
		    line_split = dest;
		    break;
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
		    s = getbracket(bra_compex,*pattern - '0');
		    break;
		case 'a':
		    if (in_ng) {
			s = scrbuf;
			sprintf(s,"%ld",(long)art);
		    }
		    else
			s = nullstr;
		    break;
		case 'A':
		    if (in_ng) {
#ifdef SUPPORT_NNTP
			if (datasrc->flags & DF_REMOTE) {
			    if (artopen(art,(ART_POS)0)) {
				nntp_finishbody(FB_SILENT);
				sprintf(s = scrbuf,"%s/%s",datasrc->spool_dir,
					nntp_artname(art, FALSE));
			    }
			    else
				s = nullstr;
			}
			else
#endif
#ifdef LINKART
			    s = linkartname;  /* for Eunice */
#else
			    sprintf(s = scrbuf,"%s/%s/%ld",datasrc->spool_dir,
				    ngdir,(long)art);
#endif
		    }
		    else
			s = nullstr;
		    break;
		case 'b':
		    s = savedest? savedest : nullstr;
		    break;
		case 'B':
		    s = scrbuf;
		    sprintf(s,"%ld",(long)savefrom);
		    break;
		case 'c':
		    s = ngdir? ngdir : nullstr;
		    break;
		case 'C':
		    s = ngname? ngname : nullstr;
		    break;
		case 'd':
		    if (ngdir) {
			s = scrbuf;
			sprintf(s,"%s/%s",datasrc->spool_dir,ngdir);
		    }
		    else
			s = nullstr;
		    break;
		case 'D':
		    if (in_ng)
			s = dist_buf = fetchlines(art,DIST_LINE);
		    else
			s = nullstr;
		    break;
		case 'e':
		    s = extractprog? extractprog : "-";
		    break;
		case 'E':
		    s = extractdest? extractdest : nullstr;
		    break;
		case 'f':			/* from line */
		    if (in_ng) {
			parseheader(art);
			if (htype[REPLY_LINE].minpos >= 0 && !comment_parse) {
						/* was there a reply line? */
			    if (!(s=reply_buf))
				s = reply_buf = fetchlines(art,REPLY_LINE);
			}
			else if (!(s = from_buf))
			    s = from_buf = fetchlines(art,FROM_LINE);
		    }
		    else
			s = nullstr;
		    break;
		case 'F':
		    if (in_ng) {
			parseheader(art);
			if (htype[FOLLOW_LINE].minpos >= 0)
					/* is there a Followup-To line? */
			    s = follow_buf = fetchlines(art,FOLLOW_LINE);
			else 
			    s = ngs_buf = fetchlines(art,NGS_LINE);
		    }
		    else
			s = nullstr;
		    break;
		case 'g':			/* general mode */
		    s = scrbuf;
		    *s = gmode;
		    s[1] = '\0';
		    break;
		case 'h':			/* header file name */
		    s = headname;
		    break;
		case 'H':			/* host name in postings */
		    s = phostname;
		    break;
		case 'i':
		    if (in_ng) {
			if (!(s=artid_buf))
			    s = artid_buf = fetchlines(art,MSGID_LINE);
			if (*s && *s != '<') {
			    sprintf(scrbuf,"<%s>",artid_buf);
			    s = scrbuf;
			}
		    }
		    else
			s = nullstr;
		    break;
		case 'I':			/* indent string for quoting */
		    s = scrbuf;
		    sprintf(scrbuf,"'%s'",indstr);
		    break;
		case 'j':
		    s = scrbuf;
		    sprintf(scrbuf,"%d",just_a_sec*10);
		    break;
		case 'l':			/* rn library */
#ifdef NEWS_ADMIN
		    s = newsadmin;
#else
		    s = "???";
#endif
		    break;
		case 'L':			/* login id */
		    s = loginName;
		    break;
		case 'm':		/* current mode */
		    s = scrbuf;
		    *s = mode;
		    s[1] = '\0';
		    break;
		case 'M':
		    sprintf(scrbuf,"%ld",(long)dmcount);
		    s = scrbuf;
		    break;
		case 'n':			/* newsgroups */
		    if (in_ng)
			s = ngs_buf = fetchlines(art,NGS_LINE);
		    else
			s = nullstr;
		    break;
		case 'N':			/* full name */
		    s = getval("NAME",realName);
		    break;
		case 'o':			/* organization */
#ifdef IGNOREORG
		    s = getval("NEWSORG",orgname); 
#else
		    s = getval("NEWSORG",NULL);
		    if (s == NULL) 
			s = getval("ORGANIZATION",orgname); 
#endif
		    s = filexp(s);
#ifdef ORGFILE
		    if (FILE_REF(s)) {
			FILE* ofp = fopen(s,"r");

			if (ofp) {
			    if (fgets(scrbuf,sizeof scrbuf,ofp) == NULL)
			    	*scrbuf = '\0';
			    fclose(ofp);
			    s = scrbuf+strlen(scrbuf)-1;
			    if (*scrbuf && *s == '\n')
				*s = '\0';
			    s = scrbuf;
			}
			else
			    s = nullstr;
		    }
#endif
		    break;
		case 'O':
		    s = origdir;
		    break;
		case 'p':
		    s = cwd;
		    break;
		case 'P':
		    s = datasrc? datasrc->spool_dir : nullstr;
		    break;
		case 'q':
		    s = input_str;
		    break;
		case 'r':
		    if (in_ng) {
			parseheader(art);
			safefree0(refs_buf);
			if (htype[REFS_LINE].minpos >= 0) {
			    refs_buf = fetchlines(art,REFS_LINE);
			    normalize_refs(refs_buf);
			    if ((s = rindex(refs_buf,'<')) != NULL)
				break;
			}
		    }
		    s = nullstr;
		    break;
		case 'R': {
		    int len, j;

		    if (!in_ng) {
			s = nullstr;
			break;
		    }
		    parseheader(art);
		    safefree0(refs_buf);
		    if (htype[REFS_LINE].minpos >= 0) {
			refs_buf = fetchlines(art,REFS_LINE);
			len = strlen(refs_buf)+1;
			normalize_refs(refs_buf);
			/* no more than 3 prior references PLUS the
			** root article allowed, including the one
			** concatenated below */
			if ((s = rindex(refs_buf,'<')) != NULL && s > refs_buf) {
			    *s = '\0';
			    h = rindex(refs_buf,'<');
			    *s = '<';
			    if (h && h > refs_buf) {
				s = index(refs_buf+1,'<');
				if (s < h)
				    safecpy(s,h,len);
			    }
			}
		    }
		    else
			len = 0;
		    if (!artid_buf)
			artid_buf = fetchlines(art,MSGID_LINE);
		    i = refs_buf? strlen(refs_buf) : 0;
		    j = strlen(artid_buf) + (i? 1 : 0)
		      + (artid_buf[0] == '<'? 0 : 2) + 1;
		    if (len < i + j)
			refs_buf = saferealloc(refs_buf, i + j);
		    if (i)
			refs_buf[i++] = ' ';
		    if (artid_buf[0] == '<')
			strcpy(refs_buf+i, artid_buf);
		    else if (artid_buf[0])
			sprintf(refs_buf+i, "<%s>", artid_buf);
		    else
			refs_buf[i] = '\0';
		    s = refs_buf;
		    break;
		}
		case 's':
		case 'S': {
		    char* str;
		    if (!in_ng || !art || !artp) {
			s = nullstr;
			break;
		    }
		    if ((str = subj_buf) == NULL)
			str = subj_buf = fetchsubj(art,TRUE);
		    subject_has_Re(str,&str);
		    if (*pattern == 's'
		     && (h = instr(str,"- (nf", TRUE)) != NULL)
			*h = '\0';
		    s = str;
		    break;
		}
		case 't':
		case 'T':
		    if (!in_ng) {
			s = nullstr;
			break;
		    }
		    parseheader(art);
		    if (htype[REPLY_LINE].minpos >= 0) {
					/* was there a reply line? */
			if (!(s=reply_buf))
			    s = reply_buf = fetchlines(art,REPLY_LINE);
		    }
		    else if (!(s = from_buf))
			s = from_buf = fetchlines(art,FROM_LINE);
		    else
			s = "noname";
		    if (*pattern == 'T') {
			if (htype[PATH_LINE].minpos >= 0) {
					/* should we substitute path? */
			    s = path_buf = fetchlines(art,PATH_LINE);
			}
			i = strlen(phostname);
			if (strnEQ(phostname,s,i) && s[i] == '!')
			    s += i + 1;
		    }
		    address_parse = TRUE;	/* just the good part */
		    break;
		case 'u':
		    if (in_ng) {
			sprintf(scrbuf,"%ld",(long)ngptr->toread);
			s = scrbuf;
		    }
		    else
			s = nullstr;
		    break;
		case 'U': {
		    int unseen;

		    if (!in_ng) {
			s = nullstr;
			break;
		    }
		    unseen = (art <= lastart) && !was_read(art);
		    if (selected_only) {
			int selected;

			selected = curr_artp && (curr_artp->flags & AF_SEL);
			sprintf(scrbuf,"%ld",
				(long)selected_count - (selected && unseen));
		    }
		    else
			sprintf(scrbuf,"%ld",(long)ngptr->toread - unseen);
		    s = scrbuf;
		    break;
		}
		case 'v': {
		    int selected, unseen;

		    if (in_ng) {
			selected = curr_artp && (curr_artp->flags & AF_SEL);
			unseen = (art <= lastart) && !was_read(art);
			sprintf(scrbuf,"%ld",(long)ngptr->toread-selected_count
						- (!selected && unseen));
			s = scrbuf;
		    }
		    else
			s = nullstr;
		    break;
		}
		case 'V':
		    s = patchlevel + 1;
		    break;
		case 'W':
		    s = datasrc? datasrc->thread_dir : nullstr;
		    break;
		case 'x':			/* news library */
		    s = lib;
		    break;
		case 'X':			/* rn library */
		    s = rnlib;
		    break;
#ifdef SCORE
		case 'y':	/* from line with *-shortening */
		    if (!in_ng) {
			s = nullstr;
			break;
		    }
#ifdef ASYNC_PARSE
/*		    parse_maybe(art); */
#endif
		    /* XXX Rewrite this! */
		    {	/* sick, but I don't want to hunt down a buf... */
			static char tmpbuf[1024];
			char* s2;
			char* s3;
			int i = 0;

			s2 = fetchlines(art,FROM_LINE);
			strcpy(tmpbuf,s2);
			free(s2);
			for (s2=tmpbuf;
			     (*s2 && (*s2 != '@') && (*s2 !=' '));s2++)
				; /* EMPTY */
			if (*s2 == '@') {	/* we have normal form... */
			    for (s3=s2+1;(*s3 && (*s3 != ' '));s3++)
				if (*s3 == '.')
				    i++;
			}
			if (i>1) { /* more than one dot */
			    s3 = s2;	/* will be incremented before use */
			    while (i>=2) {
				s3++;
				if (*s3 == '.')
				    i--;
			    }
			    s2++;
			    *s2 = '*';
			    s2++;
			    *s2 = '\0';
			    from_buf = (char*)safemalloc(
				(strlen(tmpbuf)+strlen(s3)+1)*sizeof(char));
			    strcpy(from_buf,tmpbuf);
			    strcat(from_buf,s3);
			} else {
			    from_buf = savestr(tmpbuf);
			}
			s = from_buf;
		    }
		    break;
#endif /* SCORE */
		case 'Y':
		    s = tmpdir;
		    break;
		case 'z':
		    if (!in_ng) {
			s = nullstr;
			break;
		    }
#ifdef LINKART
		    s = linkartname;	/* so Eunice people get right file */
#else
		    s = scrbuf;
		    sprintf(s,"%ld",(long)art);
#endif
		    if (stat(s,&filestat) < 0)
			filestat.st_size = 0L;
		    sprintf(scrbuf,"%5ld",(long)filestat.st_size);
		    s = scrbuf;
		    break;
		case 'Z':
		    sprintf(scrbuf,"%ld",(long)selected_count);
		    s = scrbuf;
		    break;
		case '\0':
		    s = nullstr;
		    break;
		default:
		    if (--destsize <= 0)
			abort_interp();
		    *dest++ = *pattern | metabit;
		    s = nullstr;
		    break;
		}
	    }
	    if (proc_sprintf) {
		sprintf(scrbuf,spfbuf,s);
		s = scrbuf;
	    }
	    if (*pattern)
		pattern++;
	    if (upper || lastcomp) {
		char* t;

		if (s != scrbuf) {
		    safecpy(scrbuf,s,sizeof scrbuf);
		    s = scrbuf;
		}
		if (upper || !(t = rindex(s,'/')))
		    t = s;
		while (*t && !isalpha(*t))
		    t++;
		if (islower(*t))
		    *t = toupper(*t);
	    }
	    /* Do we have room left? */
	    i = strlen(s);
	    if (destsize <= i)
		abort_interp();
	    destsize -= i;	/* adjust the size now. */

#ifdef DEBUG
	    if (debug & DEB_INTRP)
		printf("%% = %s\n",s);
#endif

	    /* A maze of twisty little conditions, all alike... */
	    if (address_parse || comment_parse) {
		if (s != scrbuf) {
		    safecpy(scrbuf,s,sizeof scrbuf);
		    s = scrbuf;
		}
		decode_header(s, s, strlen(s));
		if (address_parse) {
		    if ((h=index(s,'<')) != NULL) { /* grab the good part */
			s = h+1;
			if ((h=index(s,'>')) != NULL)
			    *h = '\0';
		    } else if ((h=index(s,'(')) != NULL) {
			while (h-- != s && *h == ' ')
			    ;
			h[1] = '\0';		/* or strip the comment */
		    }
		} else {
		    if (!(s = extract_name(s)))
			s = nullstr;
		}
	    }
	    if (metabit) {
		/* set meta bit while copying. */
		i = metabit;		/* maybe get into register */
		if (s == dest) {
		    while (*dest)
			*dest++ |= i;
		} else {
		    while (*s)
			*dest++ = *s++ | i;
		}
	    } else if (re_quote || tick_quote) {
		/* put a backslash before regexp specials while copying. */
		if (s == dest) {
		    /* copy out so we can copy in. */
		    safecpy(scrbuf, s, sizeof scrbuf);
		    s = scrbuf;
		    if (i > sizeof scrbuf)	/* we truncated, ack! */
			abort_interp();
		}
		while (*s) {
		    if ((re_quote && index(regexp_specials, *s))
		     || (tick_quote == 2 && *s == '"')) {
			if (--destsize <= 0)
			    abort_interp();
			*dest++ = '\\';
		    }
		    else if (re_quote && *s == ' ' && s[1] == ' ') {
			*dest++ = ' ';
			*dest++ = '*';
			while (*++s == ' ') ;
			continue;
		    }
		    else if (tick_quote && *s == '\'') {
			if ((destsize -= 3) <= 0)
			    abort_interp();
			*dest++ = '\'';
			*dest++ = '\\';
			*dest++ = '\'';
		    }
		    *dest++ = *s++;
		}
	    } else {
		/* straight copy. */
		if (s == dest) {
		    dest += i;
		} else {
		    while (*s)
			*dest++ = *s++;
		}
	    }
	}
	else {
	    if (--destsize <= 0)
		abort_interp();
	    if (*pattern == '^' && pattern[1]) {
		pattern++;
		i = *(Uchar*)pattern;	/* get char after arrow into a register */
		if (i == '?')
		    *dest++ = '\177' | metabit;
		else if (i == '(') {
		    metabit = 0200;
		    destsize++;
		}
		else if (i == ')') {
		    metabit = 0;
		    destsize++;
		}
		else if (i >= '@')
		    *dest++ = (i & 037) | metabit;
		else
		    *dest++ = *--pattern | metabit;
		pattern++;
	    }
	    else if (*pattern == '\\' && pattern[1]) {
		++pattern;		/* skip backslash */
		pattern = interp_backslash(dest, pattern) + 1;
		*dest++ |= metabit;
	    }
	    else if (*pattern)
		*dest++ = *pattern++ | metabit;
	}
    }
    *dest = '\0';
    if (line_split != NULL)
	if ((int)strlen(orig_dest) > 79)
	    *line_split = '\n';
getout:
    safefree(subj_buf);		/* return any checked out storage */
    safefree(ngs_buf);
    safefree(refs_buf);
    safefree(artid_buf);
    safefree(reply_buf);
    safefree(from_buf);
    safefree(path_buf);
    safefree(follow_buf);
    safefree(dist_buf);
    safefree(line_buf);
#ifdef DEBUG
    if (debug & DEB_INTRP)
	printf("<dointerp: %s\n",orig_dest);
#endif
    return pattern;			/* where we left off */
}

char*
interp_backslash(dest,pattern)
char* dest;
register char* pattern;
{
    register int i = *pattern;

    if (i >= '0' && i <= '7') {
	i = 0;
	while (i < 01000 && *pattern >= '0' && *pattern <= '7') {
	    i <<= 3;
	    i += *pattern++ - '0';
	}
	*dest = (char)(i & 0377);
	return pattern - 1;
    }
    switch (i) {
    case 'b':
	*dest = '\b';
	break;
    case 'f':
	*dest = '\f';
	break;
    case 'n':
	*dest = '\n';
	break;
    case 'r':
	*dest = '\r';
	break;
    case 't':
	*dest = '\t';
	break;
    case '\0':
	*dest = '\\';
	return pattern - 1;
    default:
	*dest = (char)i;
	break;
    }
    return pattern;
}

/* helper functions */

char*
interp(dest,destsize,pattern)
register char* dest;
register int destsize;
register char* pattern;
{
    return dointerp(dest,destsize,pattern,(char*)NULL,(char*)NULL);
}

char*
interpsearch(dest,destsize,pattern,cmd)
register char* dest;
register int destsize;
register char* pattern;
char* cmd;
{
    return dointerp(dest,destsize,pattern,(char*)NULL,cmd);
}

/* normalize a references line in place */

void
normalize_refs(refs)
char* refs;
{
    register char* f;
    register char* t;
    
    for (f = t = refs; *f; ) {
	if (*f == '<') {
	    while (*f && (*t++ = *f++) != '>') ;
	    while (*f == ' ' || *f == '\t' || *f == '\n' || *f == ',') f++;
	    if (f != t)
		*t++ = ' ';
	}
	else
	    f++;
    }
    if (t != refs && t[-1] == ' ')
	t--;
    *t = '\0';
} 

static void
abort_interp()
{
    fputs("\n% interp buffer overflow!\n",stdout) FLUSH;
    sig_catcher(0);
}
