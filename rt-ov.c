/* rt-ov.c
*/
/* This software is copyrighted as detailed in the LICENSE file. */


#include "EXTERN.h"
#include "common.h"
#include "list.h"
#include "trn.h"
#include "hash.h"
#include "cache.h"
#include "bits.h"
#include "head.h"
#include "ngdata.h"
#include "nntpclient.h"
#include "datasrc.h"
#include "nntp.h"
#include "util.h"
#include "util2.h"
#include "env.h"
#include "ng.h"
#include "term.h"
#include "intrp.h"
#include "final.h"
#include "rthread.h"
#include "rt-process.h"
#include "rt-util.h"
#include "parsedate.h"
#include "INTERN.h"
#include "rt-ov.h"
#include "rt-ov.ih"

bool
ov_init()
{
    bool has_overview_fmt;
    Uchar* fieldnum = datasrc->fieldnum;
    Uchar* fieldflags = datasrc->fieldflags;
    datasrc->flags &= ~DF_TRY_OVERVIEW;
#ifdef SUPPORT_NNTP
    if (!datasrc->over_dir) {
	int ret;
	/* Check if the server is XOVER compliant */
	if (nntp_command("XOVER") <= 0)
	    return FALSE;
	if (nntp_check() < 0)
	    return FALSE;/*$$*/
	if (atoi(ser_line) == NNTP_BAD_COMMAND_VAL)
	    return FALSE;
	/* Just in case... */
	if (*ser_line == NNTP_CLASS_OK)
	    nntp_finish_list();
	if ((ret = nntp_list("overview.fmt",nullstr,0)) < -1)
	    return FALSE;
	has_overview_fmt = ret > 0;
    }
    else
#endif
    {
	has_overview_fmt = datasrc->over_fmt != NULL
			&& (tmpfp = fopen(datasrc->over_fmt, "r")) != NULL;
    }

    if (has_overview_fmt) {
	int i;
	fieldnum[0] = OV_NUM;
	fieldflags[OV_NUM] = FF_HAS_FIELD;
	for (i = 1;;) {
#ifdef SUPPORT_NNTP
	    if (!datasrc->over_dir) {
		if (nntp_gets(buf, sizeof buf) < 0)
		    break;/*$$*/
		if (nntp_at_list_end(buf))
		    break;
	    }
#endif
	    ElseIf (!fgets(buf, sizeof buf, tmpfp)) {
		fclose(tmpfp);
		break;
	    }
	    if (*buf == '#')
		continue;
	    if (i < OV_MAX_FIELDS) {
		char *s = index(buf,':');
		fieldnum[i] = ov_num(buf,s);
		fieldflags[fieldnum[i]] = FF_HAS_FIELD |
		    ((s && strncaseEQ("full",s+1,4))? FF_HAS_HDR : 0);
		i++;
	    }
	}
	if (!fieldflags[OV_SUBJ] || !fieldflags[OV_MSGID]
	 || !fieldflags[OV_FROM] || !fieldflags[OV_DATE])
	    return FALSE;
	if (i < OV_MAX_FIELDS) {
	    int j;
	    for (j = OV_MAX_FIELDS; j--; ) {
		if (!fieldflags[j])
		    break;
	    }
	    while (i < OV_MAX_FIELDS)
		fieldnum[i++] = j;
	}
    }
    else {
	int i;
	for (i = 0; i < OV_MAX_FIELDS; i++) {
	    fieldnum[i] = i;
	    fieldflags[i] = FF_HAS_FIELD;
	}
	fieldflags[OV_XREF] = FF_CHECK4FIELD | FF_CHECK4HDR;
    }
    datasrc->flags |= DF_TRY_OVERVIEW;
    return TRUE;
}

int
ov_num(hdr,end)
char* hdr;
char* end;
{
    if (!end)
	end = hdr + strlen(hdr);

    switch (set_line_type(hdr,end)) {
      case SUBJ_LINE:
	return OV_SUBJ;
      case AUTHOR_LINE:		/* This hack is for the Baen NNTP server */
      case FROM_LINE:
	return OV_FROM;
      case DATE_LINE:
	return OV_DATE;
      case MSGID_LINE:
	return OV_MSGID;
      case REFS_LINE:
	return OV_REFS;
      case BYTES_LINE:
	return OV_BYTES;
      case LINES_LINE:
	return OV_LINES;
      case XREF_LINE:
	return OV_XREF;
    }
    return 0;
}

