/* cache.c
 * vi: set sw=4 ts=8 ai sm noet :
 */
/* This software is copyrighted as detailed in the LICENSE file. */


#include "EXTERN.h"
#include "common.h"
#include "list.h"
#include "intrp.h"
#include "search.h"
#include "ng.h"
#include "trn.h"
#include "hash.h"
#include "ngdata.h"
#include "nntpclient.h"
#include "datasrc.h"
#include "nntp.h"
#include "term.h"
#include "final.h"
#include "artsrch.h"
#include "head.h"
#include "mime.h"
#include "bits.h"
#include "kfile.h"
#include "rcstuff.h"
#include "rthread.h"
#include "rt-ov.h"
#include "rt-page.h"
#include "rt-process.h"
#include "rt-select.h"
#include "rt-util.h"
#ifdef SCORE
#include "score.h"
#endif
#include "env.h"
#include "util.h"
#include "util2.h"
#include "utf.h"
#include "INTERN.h"
#include "cache.h"
#include "cache.ih"

#ifdef PENDING
#   ifdef ARTSEARCH
	COMPEX srchcompex;		/* compiled regex for searchahead */
#   endif
#endif

HASHTABLE* subj_hash = 0;
HASHTABLE* shortsubj_hash = 0;

void
cache_init()
{
#ifdef PENDING
# ifdef ARTSEARCH
    init_compex(&srchcompex);
# endif
#endif
}

static NGDATA* cached_ng = NULL;
static time_t cached_time = 0;

void
build_cache()
{
    if (cached_ng == ngptr && time((time_t*)NULL) < cached_time + 6*60*60L) {
	ART_NUM an;
	cached_time = time((time_t*)NULL);
	if (sel_mode == SM_ARTICLE)
	    set_selector(sel_mode, sel_artsort);
	else
	    set_selector(sel_threadmode, sel_threadsort);
	for (an = last_cached+1; an <= lastart; an++)
	    article_ptr(an)->flags |= AF_EXISTS;
	rc_to_bits();
	article_list->high = lastart;
	thread_grow();
	return;
    }

    close_cache();

    cached_ng = ngptr;
    cached_time = time((time_t*)NULL);
    article_list = new_list(absfirst, lastart, sizeof (ARTICLE), 371,
			    LF_SPARSE, init_artnode);
    subj_hash = hashcreate(991, subject_cmp);	/*TODO: pick a better size */

    set_firstart(ngptr->rcline + ngptr->numoffset);
    first_cached = thread_always? absfirst : firstart;
    last_cached = first_cached-1;
    cached_all_in_range = FALSE;
#ifdef PENDING
    subj_to_get = xref_to_get = firstart;
#endif

    /* Cache as much data in advance as possible, possibly threading
    ** articles as we go. */
    thread_open();
}

void
close_cache()
{
    SUBJECT* sp;
    SUBJECT* next;

#ifdef SUPPORT_NNTP
    nntp_artname(0, FALSE);		/* clear the tmpfile cache */
#endif

    if (subj_hash) {
	hashdestroy(subj_hash);
	subj_hash = 0;
    }
    if (shortsubj_hash) {
	hashdestroy(shortsubj_hash);
	shortsubj_hash = 0;
    }
    /* Free all the subjects. */
    for (sp = first_subject; sp; sp = next) {
	next = sp->next;
	free(sp->str);
	free((char*)sp);
    }
    first_subject = last_subject = NULL;
    subject_count = 0;			/* just to be sure */
    parsed_art = 0;

    if (artptr_list) {
	free((char*)artptr_list);
	artptr_list = NULL;
    }
    artptr = NULL;
    thread_close();

    if (article_list) {
	walk_list(article_list, clear_artitem, 0);
	delete_list(article_list);
	article_list = NULL;
    }
    cached_ng = NULL;
}

/* Initialize the memory for an entire node's worth of article's */
static void
init_artnode(list, node)
LIST* list;
LISTNODE* node;
{
    register ART_NUM i;
    register ARTICLE* ap;
    bzero(node->data, list->items_per_node * list->item_size);
    for (i = node->low, ap = (ARTICLE*)node->data; i <= node->high; i++, ap++)
	ap->num = i;
}

