/* artio.c
 * vi: set sw=4 ts=8 ai sm noet :
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
#include "cache.h"
#include "rthread.h"
#include "head.h"
#include "mime.h"
#include "term.h"
#include "ng.h"
#include "art.h"
#include "search.h"
#include "artstate.h"
#include "bits.h"
#include "final.h"
#include "util.h"
#include "util2.h"
#include "color.h"
#include "decode.h"
#include "INTERN.h"
#include "artio.h"

void
artio_init()
{
    artbuf_size = 8 * 1024;
    artbuf = safemalloc(artbuf_size);
    clear_artbuf();
}

/* open an article, unless it's already open */

FILE*
artopen(artnum, pos)
ART_NUM artnum;
ART_POS pos;
{
    char artname[MAXFILENAME];		/* filename of current article */
    ARTICLE* ap = article_find(artnum);

    if (!ap || !artnum || (ap->flags & (AF_EXISTS|AF_FAKE)) != AF_EXISTS) {
	errno = ENOENT;
	return NULL;
    }
    if (openart == artnum) {		/* this article is already open? */
	seekart(pos);			/* yes: just seek the file */
	return artfp;			/* and say we succeeded */
    }
    artclose();
retry_open:
#ifdef SUPPORT_NNTP
    if (datasrc->flags & DF_REMOTE)
	nntp_body(artnum);
    else
#endif
    {
	sprintf(artname,"%ld",(long)artnum);
	artfp = fopen(artname,"r");
	/*artio_setbuf(artfp);$$*/
    }
    if (!artfp) {
#ifdef ETIMEDOUT
	if (errno == ETIMEDOUT)
	    goto retry_open;
#endif
	if (errno == EINTR)
	    goto retry_open;
	uncache_article(ap,FALSE);
    } else {
#ifdef LINKART
#ifdef SUPPORT_NNTP
	if (!(datasrc->flags & DF_REMOTE))
#endif
	{
	    char tmpbuf[256];
	    char* s;

	    if (!fstat(fileno(artfp),&filestat)
	     && filestat.st_size < sizeof tmpbuf) {
		fgets(tmpbuf,sizeof tmpbuf,artfp);
		if (FILE_REF(tmpbuf)) {	/* is a "link" to another article */
		    fclose(artfp);
		    if ((s = index(tmpbuf,'\n')) != NULL)
			*s = '\0';
		    if (!(artfp = fopen(tmpbuf,"r")))
			uncache_article(ap,FALSE);
		    else {
			if (*linkartname)
			    free(linkartname);
			linkartname = savestr(tmpbuf);
		    }
		}
	    }
	}
#endif
	openart = artnum;		/* remember what we did here */
	seekart(pos);
    }
    return artfp;			/* and return either fp or NULL */
}

void
artclose()
{
    if (artfp != NULL) {		/* article still open? */
#ifdef SUPPORT_NNTP
	if (datasrc->flags & DF_REMOTE)
	    nntp_finishbody(FB_DISCARD);
#endif
	fclose(artfp);			/* close it */
	artfp = NULL;			/* and tell the world */
	openart = 0;
	clear_artbuf();
    }
}

int
seekart(pos)
ART_POS pos;
{
#ifdef SUPPORT_NNTP
    if (datasrc->flags & DF_REMOTE)
	return nntp_seekart(pos);
#endif
    return fseek(artfp,(long)pos,0);
}

ART_POS
tellart()
{
#ifdef SUPPORT_NNTP
    if (datasrc->flags & DF_REMOTE)
	return nntp_tellart();
#endif
    return (ART_POS)ftell(artfp);
}

char*
readart(s, limit)
char* s;
int limit;
{
#ifdef SUPPORT_NNTP
    if (datasrc->flags & DF_REMOTE)
	return nntp_readart(s,limit);
#endif
    return fgets(s,limit,artfp);
}

void
clear_artbuf()
{
    *artbuf = '\0';
    artbuf_pos = artbuf_seek = artbuf_len = 0;
}

int
seekartbuf(pos)
ART_POS pos;
{
    if (!do_hiding)
	return seekart(pos);

    pos -= htype[PAST_HEADER].minpos;
    artbuf_pos = artbuf_len;

    while (artbuf_pos < pos) {
	if (!readartbuf(FALSE))
	    return -1;
    }

    artbuf_pos = pos;

    return 0;
}