/* Process the data in the group's news-overview file.
*/
bool
ov_data(first, last, cheating)
ART_NUM first, last;
bool_int cheating;
{
    ART_NUM artnum, an;
    char* line;
    char* last_buf = buf;
    MEM_SIZE last_buflen = LBUFLEN;
    bool success = TRUE;
    ART_NUM real_first = first;
#ifdef SUPPORT_NNTP
    ART_NUM real_last = last;
    int line_cnt;
    int ov_chunk_size = cheating? OV_CHUNK_SIZE : OV_CHUNK_SIZE * 8;
#endif
    time_t started_request;
    bool remote = !datasrc->over_dir;

#ifdef SUPPORT_NNTP
beginning:
#endif
    for (;;) {
	artnum = article_first(first);
	if (artnum > first || !(article_ptr(artnum)->flags & AF_CACHED))
	    break;
	spin_todo--;
	first++;
    }
    if (first > last)
	goto exit;
#ifdef SUPPORT_NNTP
    if (remote) {
	if (last - first > ov_chunk_size + ov_chunk_size/2 - 1) {
	    last = first + ov_chunk_size - 1;
	    line_cnt = 0;
	}
    }
#endif
    started_request = time((time_t*)NULL);
    for (;;) {
	artnum = article_last(last);
	if (artnum < last || !(article_ptr(artnum)->flags & AF_CACHED))
	    break;
	spin_todo--;
	last--;
    }

#ifdef SUPPORT_NNTP
    if (remote) {
	sprintf(ser_line, "XOVER %ld-%ld", (long)first, (long)last);
	if (nntp_command(ser_line) <= 0 || nntp_check() <= 0) {
	    success = FALSE;
	    goto exit;
	}
# ifdef VERBOSE
	IF(verbose && !first_subject && !datasrc->ov_opened)
	    printf("\nGetting overview file."), fflush(stdout);
# endif
    }
#endif
    ElseIf (datasrc->ov_opened < started_request - 60*60) {
	ov_close();
	if ((datasrc->ov_in = fopen(ov_name(ngname), "r")) == NULL)
	    return FALSE;
#ifdef VERBOSE
	IF(verbose && !first_subject)
	    printf("\nReading overview file."), fflush(stdout);
#endif
    }
    if (!datasrc->ov_opened) {
	if (cheating)
	    setspin(SPIN_BACKGROUND);
	else {
#ifdef SUPPORT_NNTP
	    int lots2do = ((datasrc->flags & DF_REMOTE)? netspeed : 20) * 100;
#else
	    int lots2do = 20 * 100;
#endif
	    if (spin_estimate > spin_todo)
		spin_estimate = spin_todo;
	    setspin(spin_estimate > lots2do? SPIN_BARGRAPH : SPIN_FOREGROUND);
	}
	datasrc->ov_opened = started_request;
    }

    artnum = first-1;
    for (;;) {
#ifdef SUPPORT_NNTP
	if (remote) {
	    line = nntp_get_a_line(last_buf,last_buflen,last_buf!=buf);
	    if (nntp_at_list_end(line))
		break;
	    line_cnt++;
	}
#endif
	ElseIf (!(line = get_a_line(last_buf,last_buflen,last_buf!=buf,datasrc->ov_in)))
	    break;

	last_buf = line;
	last_buflen = buflen_last_line_got;
	an = atol(line);
	if (an < first)
	    continue;
	if (an > last) {
	    artnum = last;
#ifdef SUPPORT_NNTP
	    if (remote)
		continue;
#endif
	    break;
	}
	spin_todo -= an - artnum - 1;
	ov_parse(line, artnum = an, remote);
	if (int_count) {
	    int_count = 0;
	    success = FALSE;
#ifdef SUPPORT_NNTP
	    if (!remote)
#endif
		break;
	}
	if (!remote && cheating) {
	    if (input_pending()) {
		success = FALSE;
		break;
	    }
	    if (curr_artp != sentinel_artp) {
		pushchar('\f' | 0200);
		success = FALSE;
		break;
	    }
	}
    }
#ifdef SUPPORT_NNTP
    if (remote && line_cnt == 0 && last < real_last) {
	an = nntp_find_real_art(last);
	if (an > 0) {
	    last = an - 1;
	    spin_todo -= last - artnum;
	    artnum = last;
	}
    }
    if (remote) {
	int cachemask = (ThreadedGroup? AF_THREADED : AF_CACHED);
	ARTICLE* ap;
	for (ap = article_ptr(article_first(real_first));
	     ap && article_num(ap) <= artnum;
	     ap = article_nextp(ap))
	{
	    if (!(ap->flags & cachemask))
		onemissing(ap);
	}
	spin_todo -= last - artnum;
    }
#endif
    if (artnum > last_cached && artnum >= first)
	last_cached = artnum;
  exit:
    if (int_count || !success) {
	int_count = 0;
	success = FALSE;
    }
#ifdef SUPPORT_NNTP
    else if (remote) {
	if (cheating && curr_artp != sentinel_artp) {
	    pushchar('\f' | 0200);
	    success = FALSE;
	} else if (last < real_last) {
	    if (!cheating || !input_pending()) {
		long elapsed_time = time((time_t*)NULL) - started_request;
		long expected_time = cheating? 2 : 10;
		int max_chunk_size = cheating? 500 : 2000;
		ov_chunk_size += (expected_time - elapsed_time) * OV_CHUNK_SIZE;
		if (ov_chunk_size <= OV_CHUNK_SIZE / 2)
		    ov_chunk_size = OV_CHUNK_SIZE / 2 + 1;
		else if (ov_chunk_size > max_chunk_size)
		    ov_chunk_size = max_chunk_size;
		first = last+1;
		last = real_last;
		goto beginning;
	    }
	    success = FALSE;
	}
    }
#endif
    if (!cheating && datasrc->ov_in)
	fseek(datasrc->ov_in, 0L, 0);	/* rewind it for the cheating phase */
    if (success && real_first <= first_cached) {
	first_cached = real_first;
	cached_all_in_range = TRUE;
    }
    setspin(SPIN_POP);
    if (last_buf != buf)
	free(last_buf);
    return success;
}