static bool
clear_artitem(cp, arg)
char* cp;
int arg;
{
    clear_article((ARTICLE*)cp);
    return 0;
}

/* The article has all it's data in place, so add it to the list of articles
** with the same subject.
*/
void
cache_article(ap)
register ARTICLE* ap;
{
    register ARTICLE* next;
    register ARTICLE* ap2;

    if (!(next = ap->subj->articles) || ap->date < next->date)
	ap->subj->articles = ap;
    else {
	while ((next = (ap2 = next)->subj_next) && next->date <= ap->date)
	    ;
	ap2->subj_next = ap;
    }
    ap->subj_next = next;
    ap->flags |= AF_CACHED;

    if (!!(ap->flags & AF_UNREAD) ^ sel_rereading) {
	if (ap->subj->flags & sel_mask)
	    select_article(ap, 0);
	else {
	    if (ap->subj->flags & SF_WASSELECTED) {
#if 0
		if (selected_only)
		    ap->flags |= sel_mask;
		else
#endif
		    select_article(ap, 0);
	    }
	    ap->subj->flags |= SF_VISIT;
	}
    }

    if (join_subject_len != 0)
	check_for_near_subj(ap);
}

void
check_for_near_subj(ap)
ARTICLE* ap;
{
    register SUBJECT* sp;
    if (!shortsubj_hash) {
	shortsubj_hash = hashcreate(401, subject_cmp);	/*TODO: pick a better size */
	sp = first_subject;
    }
    else {
	sp = ap->subj;
	if (sp->next)
	    sp = 0;
    }
    while (sp) {
	if ((int)strlen(sp->str+4) >= join_subject_len && sp->thread) {
	    SUBJECT* sp2;
	    HASHDATUM data;
	    data = hashfetch(shortsubj_hash, sp->str+4, join_subject_len);
	    if (!(sp2 = (SUBJECT*)data.dat_ptr)) {
		data.dat_ptr = (char*)sp;
		hashstorelast(data);
	    }
	    else if (sp->thread != sp2->thread) {
		merge_threads(sp2, sp);
	    }
	}
	sp = sp->next;
    }
}

void
change_join_subject_len(len)
int len;
{
    if (join_subject_len != len) {
	if (shortsubj_hash) {
	    hashdestroy(shortsubj_hash);
	    shortsubj_hash = 0;
	}
	join_subject_len = len;
	if (len && first_subject && first_subject->articles)
	    check_for_near_subj(first_subject->articles);
    }
}

void
check_poster(ap)
register ARTICLE* ap;
{
    if (auto_select_postings && (ap->flags & AF_EXISTS) && ap->from) {
	if (ap->flags & AF_FROMTRUNCED) {
	    strcpy(cmd_buf,realName);
	    if (strEQ(ap->from,compress_name(cmd_buf,16))) {
		untrim_cache = TRUE;
		fetchfrom(article_num(ap),FALSE);
		untrim_cache = FALSE;
	    }
	}
	if (!(ap->flags & AF_FROMTRUNCED)) {
	    char* s = cmd_buf;
	    char* u;
	    char* h;
	    strcpy(s,ap->from);
	    if ((h=index(s,'<')) != NULL) { /* grab the good part */
		s = h+1;
		if ((h=index(s,'>')) != NULL)
		    *h = '\0';
	    } else if ((h=index(s,'(')) != NULL) {
		while (h-- != s && *h == ' ')
		    ;
		h[1] = '\0';		/* or strip the comment */
	    }
	    if ((h = index(s,'%')) != NULL || (h = index(s,'@')) != NULL) {
		*h++ = '\0';
		u = s;
	    } else if ((u = rindex(s,'!')) != NULL) {
		*u++ = '\0';
		h = s;
	    } else
		h = u = s;
	    if (strEQ(u,loginName)) {
		if (instr(h,hostname,FALSE)) {
		    switch (auto_select_postings) {
		      case '.':
			select_subthread(ap,AUTO_SEL_FOL);
			break;
		      case '+':
			select_arts_thread(ap,AUTO_SEL_THD);
			break;
		      case 'p':
			if (ap->parent)
			    select_subthread(ap->parent,AUTO_SEL_FOL);
			else
			    select_subthread(ap,AUTO_SEL_FOL);
			break;
		    }
		} else {
#ifdef REPLYTO_POSTER_CHECKING
		    char* reply_buf = fetchlines(article_num(ap),REPLY_LINE);
		    if (instr(reply_buf,loginName,TRUE))
			select_subthread(ap,AUTO_SEL_FOL);
		    free(reply_buf);
#endif
		}
	    }
	}
    }
}