char*
readartbuf(view_inline)
bool_int view_inline;
{
    char* bp;
    char* s;
    int read_offset, line_offset, filter_offset, extra_offset, len, o;
    int word_wrap, extra_chars = 0;
    int read_something = 0;

    if (!do_hiding) {
	bp = readart(art_line,(sizeof art_line)-1);
	artbuf_pos = artbuf_seek = tellart() - htype[PAST_HEADER].minpos;
	return bp;
    }
    if (artbuf_pos == artsize - htype[PAST_HEADER].minpos)
	return NULL;
    bp = artbuf + artbuf_pos;
    if (*bp == '\001' || *bp == '\002') {
	bp++;
	artbuf_pos++;
    }
    if (*bp) {
	for (s = bp; *s && !AT_NL(*s); s++) ;
	if (*s) {
	    len = s - bp + 1;
	    goto done;
	}
	read_offset = line_offset = filter_offset = s - bp;
    }
    else
	read_offset = line_offset = filter_offset = 0;

  read_more:
    extra_offset = mime_state == HTMLTEXT_MIME? 1024 : 0;
    o = read_offset + extra_offset;
    if (artbuf_size < artbuf_pos + o + LBUFLEN) {
	artbuf_size += LBUFLEN * 4;
	artbuf = saferealloc(artbuf,artbuf_size);
	bp = artbuf + artbuf_pos;
    }
    switch (mime_state) {
      case IMAGE_MIME:
      case AUDIO_MIME:
	break;
      default:
	read_something = 1;
	/* The -1 leaves room for appending a newline, if needed */
	if (!readart(bp+o, artbuf_size-artbuf_pos-o-1)) {
	    if (!read_offset) {
		*bp = '\0';
		len = 0;
		bp = NULL;
		goto done;
	    }
	    strcpy(bp+o, "\n");
	    read_something = -1;
	}
	len = strlen(bp+o) + read_offset;
	if (bp[len+extra_offset-1] != '\n') {
	    if (read_something >= 0) {
		read_offset = len;
		goto read_more;
	    }
	    strcpy(bp + len++ + extra_offset, "\n");
	}
	if (!is_mime)
	    goto done;
	o = line_offset + extra_offset;
	mime_SetState(bp+o);
	if (bp[o] == '\0') {
	    strcpy(bp+o, "\n");
	    len = line_offset+1;
	}
	break;
    }
  mime_switch:
    switch (mime_state) {
      case ISOTEXT_MIME:
#if 0 /*def CHARSUBST*/
	charsubst = "a"; /*$$*/
#endif
	mime_state = TEXT_MIME;
	/* FALL THROUGH */
      case TEXT_MIME:
      case HTMLTEXT_MIME:
	if (mime_section->encoding == MENCODE_QPRINT) {
	    o = line_offset + extra_offset;
	    len = qp_decodestring(bp+o, bp+o, 0) + line_offset;
	    if (len == line_offset || bp[len+extra_offset-1] != '\n') {
		if (read_something >= 0) {
		    read_offset = line_offset = len;
		    goto read_more;
		}
		strcpy(bp + len++ + extra_offset, "\n");
	    }
	}
	else if (mime_section->encoding == MENCODE_BASE64) {
	    o = line_offset + extra_offset;
	    len = b64_decodestring(bp+o, bp+o) + line_offset;
	    if ((s = index(bp+o, '\n')) == NULL) {
		if (read_something >= 0) {
		    read_offset = line_offset = len;
		    goto read_more;
		}
		strcpy(bp + len++ + extra_offset, "\n");
	    }
	    else {
		extra_chars += len;
		len = s - bp - extra_offset + 1;
		extra_chars -= len;
	    }
	}
	if (mime_state != HTMLTEXT_MIME)
	    break;
	o = filter_offset + extra_offset;
	len = filter_html(bp+filter_offset, bp+o) + filter_offset;
	if (len == filter_offset || (s = index(bp,'\n')) == NULL) {
	    if (read_something >= 0) {
		read_offset = line_offset = filter_offset = len;
		goto read_more;
	    }
	    strcpy(bp + len++, "\n");
	    extra_chars = 0;
	}
	else {
	    extra_chars = len;
	    len = s - bp + 1;
	    extra_chars -= len;
	}
	break;
      case DECODE_MIME: {
	MIMECAP_ENTRY* mcp;
	mcp = mime_FindMimecapEntry(mime_section->type_name,
				    MCF_NEEDSTERMINAL|MCF_COPIOUSOUTPUT);
	if (mcp) {
	    int save_term_line = term_line;
	    nowait_fork = TRUE;
	    color_object(COLOR_MIMEDESC, 1);
	    if (decode_piece(mcp,bp) != 0) {
		strcpy(bp = artbuf + artbuf_pos, art_line);
		mime_SetState(bp);
		if (mime_state == DECODE_MIME)
		    mime_state = SKIP_MIME;
	    }
	    else
		mime_state = SKIP_MIME;
	    color_pop();
	    chdir_newsdir();
	    erase_line(FALSE);
	    nowait_fork = FALSE;
	    first_view = artline;
	    term_line = save_term_line;
	    if (mime_state != SKIP_MIME)
		goto mime_switch;
	}
	/* FALL THROUGH */
      }
      case SKIP_MIME: {
	MIME_SECT* mp = mime_section;
	while ((mp = mp->prev) != NULL && !mp->boundary_len) ;
	if (!mp) {
	    artbuf_len = artbuf_pos;
	    artsize = artbuf_len + htype[PAST_HEADER].minpos;
	    read_something = 0;
	    bp = NULL;
	}
	else if (read_something >= 0) {
	    *bp = '\0';
	    read_offset = line_offset = filter_offset = 0;
	    goto read_more;
	}
	else
	    *bp = '\0';
	len = 0;
	break;
      }
    case END_OF_MIME:
	if (mime_section->prev)
	    mime_state = SKIP_MIME;
	else {
#ifdef SUPPORT_NNTP
	    if (datasrc->flags & DF_REMOTE) {
		nntp_finishbody(FB_SILENT);
		raw_artsize = nntp_artsize();
	    }
#endif
	    seekart(raw_artsize);
	}
	/* FALL THROUGH */
      case BETWEEN_MIME:
	len = strlen(multipart_separator) + 1;
	if (extra_offset && filter_offset) {
	    extra_chars = len + 1;
	    len = o = read_offset + 1;
	    bp[o-1] = '\n';
	}
	else {
	    o = -1;
	    artbuf_pos++;
	    bp++;
	}
	sprintf(bp+o,"\002%s\n",multipart_separator);
	break;
      case UNHANDLED_MIME:
	mime_state = SKIP_MIME;
	*bp++ = '\001';
	artbuf_pos++;
	mime_Description(mime_section,bp,tc_COLS);
	len = strlen(bp);
	break;
      case ALTERNATE_MIME:
	mime_state = SKIP_MIME;
	*bp++ = '\001';
	artbuf_pos++;
	sprintf(bp,"[Alternative: %s]\n", mime_section->type_name);
	len = strlen(bp);
	break;
      case IMAGE_MIME:
      case AUDIO_MIME:
	if (!mime_article.total && !multimedia_mime)
	    multimedia_mime = TRUE;
	/* FALL THROUGH */
      default:
	if (view_inline && first_view < artline
	 && (mime_section->flags & MSF_INLINE))
	    mime_state = DECODE_MIME;
	else
	    mime_state = SKIP_MIME;
	*bp++ = '\001';
	artbuf_pos++;
	mime_Description(mime_section,bp,tc_COLS);
	len = strlen(bp);
	break;
    }

  done:
    word_wrap = tc_COLS - word_wrap_offset;
    if (read_something && word_wrap_offset >= 0 && word_wrap > 20 && bp) {
	char* cp;
	for (cp = bp; *cp && (s = index(cp, '\n')) != NULL; cp = s+1) {
	    if (s - cp > tc_COLS) {
		char* t;
		do {
		    for (t = cp+word_wrap; *t!=' ' && *t!='\t' && t > cp; t--) ;
		    if (t == cp) {
			for (t = cp+word_wrap; *t!=' ' && *t!='\t' && t<=cp+tc_COLS; t++) ;
			if (t > cp+tc_COLS) {
			    t = cp + tc_COLS - 1;
			    continue;
			}
		    }
		    if (cp == bp) {
			extra_chars += len;
			len = t - bp + 1;
			extra_chars -= len;
		    }
		    *t = wrapped_nl;
		    if (t[1] == ' ' || t[1] == '\t') {
			int spaces = 1;
			for (t++; *++t == ' ' || *t == '\t'; spaces++) ;
			safecpy(t-spaces,t,extra_chars);
			extra_chars -= spaces;
			t -= spaces + 1;
		    }
		} while (s - (cp = t+1) > word_wrap);
	    }
	}
    }
    artbuf_pos += len;
    if (read_something) {
    	artbuf_seek = tellart();
	artbuf_len = artbuf_pos + extra_chars;
	if (artsize >= 0)
	    artsize = raw_artsize-artbuf_seek+artbuf_len+htype[PAST_HEADER].minpos;
    }

    return bp;
}