static void
ov_parse(line, artnum, remote)
register char* line;
ART_NUM artnum;
bool_int remote;
{
    register ARTICLE* article;
    register int i;
    int fn;
    Uchar* fieldnum = datasrc->fieldnum;
    Uchar* fieldflags = datasrc->fieldflags;
    char* fields[OV_MAX_FIELDS];
    char* cp;
    char* tab;

    article = article_ptr(artnum);
    if (article->flags & AF_THREADED) {
	spin_todo--;
	return;
    }

    if (len_last_line_got > 0 && line[len_last_line_got-1] == '\n') {
#ifdef SUPPORT_NNTP
	if (len_last_line_got > 1 && line[len_last_line_got-2] == '\r')
	    line[len_last_line_got-2] = '\0';
	else
#endif
	    line[len_last_line_got-1] = '\0';
    }
    cp = line;

    bzero((char*)fields, sizeof fields);
    for (i = 0; cp && i < OV_MAX_FIELDS; cp = tab) {
	if ((tab = index(cp, '\t')) != NULL)
	    *tab++ = '\0';
	fn = fieldnum[i];
	if (!(fieldflags[fn] & (FF_HAS_FIELD | FF_CHECK4FIELD)))
	    break;
	if (fieldflags[fn] & (FF_HAS_HDR | FF_CHECK4HDR)) {
	    char* s = index(cp, ':');
	    if (fieldflags[fn] & FF_CHECK4HDR) {
		if (s)
		    fieldflags[fn] |= FF_HAS_HDR;
		fieldflags[fn] &= ~FF_CHECK4HDR;
	    }
	    if (fieldflags[fn] & FF_HAS_HDR) {
		if (!s)
		    break;
		if (s - cp != htype[hdrnum[fn]].length
		 || strncaseNE(cp,htype[hdrnum[fn]].name,htype[hdrnum[fn]].length))
		    continue;
		cp = s;
		while (*++cp == ' ') ;
	    }
	}
	fields[fn] = cp;
	i++;
    }
    if (!fields[OV_SUBJ] || !fields[OV_MSGID]
     || !fields[OV_FROM] || !fields[OV_DATE])
	return;		/* skip this line if it's too short */

    if (!article->subj)
	set_subj_line(article, fields[OV_SUBJ], strlen(fields[OV_SUBJ]));
    if (!article->msgid)
	set_cached_line(article, MSGID_LINE, savestr(fields[OV_MSGID]));
    if (!article->from)
	set_cached_line(article, FROM_LINE, savestr(fields[OV_FROM]));
    if (!article->date)
	article->date = parsedate(fields[OV_DATE]);
#ifdef USE_FILTER
    if (!article->refs && fields[OV_REFS])
	set_cached_line(article, REFS_LINE, *fields[OV_REFS]? savestr(fields[OV_REFS]) : nullstr);
#endif
    if (!article->bytes && fields[OV_BYTES])
	set_cached_line(article, BYTES_LINE, fields[OV_BYTES]);
    if (!article->lines && fields[OV_LINES])
	set_cached_line(article, LINES_LINE, fields[OV_LINES]);

    if (fieldflags[OV_XREF] & (FF_HAS_FIELD | FF_CHECK4FIELD)) {
	if (!article->xrefs && fields[OV_XREF]) {
	    /* Exclude an xref for just this group */
	    cp = index(fields[OV_XREF], ':');
	    if (cp && index(cp+1, ':'))
		article->xrefs = savestr(fields[OV_XREF]);
	}

	if (fieldflags[OV_XREF] & FF_HAS_FIELD) {
	    if (!article->xrefs)
		article->xrefs = nullstr;
	}
	else if (fields[OV_XREF]) {
	    ART_NUM an;
	    ARTICLE* ap;
	    for (an=article_first(absfirst); an<artnum; an=article_next(an)) {
		ap = article_ptr(an);
		if (!ap->xrefs)
		    ap->xrefs = nullstr;
	    }
	    fieldflags[OV_XREF] |= FF_HAS_FIELD;
	}
    }

    if (remote)
	article->flags |= AF_EXISTS;

    if (ThreadedGroup) {
	if (valid_article(article))
	    thread_article(article, fields[OV_REFS]);
    } else if (!(article->flags & AF_CACHED))
	cache_article(article);

    if (article->flags & AF_UNREAD)
	check_poster(article);
    spin(100);
}