/* The article turned out to be a duplicate, so remove it from the cached
** list and possibly destroy the subject (should only happen if the data
** was corrupt and the duplicate id got a different subject).
*/
void
uncache_article(ap, remove_empties)
register ARTICLE* ap;
bool_int remove_empties;
{
    register ARTICLE* next;

    if (ap->subj) {
	if (ALLBITS(ap->flags, AF_CACHED | AF_EXISTS)) {
	    if ((next = ap->subj->articles) == ap)
		ap->subj->articles = ap->subj_next;
	    else {
		register ARTICLE* ap2;
		while (next) {
		    if ((ap2 = next->subj_next) == ap) {
			next->subj_next = ap->subj_next;
			break;
		    }
		    next = ap2;
		}
	    }
	}
	if (remove_empties && !ap->subj->articles) {
	    register SUBJECT* sp = ap->subj;
	    if (sp == first_subject)
		first_subject = sp->next;
	    else
		sp->prev->next = sp->next;
	    if (sp == last_subject)
		last_subject = sp->prev;
	    else
		sp->next->prev = sp->prev;
	    hashdelete(subj_hash, sp->str+4, strlen(sp->str+4));
	    free((char*)sp);
	    ap->subj = NULL;
	    subject_count--;
	}
    }
    ap->flags2 |= AF2_BOGUS;
    onemissing(ap);
}

/* get the header line from an article's cache or parse the article trying */

char*
fetchcache(artnum,which_line,fill_cache)
ART_NUM artnum;
int which_line;
bool_int fill_cache;
{
    register char* s;
    register ARTICLE* ap;
    register bool cached = (htype[which_line].flags & HT_CACHED);

    /* article_find() returns a NULL if the artnum value is invalid */
    if (!(ap = article_find(artnum)) || !(ap->flags & AF_EXISTS))
	return nullstr;
    if (cached && (s=get_cached_line(ap,which_line,untrim_cache)) != NULL)
	return s;
    if (!fill_cache)
	return NULL;
    if (!parseheader(artnum))
	return nullstr;
    if (cached)
	return get_cached_line(ap,which_line,untrim_cache);
    return NULL;
}

/* Return a pointer to a cached header line for the indicated article.
** Truncated headers (e.g. from a .thread file) are optionally ignored.
*/
char*
get_cached_line(ap, which_line, no_truncs)
register ARTICLE* ap;
int which_line;
bool_int no_truncs;
{
    register char* s;

    switch (which_line) {
      case SUBJ_LINE:
	if (!ap->subj || (no_truncs && (ap->subj->flags & SF_SUBJTRUNCED)))
	    s = NULL;
	else
	    s = ap->subj->str + ((ap->flags & AF_HAS_RE) ? 0 : 4);
	break;
      case FROM_LINE:
	if (no_truncs && (ap->flags & AF_FROMTRUNCED))
	    s = NULL;
	else
	    s = ap->from;
	break;
#ifdef DBM_XREFS
      case NGS_LINE:
#else
      case XREF_LINE:
#endif
	s = ap->xrefs;
	break;
      case MSGID_LINE:
	s = ap->msgid;
	break;
#ifdef USE_FILTER
      case REFS_LINE:
	s = ap->refs;
	break;
#endif
      case LINES_LINE: {
	static char linesbuf[32];
	sprintf(linesbuf, "%ld", ap->lines);
	s = linesbuf;
	break;
      }
      case BYTES_LINE: {
	static char bytesbuf[32];
	sprintf(bytesbuf, "%ld", ap->bytes);
	s = bytesbuf;
	break;
      }
      default:
	s = NULL;
	break;
    }
    return s;
}