/* Change a newsgroup name into the name of the overview data file.  We
** subsitute any '.'s in the group name into '/'s, prepend the path, and
** append the '/.overview' or '.ov') on to the end.
*/
static char*
ov_name(group)
char* group;
{
    register char* cp;

    strcpy(buf, datasrc->over_dir);
    cp = buf + strlen(buf);
    *cp++ = '/';
    strcpy(cp, group);
    while ((cp = index(cp, '.')))
	*cp = '/';
    strcat(buf, OV_FILE_NAME);
    return buf;
}

void
ov_close()
{
    if (datasrc && datasrc->ov_opened) {
	if (datasrc->ov_in) {
	    (void) fclose(datasrc->ov_in);
	    datasrc->ov_in = NULL;
	}
	datasrc->ov_opened = 0;
    }
}

char*
ov_fieldname(num)
int num;
{
    return htype[hdrnum[num]].name;
}

char*
ov_field(ap, num)
ARTICLE* ap;
int num;
{
    char* s;
    int fn;

    fn = datasrc->fieldnum[num];
    if (!(datasrc->fieldflags[fn] & (FF_HAS_FIELD | FF_CHECK4FIELD)))
	return NULL;

    if (fn == OV_NUM) {
	sprintf(cmd_buf, "%ld", (long)ap->num);
	return cmd_buf;
    }

    if (fn == OV_DATE) {
	sprintf(cmd_buf, "%ld", (long)ap->date);
	return cmd_buf;
    }

    s = get_cached_line(ap, hdrnum[fn], TRUE);
    return s? s : nullstr;
}