void
set_subj_line(ap, subj, size)
ARTICLE* ap;
char* subj;	/* not yet allocated, so we can tweak it first */
int size;
{
    HASHDATUM data;
    SUBJECT* sp;
    char* newsubj;
    char* subj_start;
    short def_flags = 0;

    if (subject_has_Re(subj, &subj_start))
	ap->flags |= AF_HAS_RE;
    if ((size -= subj_start - subj) < 0)
	size = 0;

    newsubj = safemalloc(size + 4 + 1);
    strcpy(newsubj, "Re: ");
    size = decode_header(newsubj + 4, subj_start, size);

    /* Do the Re:-stripping over again, just in case it was encoded. */
    if (subject_has_Re(newsubj + 4, &subj_start))
	ap->flags |= AF_HAS_RE;
    if (subj_start != newsubj + 4) {
	safecpy(newsubj + 4, subj_start, size);
	if ((size -= subj_start - newsubj - 4) < 0)
	    size = 0;
    }
    if (ap->subj && strnEQ(ap->subj->str+4, newsubj+4, size)) {
	free(newsubj);
	return;
    }

    if (ap->subj) {
	/* This only happens when we freshen truncated subjects */
	hashdelete(subj_hash, ap->subj->str+4, strlen(ap->subj->str+4));
	free(ap->subj->str);
	ap->subj->str = newsubj;
	ap->subj->flags |= def_flags;
	data.dat_ptr = (char*)ap->subj;
	hashstore(subj_hash, newsubj + 4, size, data);
    } else {
	data = hashfetch(subj_hash, newsubj + 4, size);
	if (!(sp = (SUBJECT*)data.dat_ptr)) {
	    sp = (SUBJECT*)safemalloc(sizeof (SUBJECT));
	    bzero((char*)sp, sizeof (SUBJECT));
	    subject_count++;
	    if ((sp->prev = last_subject) != NULL)
		sp->prev->next = sp;
	    else
		first_subject = sp;
	    last_subject = sp;
	    sp->str = newsubj;
	    sp->thread_link = sp;
	    sp->flags = def_flags;

	    data.dat_ptr = (char*)sp;
	    hashstorelast(data);
	} else
	    free(newsubj);
	ap->subj = sp;
    }
}

int
decode_header(t, f, size)
char* t;
char* f;
int size;
{
    int i;
    char *s = t; /* save for pass 2 */
    bool pass2needed = FALSE;

    /* Pass 1 to decode coded bytes (which might be character fragments - so 1 pass is wrong) */
    for (i = size; *f && i--; ) {
	if (*f == '=' && f[1] == '?') {
	    char* q = index(f+2,'?');
	    char ch = (q && q[2] == '?')? q[1] : 0;
	    char* e;

	    if (ch == 'q' || ch == 'Q' || ch == 'b' || ch == 'B') {
		const char* old_ics = input_charset_name();
		const char* old_ocs = output_charset_name();
#ifdef USE_UTF_HACK
		*q = '\0';
		utf_init(f+2, "utf-8"); /*FIXME*/
		*q = '?';
#endif
		e = q+2;
		do {
		    e = index(e+1, '?');
		} while (e && e[1] != '=');
		if (e) {
		    int len = e - f + 2;
#ifdef USE_UTF_HACK
		    char *d;
#endif
		    i -= len-1;
		    size -= len;
		    q += 3;
		    f = e+2;
		    *e = '\0';
		    if (ch == 'q' || ch == 'Q')
			len = qp_decodestring(t, q, 1);
		    else
			len = b64_decodestring(t, q);
#ifdef USE_UTF_HACK
		    d = create_utf8_copy(t);
		    if (d) {
			for (len = 0; d[len]; ) {
			    t[len] = d[len];
			    len++;
			}
			free(d);
		    }
#endif
		    *e = '?';
		    t += len;
		    size += len;
		    /* If the next character is whitespace we should eat it now */
		    while (*f == ' ' || *f == '\t')
			f++;
		}
		else
		    *t++ = *f++;
#ifdef USE_UTF_HACK
		utf_init(old_ics, old_ocs);
#endif
	    }
	    else
		*t++ = *f++;
	} else if (*f != '\n')
	    *t++ = *f++;
	else
	    f++, size--;
	pass2needed = TRUE;
    }
    while (size > 1 && t[-1] == ' ')
	t--, size--;
    *t = '\0';

    /* Pass 2 to clear out "control" characters */
    if (pass2needed)
	dectrl(s);
    return size;
}

void
dectrl(str)
char* str;
{
    if (str == NULL)
	return;

    for ( ; *str;) {
	int w = byte_length_at(str);
	if (AT_GREY_SPACE(str)) {
	    register int i;
	    for (i = 0; i < w; i += 1) {
		str[i] = ' ';
	    }
	}
	str += w;
    }
}

void
set_cached_line(ap, which_line, s)
register ARTICLE* ap;
register int which_line;
register char* s;		/* already allocated, ready to save */
{
    char* cp;
    /* SUBJ_LINE is handled specially above */
    switch (which_line) {
      case FROM_LINE:
	ap->flags &= ~AF_FROMTRUNCED;
	if (ap->from)
	    free(ap->from);
	decode_header(s, s, strlen(s));
	ap->from = s;
	break;
#ifdef DBM_XREFS
      case NGS_LINE:
	if (ap->xrefs && ap->xrefs != nullstr)
	    free(ap->xrefs);
	if (!index(s, ',')) {	/* if no comma, no Xref! */
	    free(s);
	    s = nullstr;
	}
	ap->xrefs = s;
	break;
#else
      case XREF_LINE:
	if (ap->xrefs && ap->xrefs != nullstr)
	    free(ap->xrefs);
	/* Exclude an xref for just this group or "(none)". */
	cp = index(s, ':');
	if (!cp || !index(cp+1, ':')) {
	    free(s);
	    s = nullstr;
	}
	ap->xrefs = s;
	break;
#endif
      case MSGID_LINE:
	if (ap->msgid)
	    free(ap->msgid);
	ap->msgid = s;
	break;
#ifdef USE_FILTER
      case REFS_LINE:
	if (ap->refs && ap->refs != nullstr)
	    free(ap->refs);
	ap->refs = s;
	break;
#endif
      case LINES_LINE:
	ap->lines = atol(s);
	break;
      case BYTES_LINE:
	ap->bytes = atol(s);
	break;
    }
}

int
subject_cmp(key, keylen, data)
char* key;
int keylen;
HASHDATUM data;
{
    /* We already know that the lengths are equal, just compare the strings */
    return bcmp(key, ((SUBJECT*)data.dat_ptr)->str+4, keylen);
}

/* see what we can do while they are reading */

#ifdef PENDING
void
look_ahead()
{
#ifdef ARTSEARCH
    register char* h;
    register char* s;

#ifdef DEBUG
    if (debug && srchahead) {
	printf("(%ld)",(long)srchahead);
	fflush(stdout);
    }
#endif
#endif

    if (ThreadedGroup) {
	artp = curr_artp;
	inc_art(selected_only,FALSE);
	if (artp)
	    parseheader(art);
    }
    else
#ifdef ARTSEARCH
    if (srchahead && srchahead < art) {	/* in ^N mode? */
	char* pattern;

	pattern = buf+1;
	strcpy(pattern,": *");
	h = pattern + strlen(pattern);
	interp(h,(sizeof buf) - (h-buf),"%\\s");
	{			/* compensate for notesfiles */
	    register int i;
	    for (i = 24; *h && i--; h++)
		if (*h == '\\')
		    h++;
	    *h = '\0';
	}
#ifdef DEBUG
	if (debug & DEB_SEARCH_AHEAD) {
	    fputs("(hit CR)",stdout);
	    fflush(stdout);
	    fgets(buf+128, sizeof buf-128, stdin);
	    printf("\npattern = %s\n",pattern);
	    termdown(2);
	}
#endif
	if ((s = compile(&srchcompex,pattern,TRUE,TRUE)) != NULL) {
				    /* compile regular expression */
	    printf("\n%s\n",s) FLUSH;
	    termdown(2);
	    srchahead = 0;
	}
	if (srchahead) {
	    srchahead = art;
	    for (;;) {
		srchahead++;	/* go forward one article */
		if (srchahead > lastart) { /* out of articles? */
#ifdef DEBUG
		    if (debug)
			fputs("(not found)",stdout);
#endif
		    break;
		}
		if (!was_read(srchahead) &&
		    wanted(&srchcompex,srchahead,0)) {
				    /* does the shoe fit? */
#ifdef DEBUG
		    if (debug)
			printf("(%ld)",(long)srchahead);
#endif
		    parseheader(srchahead);
		    break;
		}
		if (input_pending())
		    break;
	    }
	    fflush(stdout);
	}
    }
    else
#endif /* ARTSEARCH */
    {
	if (article_next(art) <= lastart)	/* how about a pre-fetch? */
	    parseheader(article_next(art));	/* look for the next article */
    }
}
#endif /* PENDING */

/* see what else we can do while they are reading */

void
cache_until_key()
{
    if (!in_ng)
	return;
#ifdef PENDING
    if (input_pending())
	return;

# ifdef SUPPORT_NNTP
    if ((datasrc->flags & DF_REMOTE) && nntp_finishbody(FB_BACKGROUND))
	return;
# endif

# ifdef NICEBG
    if (wait_key_pause(10))
	return;
# endif

    untrim_cache = TRUE;
    sentinel_artp = curr_artp;

    /* Prioritize our caching based on what mode we're in */
    if (gmode == 's') {
	if (cache_subjects()) {
	    if (cache_xrefs()) {
		if (chase_xrefs(TRUE)) {
		    if (ThreadedGroup)
			cache_all_arts();
		    else
			cache_unread_arts();
		}
	    }
	}
    } else {
	if (!ThreadedGroup || cache_all_arts()) {
	    if (cache_subjects()) {
		if (cache_unread_arts()) {
		    if (cache_xrefs())
			chase_xrefs(TRUE);
		}
	    }
	}
    }

# ifdef SCORE
    if (!input_pending() && sc_initialized)
	sc_lookahead(TRUE,TRUE);
# endif

    setspin(SPIN_OFF);
    untrim_cache = FALSE;
#endif
#ifdef SUPPORT_NNTP
    check_datasrcs();
#endif
}

#ifdef PENDING
bool
cache_subjects()
{
    register ART_NUM an;

    if (subj_to_get > lastart)
	return TRUE;
    setspin(SPIN_BACKGROUND);
    for (an=article_first(subj_to_get); an <= lastart; an=article_next(an)) {
	if (input_pending())
	    break;
	
	if (article_unread(an))
	    fetchsubj(an,FALSE);
    }
    subj_to_get = an;
    return subj_to_get > lastart;
}

bool
cache_xrefs()
{
    register ART_NUM an;

    if (olden_days || (datasrc->flags & DF_NOXREFS) || xref_to_get > lastart)
	return TRUE;
    setspin(SPIN_BACKGROUND);
    for (an=article_first(xref_to_get); an <= lastart; an=article_next(an)) {
	if (input_pending())
	    break;
	if (article_unread(an))
	    fetchxref(an,FALSE);
    }
    xref_to_get = an;
    return xref_to_get > lastart;
}

bool
cache_all_arts()
{
    int old_last_cached = last_cached;
    if (!cached_all_in_range)
	last_cached = first_cached-1;
    if (last_cached >= lastart && first_cached <= absfirst)
	return TRUE;

    /* turn it on as late as possible to avoid fseek()ing openart */
    setspin(SPIN_BACKGROUND);
    if (last_cached < lastart) {
	if (datasrc->ov_opened)
	    ov_data(last_cached+1, lastart, TRUE);
	if (!art_data(last_cached+1, lastart, TRUE, TRUE)) {
	    last_cached = old_last_cached;
	    return FALSE;
	}
	cached_all_in_range = TRUE;
    }
    if (first_cached > absfirst) {
	if (datasrc->ov_opened)
	    ov_data(absfirst, first_cached-1, TRUE);
	else
	    art_data(absfirst, first_cached-1, TRUE, TRUE);
	/* If we got interrupted, make a quick exit */
	if (first_cached > absfirst) {
	    last_cached = old_last_cached;
	    return FALSE;
	}
    }
    /* We're all done threading the group, so if the current article is
    ** still in doubt, tell them it's missing. */
    if (curr_artp && !(curr_artp->flags & AF_CACHED) && !input_pending())
	pushchar('\f' | 0200);
    /* A completely empty group needs a count & a sort */
    if (gmode != 's' && !obj_count && !selected_only)
	thread_grow();
    return TRUE;
}

bool
cache_unread_arts()
{
    if (last_cached >= lastart)
	return TRUE;
    setspin(SPIN_BACKGROUND);
    return art_data(last_cached+1, lastart, TRUE, FALSE);
}
#endif

bool
art_data(first, last, cheating, all_articles)
ART_NUM first, last;
bool_int cheating;
bool_int all_articles;
{
    register ART_NUM i;
    ART_NUM expected_i = first;

    int cachemask = (ThreadedGroup ? AF_THREADED : AF_CACHED)
		  + (all_articles? 0 : AF_UNREAD);
    int cachemask2 = (all_articles? 0 : AF_UNREAD);

    if (cheating)
	setspin(SPIN_BACKGROUND);
    else {
#ifdef SUPPORT_NNTP
	int lots2do = ((datasrc->flags & DF_REMOTE)? netspeed : 20) * 25;
#else
	int lots2do = 20 * 25;
#endif
	setspin(spin_estimate > lots2do? SPIN_BARGRAPH : SPIN_FOREGROUND);
    }
    /*assert(first >= absfirst && last <= lastart);*/
    for (i = article_first(first); i <= last; i = article_next(i)) {
	if ((article_ptr(i)->flags & cachemask) ^ cachemask2)
	    continue;

	spin_todo -= i - expected_i;
	expected_i = i + 1;

	/* This parses the header which will cache/thread the article */
	(void) parseheader(i);

	if (int_count) {
	    int_count = 0;
	    break;
	}
	if (cheating) {
	    if (input_pending())
		break;
	    /* If the current article is no longer a '?', let them know. */
	    if (curr_artp != sentinel_artp) {
		pushchar('\f' | 0200);
		break;
	    }
	}
    }
    setspin(SPIN_POP);
    if (i > last)
	i = last;
    if (i > last_cached)
	last_cached = i;
    if (i == last) {
	if (first < first_cached)
	    first_cached = first;
	return TRUE;
    }
    return FALSE;
}

bool
cache_range(first,last)
ART_NUM first;
ART_NUM last;
{
    bool success = TRUE;
    bool all_arts = (sel_rereading || thread_always);
    ART_NUM count = 0;

    if (sel_rereading && !cached_all_in_range) {
	first_cached = first;
	last_cached = first-1;
    }
    if (first < first_cached)
	count = first_cached-first;
    if (last > last_cached)
	count += last-last_cached;
    if (!count)
	return TRUE;
    spin_todo = count;

    if (first_cached > last_cached) {
	if (sel_rereading) {
	    if (first_subject)
		count -= ngptr->toread;
	} else if (first == firstart && last == lastart && !all_arts)
	    count = ngptr->toread;
    }
    spin_estimate = count;

    printf("\n%sing %ld article%s.", ThreadedGroup? "Thread" : "Cach",
	   (long)count, PLURAL(count));
    termdown(1);

    setspin(SPIN_FOREGROUND);

    if (first < first_cached) {
	if (datasrc->ov_opened) {
	    ov_data(absfirst,first_cached-1,FALSE);
	    success = (first_cached == absfirst);
	} else {
	    success = art_data(first, first_cached-1, FALSE, all_arts);
	    cached_all_in_range = (all_arts && success);
	}
    }
    if (success && last_cached < last) {
	if (datasrc->ov_opened)
	    ov_data(last_cached+1, last, FALSE);
	success = art_data(last_cached+1, last, FALSE, all_arts);
	cached_all_in_range = (all_arts && success);
    }
    setspin(SPIN_POP);
    return success;
}

void
clear_article(ap)
register ARTICLE* ap;
{
    if (ap->from)
	free(ap->from);
    if (ap->msgid)
	free(ap->msgid);
    if (ap->xrefs && ap->xrefs != nullstr)
	free(ap->xrefs);
#ifdef USE_FILTER
    if (ap->refs && ap->refs != nullstr)
        free(ap->refs);
#endif
}
