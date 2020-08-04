/* mime.c
 * vi: set sw=4 ts=8 ai sm noet:
 */

#include "EXTERN.h"
#include "common.h"
#include "list.h"
#include "hash.h"
#include "cache.h"
#include "head.h"
#include "search.h"
#include "art.h"
#include "artio.h"
#include "artstate.h"
#include "ng.h"
#include "term.h"
#include "decode.h"
#include "respond.h"
#include "env.h"
#include "color.h"
#include "util.h"
#include "util2.h"
#include "utf.h"
#include "backpage.h"
#include "charsubst.h"
#include "INTERN.h"
#include "mime.h"
#include "mime.ih"

static char text_plain[] = "text/plain";

#ifdef USE_UTF_HACK
#define CODE_POINT_MAX	0x7FFFFFFFL
#else
#define CODE_POINT_MAX	0x7F
#endif

void
mime_init()
{
    char* s;
    char* t;
    char* mcname;

    mimecap_list = new_list(0,-1,sizeof(MIMECAP_ENTRY),40,LF_ZERO_MEM,NULL);

    if ((mcname = getenv("MIMECAPS")) == NULL)
	mcname = getval("MAILCAPS", MIMECAP);
    mcname = s = savestr(mcname);
    do {
	if ((t = index(s, ':')) != NULL)
	    *t++ = '\0';
	if (*s)
	    mime_ReadMimecap(s);
	s = t;
    } while (s && *s);
    free(mcname);
}

void
mime_ReadMimecap(mcname)
char* mcname;
{
    FILE* fp;
    char* bp;
    char* s;
    char* t;
    char* arg;
    int buflen = 2048;
    int linelen;
    MIMECAP_ENTRY* mcp;
    int i;

    if ((fp = fopen(filexp(mcname), "r")) == NULL)
	return;
    bp = safemalloc(buflen);
    for (i = mimecap_list->high; !feof(fp); ) {
	*(s = bp) = '\0';
	linelen = 0;
	while (fgets(s, buflen - linelen, fp)) {
	    if (*s == '#')
		continue;
	    linelen += strlen(s);
	    if (linelen == 0)
		continue;
	    if (bp[linelen-1] == '\n') {
		if (--linelen == 0)
		    continue;
		if (bp[linelen-1] != '\\') {
		    bp[linelen] = '\0';
		    break;
		}
		bp[--linelen] = '\0';
	    }

	    if (linelen+1024 > buflen) {
		buflen *= 2;
		bp = saferealloc(bp, buflen);
	    }

	    s = bp + linelen;
	}
	for (s = bp; isspace(*s); s++) ;
	if (!*s)
	    continue;
	t = mime_ParseEntryArg(&s);
	if (!s) {
	    fprintf(stderr, "trn: Ignoring invalid mimecap entry: %s\n", bp);
	    continue;
	}
	mcp = mimecap_ptr(++i);
	mcp->contenttype = savestr(t);
	mcp->command = savestr(mime_ParseEntryArg(&s));
	while (s) {
	    t = mime_ParseEntryArg(&s);
	    if ((arg = index(t, '=')) != NULL) {
		char* f = arg+1;
		while (arg != t && isspace(arg[-1])) arg--;
		*arg++ = '\0';
		while (isspace(*f)) f++;
		if (*f == '"')
		    f = cpytill(arg,f+1,'"');
		else
		    arg = f;
	    }
	    if (*t) {
		if (strcaseEQ(t, "needsterminal"))
		    mcp->flags |= MCF_NEEDSTERMINAL;
		else if (strcaseEQ(t, "copiousoutput"))
		    mcp->flags |= MCF_COPIOUSOUTPUT;
		else if (arg && strcaseEQ(t, "test"))
		    mcp->testcommand = savestr(arg);
		else if (arg && strcaseEQ(t, "description"))
		    mcp->label = savestr(arg);
		else if (arg && strcaseEQ(t, "label")) 
		    mcp->label = savestr(arg); /* bogus old name for description */
	    }
	}
    }
    mimecap_list->high = i;
    free(bp);
    fclose(fp);
}

static char*
mime_ParseEntryArg(cpp)
char** cpp;
{
    char* s = *cpp;
    char* f;
    char* t;

    while (isspace(*s)) s++;

    for (f = t = s; *f && *f != ';'; ) {
	if (*f == '\\') {
	    if (*++f == '%')
		*t++ = '%';
	    else if (!*f)
		break;
	}
	*t++ = *f++;
    }
    while (isspace(*f) || *f == ';') f++;
    if (!*f)
	f = NULL;
    while (t != s && isspace(t[-1])) t--;
    *t = '\0';
    *cpp = f;
    return s;
}

MIMECAP_ENTRY*
mime_FindMimecapEntry(contenttype, skip_flags)
char* contenttype;
int skip_flags;
{
    MIMECAP_ENTRY* mcp;
    int i;

    for (i = 0; i <= mimecap_list->high; i++) {
	mcp = mimecap_ptr(i);
	if (!(mcp->flags & skip_flags)
	 && mime_TypesMatch(contenttype, mcp->contenttype)) {
	    if (!mcp->testcommand)
		return mcp;
	    if (mime_Exec(mcp->testcommand) == 0)
		return mcp;
	}
    }
    return NULL;
}

bool
mime_TypesMatch(ct,pat)
char* ct;
char* pat;
{
    char* s = index(pat,'/');
    int len = (s? s - pat : strlen(pat));
    bool iswild = (!s || strEQ(s+1,"*"));

    return strcaseEQ(ct,pat)
	|| (iswild && strncaseEQ(ct,pat,len) && ct[len] == '/');
}

int
mime_Exec(cmd)
char* cmd;
{
    char* f;
    char* t;

    for (f = cmd, t = cmd_buf; *f && t-cmd_buf < CBUFLEN-2; f++) {
	if (*f == '%') {
	    switch (*++f) {
	      case 's':
		safecpy(t, decode_filename, CBUFLEN-(t-cmd_buf));
		t += strlen(t);
		break;
	      case 't':
		*t++ = '\'';
		safecpy(t, mime_section->type_name, CBUFLEN-(t-cmd_buf)-1);
		t += strlen(t);
		*t++ = '\'';
		break;
	      case '{': {
		char* s = index(f, '}');
		char* p;
                if (!s)
		    return -1;
		f++;
		*s = '\0';
		p = mime_FindParam(mime_section->type_params, f);
		*s = '}'; /* restore */
		f = s;
		*t++ = '\'';
		safecpy(t, p, CBUFLEN-(t-cmd_buf)-1);
		t += strlen(t);
		*t++ = '\'';
		break;
	      }
	      case '%':
		*t++ = '%';
		break;
	      case 'n':
	      case 'F':
		return -1;
	    }
        }
	else
            *t++ = *f;
    }
    *t = '\0';

    return doshell(sh, cmd_buf);
}

void
mime_InitSections()
{
    while (mime_PopSection()) ;
    mime_ClearStruct(mime_section);
    mime_state = NOT_MIME;
}

void
mime_PushSection()
{
    MIME_SECT* mp = (MIME_SECT*)safemalloc(sizeof (MIME_SECT));
    bzero((char*)mp, sizeof (MIME_SECT));
    mp->prev = mime_section;
    mime_section = mp;
}

bool
mime_PopSection()
{
    MIME_SECT* mp = mime_section->prev;
    if (mp) {
	mime_ClearStruct(mime_section);
	free((char*)mime_section);
	mime_section = mp;
	mime_state = mp->type;
	return TRUE;
    }
    mime_state = mime_article.type;
    return FALSE;
}

/* Free up this mime structure's resources */
void
mime_ClearStruct(mp)
MIME_SECT* mp;
{
    safefree0(mp->filename);
    safefree0(mp->type_name);
    safefree0(mp->type_params);
    safefree0(mp->boundary);
    safefree0(mp->html_blks);
    mp->type = NOT_MIME;
    mp->encoding = MENCODE_NONE;
    mp->part = mp->total = mp->boundary_len = mp->flags = mp->html
	     = mp->html_blkcnt = 0;
    mp->html_line_start = 0;
}

/* Setup mime_article structure based on article's headers */
void
mime_SetArticle()
{
    char* s;

    mime_InitSections();
    /*$$ Check mime version #? */
    multimedia_mime = FALSE;
    is_mime = (htype[MIMEVER_LINE].flags & HT_MAGIC)
	    && htype[MIMEVER_LINE].minpos >= 0;

    s = fetchlines(art,CONTTYPE_LINE);
    mime_ParseType(mime_section,s);
    free(s);

    if (is_mime) {
	s = fetchlines(art,CONTXFER_LINE);
	mime_ParseEncoding(mime_section,s);
	free(s);

	s = fetchlines(art,CONTDISP_LINE);
	mime_ParseDisposition(mime_section,s);
	free(s);

	mime_state = mime_section->type;
	if (mime_state == NOT_MIME
	 || (mime_state == TEXT_MIME && mime_section->encoding == MENCODE_NONE))
	    is_mime = FALSE;
	else if (!mime_section->type_name)
	    mime_section->type_name = savestr(text_plain);
    }
}

/* Use the Content-Type to set values in the mime structure */
void
mime_ParseType(mp, s)
MIME_SECT* mp;
char* s;
{
    char* t;

    safefree0(mp->type_name);
    safefree0(mp->type_params);

    mp->type_params = mime_ParseParams(s);
    if (!*s) {
	mp->type = NOT_MIME;
	return;
    }
    mp->type_name = savestr(s);
    t = mime_FindParam(mp->type_params,"name");
    if (t) {
	safefree(mp->filename);
	mp->filename = savestr(t);
    }

    if (strncaseEQ(s, "text", 4)) {
	mp->type = TEXT_MIME;
	s += 4;
	if (*s++ != '/')
	    return;
	utf_init(mime_FindParam(mp->type_params,"charset"), "utf-8"); /*FIXME*/
	if (strncaseEQ(s, "html", 4))
	    mp->type = HTMLTEXT_MIME;
	else if (strncaseEQ(s, "x-vcard", 7))
	    mp->type = UNHANDLED_MIME;
	return;
    }

    if (strncaseEQ(s, "message/", 8)) {
	s += 8;
	mp->type = MESSAGE_MIME;
	if (strcaseEQ(s, "partial")) {
	    t = mime_FindParam(mp->type_params,"id");
	    if (!t)
		return;
	    safefree(mp->filename);
	    mp->filename = savestr(t);
	    t = mime_FindParam(mp->type_params,"number");
	    if (t)
		mp->part = (short)atoi(t);
	    t = mime_FindParam(mp->type_params,"total");
	    if (t)
		mp->total = (short)atoi(t);
	    if (!mp->total) {
		mp->part = 0;
		return;
	    }
	    return;
	}
	return;
    }

    if (strncaseEQ(s, "multipart/", 10)) {
	s += 10;
	t = mime_FindParam(mp->type_params,"boundary");
	if (!t) {
	    mp->type = UNHANDLED_MIME;
	    return;
	}
	if (strncaseEQ(s, "alternative", 11))
	    mp->flags |= MSF_ALTERNATIVE;
	safefree(mp->boundary);
	mp->boundary = savestr(t);
	mp->boundary_len = (short)strlen(t);
	mp->type = MULTIPART_MIME;
	return;
    }

    if (strncaseEQ(s, "image/", 6)) {
	mp->type = IMAGE_MIME;
	return;
    }

    if (strncaseEQ(s, "audio/", 6)) {
	mp->type = AUDIO_MIME;
	return;
    }

    mp->type = UNHANDLED_MIME;
}

/* Use the Content-Disposition to set values in the mime structure */
void
mime_ParseDisposition(mp, s)
MIME_SECT* mp;
char* s;
{
    char* params;

    params = mime_ParseParams(s);
    if (strcaseEQ(s,"inline"))
	mp->flags |= MSF_INLINE;

    s = mime_FindParam(params,"filename");
    if (s) {
	safefree(mp->filename);
	mp->filename = savestr(s);
    }
    safefree(params);
}

/* Use the Content-Transfer-Encoding to set values in the mime structure */
void
mime_ParseEncoding(mp, s)
MIME_SECT* mp;
char* s;
{
    s = mime_SkipWhitespace(s);
    if (!*s) {
	mp->encoding = MENCODE_NONE;
	return;
    }
    if (*s == '7' || *s == '8') {
	if (strncaseEQ(s+1, "bit", 3)) {
	    s += 4;
	    mp->encoding = MENCODE_NONE;
	}
    }
    else if (strncaseEQ(s, "quoted-printable", 16)) {
	s += 16;
	mp->encoding = MENCODE_QPRINT;
    }
    else if (strncaseEQ(s, "binary", 6)) {
	s += 6;
	mp->encoding = MENCODE_NONE;
    }
    else if (strncaseEQ(s, "base64", 6)) {
	s += 6;
	mp->encoding = MENCODE_BASE64;
    }
    else if (strncaseEQ(s, "x-uue", 5)) {
	s += 5;
	mp->encoding = MENCODE_UUE;
	if (strncaseEQ(s, "ncode", 5))
	    s += 5;
    }
    else {
	mp->encoding = MENCODE_UNHANDLED;
	return;
    }
    if (*s != '\0' && !isspace(*s) && *s != ';' && *s != '(')
	mp->encoding = MENCODE_UNHANDLED;
}

/* Parse a multipart mime header and affect the *mime_section structure */

void
mime_ParseSubheader(ifp, next_line)
FILE* ifp;
char* next_line;
{
    static char* line = NULL;
    static int line_size = 0;
    char* s;
    int pos, linetype, len;
    mime_ClearStruct(mime_section);
    mime_section->type = TEXT_MIME;
    for (;;) {
	for (pos = 0; ; pos += strlen(line+pos)) {
	    len = pos + (next_line? strlen(next_line) : 0) + LBUFLEN;
	    if (line_size < len) {
		line_size = len + LBUFLEN;
		line = saferealloc(line, line_size);
	    }
	    if (next_line) {
		safecpy(line+pos, next_line, line_size - pos);
		next_line = NULL;
	    }
	    else if (ifp) {
		if (!fgets(line + pos, LBUFLEN, ifp))
		    break;
	    }
	    else if (!readart(line + pos, LBUFLEN))
		break;
	    if (line[0] == '\n')
		break;
	    if (pos && line[pos] != ' ' && line[pos] != '\t') {
		next_line = line + pos;
		line[pos-1] = '\0';
		break;
	    }
	}
	s = index(line,':');
	if (s == NULL)
	    break;

	linetype = set_line_type(line,s);
	switch (linetype) {
	  case CONTTYPE_LINE:
	    mime_ParseType(mime_section,s+1);
	    break;
	  case CONTXFER_LINE:
	    mime_ParseEncoding(mime_section,s+1);
	    break;
	  case CONTDISP_LINE:
	    mime_ParseDisposition(mime_section,s+1);
	    break;
	  case CONTNAME_LINE:
	    safefree(mime_section->filename);
	    s = mime_SkipWhitespace(s+1);
	    mime_section->filename = savestr(s);
	    break;
#if 0
	  case CONTLEN_LINE:
	    mime_section->content_len = atol(s+1);
	    break;
#endif
	}
    }
    mime_state = mime_section->type;
    if (!mime_section->type_name)
	mime_section->type_name = savestr(text_plain);
}

void
mime_SetState(bp)
char* bp;
{
    int ret;

    if (mime_state == BETWEEN_MIME) {
	mime_ParseSubheader((FILE*)NULL,bp);
	*bp = '\0';
	if (mime_section->prev->flags & MSF_ALTERNADONE)
	    mime_state = ALTERNATE_MIME;
	else if (mime_section->prev->flags & MSF_ALTERNATIVE)
	    mime_section->prev->flags |= MSF_ALTERNADONE;
    }

    while (mime_state == MESSAGE_MIME) {
	mime_PushSection();
	mime_ParseSubheader((FILE*)NULL,*bp? bp : (char*)NULL);
	*bp = '\0';
    }

    if (mime_state == MULTIPART_MIME) {
	mime_PushSection();
	mime_state = SKIP_MIME;		/* Skip anything before 1st part */
    }

    ret = mime_EndOfSection(bp);
    switch (ret) {
      case 0:
	break;
      case 1:
	while (!mime_section->prev->boundary_len)
	    mime_PopSection();
	mime_state = BETWEEN_MIME;
	break;
      case 2:
	while (!mime_section->prev->boundary_len)
	    mime_PopSection();
	mime_PopSection();
	mime_state = END_OF_MIME;
	break;
    }
}

int
mime_EndOfSection(bp)
char* bp;
{
    MIME_SECT* mp = mime_section->prev;
    while (mp && !mp->boundary_len)
	mp = mp->prev;
    if (mp) {
	/* have we read all the data in this part? */
	if (bp[0] == '-' && bp[1] == '-'
	 && strnEQ(bp+2,mp->boundary,mp->boundary_len)) {
	    int len = 2 + mp->boundary_len;
	    /* have we found the last boundary? */
	    if (bp[len] == '-' && bp[len+1] == '-'
	     && (bp[len+2] == '\n' || bp[len+2] == '\0'))
		return 2;
	    return bp[len] == '\n' || bp[len] == '\0';
	}
    }
    return 0;
}

/* Return a saved string of all the extra parameters on this mime
 * header line.  The passed-in string is transformed into just the
 * first word on the line.
 */
char*
mime_ParseParams(str)
char* str;
{
    char* s;
    char* t;
    char* e;
    s = e = mime_SkipWhitespace(str);
    while (*e && *e != ';' && !isspace(*e) && *e != '(') e++;
    t = savestr(mime_SkipWhitespace(e));
    *e = '\0';
    if (s != str)
	safecpy(str, s, e - s + 1);
    str = s = t;
    while (*s == ';') {
	s = mime_SkipWhitespace(s+1);
	while (*s && *s != ';' && *s != '(' && *s != '=' && !isspace(*s))
	    *t++ = *s++;
	s = mime_SkipWhitespace(s);
	if (*s == '=') {
	    *t++ = *s;
	    s = mime_SkipWhitespace(s+1);
	    if (*s == '"') {
		s = cpytill(t,s+1,'"');
		if (*s == '"')
		    s++;
		t += strlen(t);
	    }
	    else
		while (*s && *s != ';' && !isspace(*s) && *s != '(')
		    *t++ = *s++;
	}
	*t++ = '\0';
    }
    *t = '\0';
    return str;
}

char*
mime_FindParam(s, param)
char* s;
char* param;
{
    int param_len = strlen(param);
    while (s && *s) {
	if (strncaseEQ(s, param, param_len) && s[param_len] == '=')
	    return s + param_len + 1;
	s += strlen(s) + 1;
    }
    return NULL;
}

/* Skip whitespace and RFC-822 comments. */

char*
mime_SkipWhitespace(s)
char* s;
{
    int comment_level = 0;

    while (*s) {
	if (*s == '(') {
	    s++;
	    comment_level++;
	    while (comment_level) {
		switch (*s++) {
		  case '\0':
		    return s-1;
		  case '\\':
		    s++;
		    break;
		  case '(':
		    comment_level++;
		    break;
		  case ')':
		    comment_level--;
		    break;
		}
	    }
	}
	else if (!isspace(*s))
	    break;
	else
	    s++;
    }
    return s;
}

void
mime_DecodeArticle(view)
bool_int view;
{
    MIMECAP_ENTRY* mcp = NULL;

    seekart(savefrom);
    *art_line = '\0';

    while (1) {
	if (mime_state != MESSAGE_MIME || !mime_section->total) {
	    if (!readart(art_line,sizeof art_line))
		break;
	    mime_SetState(art_line);
	}
	switch (mime_state) {
	  case BETWEEN_MIME:
	  case END_OF_MIME:
	    break;
	  case TEXT_MIME:
	  case HTMLTEXT_MIME:
	  case ISOTEXT_MIME:
	  case MESSAGE_MIME:
	    /* $$ Check for uuencoded file here? */
	    mime_state = SKIP_MIME;
	    /* FALL THROUGH */
	  case SKIP_MIME: {
	    MIME_SECT* mp = mime_section;
	    while ((mp = mp->prev) != NULL && !mp->boundary_len) ;
	    if (!mp)
		return;
	    break;
	  }
	  default:
	    if (view) {
		mcp = mime_FindMimecapEntry(mime_section->type_name,0);
		if (!mcp) {
		    printf("No view method for %s -- skipping.\n",
			   mime_section->type_name);
		    mime_state = SKIP_MIME;
		    break;
		}
	    }
	    mime_state = DECODE_MIME;
	    if (decode_piece(mcp, *art_line == '\n'? NULL : art_line) != 0) {
		mime_SetState(art_line);
		if (mime_state == DECODE_MIME)
		    mime_state = SKIP_MIME;
	    }
	    else {
		if (*msg) {
		    newline();
		    fputs(msg,stdout);
		}
		mime_state = SKIP_MIME;
	    }
	    newline();
	    break;
	}
    }
}

void
mime_Description(mp, s, limit)
MIME_SECT* mp;
char* s;
int limit;
{
    char* fn = decode_fix_fname(mp->filename);
    int len, flen = strlen(fn);

    limit -= 2;  /* leave room for the trailing ']' and '\n' */
    sprintf(s, "[Attachment type=%s, name=", mp->type_name);
    len = strlen(s);
    if (len + flen <= limit)
	sprintf(s+len, "%s]\n", fn);
    else if (len+3 >= limit)
	strcpy(s+limit-3, "...]\n");
    else {
#if 0
	sprintf(s+len, "...%s]\n", fn + flen - (limit-(len+3)));
#else
	safecpy(s+len, fn, limit - (len+3));
	strcat(s, "...]\n");
#endif
    }
}

#define XX 255
static Uchar index_hex[256] = {
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
     0, 1, 2, 3,  4, 5, 6, 7,  8, 9,XX,XX, XX,XX,XX,XX,
    XX,10,11,12, 13,14,15,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,10,11,12, 13,14,15,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
};

int
qp_decodestring(t, f, in_header)
char* t;
char* f;
bool_int in_header;
{
    char* save_t = t;
    while (*f) {
	switch (*f) {
	  case '_':
	    if (in_header) {
		*t++ = ' ';
		f++;
	    }
	    else
		*t++ = *f++;
	    break;
	  case '=':	/* decode a hex-value */ 
	    if (f[1] == '\n') {
		f += 2;
		break;
	    }
	    if (index_hex[(Uchar)f[1]] != XX && index_hex[(Uchar)f[2]] != XX) {
		*t = (index_hex[(Uchar)f[1]] << 4) + index_hex[(Uchar)f[2]];
		f += 3;
		if (*t != '\r')
		    t++;
		break;
	    }
	    /* FALL THROUGH */
	  default:
	    *t++ = *f++;
	    break;
	}
    }
    *t = '\0';
    return t - save_t;
}

int
qp_decode(ifp,state)
FILE* ifp;
int state;
{
    static FILE* ofp = NULL;
    int c1, c2;

    if (state == DECODE_DONE) {
	if (ofp)
	    fclose(ofp);
	ofp = NULL;
	return state;
    }

    if (state == DECODE_START) {
	char* filename = decode_fix_fname(mime_section->filename);
	ofp = fopen(filename, FOPEN_WB);
	if (!ofp)
	    return DECODE_ERROR;
	erase_line(0);
	printf("Decoding %s", filename);
	if (nowait_fork)
	    fflush(stdout);
	else
	    newline();
    }

    while ((c1 = mime_getc(ifp)) != EOF) {
      check_c1:
	if (c1 == '=') {
	    c1 = mime_getc(ifp);
	    if (c1 == '\n')
		continue;
	    if (index_hex[(Uchar)c1] == XX) {
		putc('=', ofp);
		goto check_c1;
	    }
	    c2 = mime_getc(ifp);
	    if (index_hex[(Uchar)c2] == XX) {
		putc('=', ofp);
		putc(c1, ofp);
		c1 = c2;
		goto check_c1;
	    }
	    c1 = (index_hex[(Uchar)c1] << 4) | index_hex[(Uchar)c2];
	    if (c1 != '\r')
		putc(c1, ofp);
	}
	else
	    putc(c1, ofp);
    }

    return DECODE_MAYBEDONE;
}

static Uchar index_b64[256] = {
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,62, XX,XX,XX,63,
    52,53,54,55, 56,57,58,59, 60,61,XX,XX, XX,XX,XX,XX,
    XX, 0, 1, 2,  3, 4, 5, 6,  7, 8, 9,10, 11,12,13,14,
    15,16,17,18, 19,20,21,22, 23,24,25,XX, XX,XX,XX,XX,
    XX,26,27,28, 29,30,31,32, 33,34,35,36, 37,38,39,40,
    41,42,43,44, 45,46,47,48, 49,50,51,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
    XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX, XX,XX,XX,XX,
};

int
b64_decodestring(t, f)
char* t;
char* f;
{
    char* save_t = t;
    Uchar ch1, ch2;

    while (*f && *f != '=') {
	ch1 = index_b64[(Uchar)*f++];
	if (ch1 == XX)
	    continue;
	do {
	    if (!*f || *f == '=')
		goto dbl_break;
	    ch2 = index_b64[(Uchar)*f++];
	} while (ch2 == XX);
	*t++ = (ch1 << 2) | (ch2 >> 4);
	do {
	    if (!*f || *f == '=')
		goto dbl_break;
	    ch1 = index_b64[(Uchar)*f++];
	} while (ch1 == XX);
	*t++ = ((ch2 & 0x0f) << 4) | (ch1 >> 2);
	do {
	    if (!*f || *f == '=')
		goto dbl_break;
	    ch2 = index_b64[(Uchar)*f++];
	} while (ch2 == XX);
	*t++ = ((ch1 & 0x03) << 6) | ch2;
    }
  dbl_break:
    *t = '\0';
    return t - save_t;
}

int
b64_decode(ifp, state)
FILE* ifp;
int state;
{
    static FILE* ofp = NULL;
    int c1, c2, c3, c4;

    if (state == DECODE_DONE) {
      all_done:
	if (ofp)
	    fclose(ofp);
	ofp = NULL;
	return state;
    }

    if (state == DECODE_START) {
	char* filename = decode_fix_fname(mime_section->filename);
	ofp = fopen(filename, FOPEN_WB);
	if (!ofp)
	    return DECODE_ERROR;
	printf("Decoding %s", filename);
	if (nowait_fork)
	    fflush(stdout);
	else
	    newline();
	state = DECODE_ACTIVE;
    }

    while ((c1 = mime_getc(ifp)) != EOF) {
	if (c1 != '=' && index_b64[c1] == XX)
	    continue;
	do {
	    c2 = mime_getc(ifp);
	    if (c2 == EOF)
		return state;
	} while (c2 != '=' && index_b64[c2] == XX);
	do {
	    c3 = mime_getc(ifp);
	    if (c3 == EOF)
		return state;
	} while (c3 != '=' && index_b64[c3] == XX);
	do {
	    c4 = mime_getc(ifp);
	    if (c4 == EOF)
		return state;
	} while (c4 != '=' && index_b64[c4] == XX);
	if (c1 == '=' || c2 == '=') {
	    state = DECODE_DONE;
	    break;
	}
	c1 = index_b64[c1];
	c2 = index_b64[c2];
	c1 = (c1 << 2) | (c2 >> 4);
	putc(c1, ofp);
	if (c3 == '=') {
	    state = DECODE_DONE;
	    break;
	}
	c3 = index_b64[c3];
	c2 = ((c2 & 0x0f) << 4) | (c3 >> 2);
	putc(c2, ofp);
	if (c4 == '=') {
	    state = DECODE_DONE;
	    break;
	}
	c4 = index_b64[c4];
	c3 = ((c3 & 0x03) << 6) | c4;
	putc(c3, ofp);
    }

    if (state == DECODE_DONE)
	goto all_done;

    return DECODE_MAYBEDONE;
}

static int
mime_getc(fp)
FILE* fp;
{
    if (fp)
	return fgetc(fp);

    if (!mime_getc_line || !*mime_getc_line) {
	mime_getc_line = readart(art_line,sizeof art_line);
	if (mime_EndOfSection(art_line))
	    return EOF;
	if (!mime_getc_line)
	    return EOF;
    }
    return *mime_getc_line++;
}

int
cat_decode(ifp, state)
FILE* ifp;
int state;
{
    static FILE* ofp = NULL;

    if (state == DECODE_DONE) {
	if (ofp)
	    fclose(ofp);
	ofp = NULL;
	return state;
    }

    if (state == DECODE_START) {
	char* filename = decode_fix_fname(mime_section->filename);
	ofp = fopen(filename, FOPEN_WB);
	if (!ofp)
	    return DECODE_ERROR;
	printf("Decoding %s", filename);
	if (nowait_fork)
	    fflush(stdout);
	else
	    newline();
    }

    if (ifp) {
	while (fgets(buf, sizeof buf, ifp))
	    fputs(buf, ofp);
    }
    else {
	while (readart(buf, sizeof buf)) {
	    if (mime_EndOfSection(buf))
		break;
	    fputs(buf, ofp);
	}
    }

    return DECODE_MAYBEDONE;
}

static int word_wrap_in_pre, normal_word_wrap, word_wrap;

static const char* named_entities[] = {
    "lt",	"<",
    "gt",	">",
    "amp",	"&",
    "quot",	"\"",
    "apo",	"'",	/* non-standard but seen in the wild */
#ifndef USE_UTF_HACK
    "nbsp",	" ",
    "ensp",	" ",	/* seen in the wild */
    "lsquo",	"'",
    "rsquo",	"'",
    "ldquo",	"\"",
    "rdquo",	"\"",
    "ndash",	"-",
    "mdash",	"-",
    "copy",	"(C)",
    "trade",	"(TM)",
    "zwsp",	"",
    "zwnj",	"",
    "ccedil",	"c",	/* per charsubst.c */
    "eacute",	"e",
#else /* USE_UTF_HACK */
    "nbsp",	" ",
    "ensp",	" ",	/* U+2002 */
    "lsquo",	"‘",
    "rsquo",	"’",
    "ldquo",	"“",
    "rdquo",	"”",
    "ndash",	"–",
    "mdash",	"—",
    "copy",	"©",
    "trade",	"™",
    "zwsp",	"​",
    "zwnj",	"‌",
    "ccedil",	"ç",
    "eacute",	"é",
#endif
    NULL,	NULL,
};

int
filter_html(t, f)
char* t;
char* f;
{
    static char tagword[32];
    static int tagword_len;
    char* bp;
    char* cp;

    if (word_wrap_offset < 0) {
	normal_word_wrap = tc_COLS - 8;
	word_wrap_in_pre = 0;
    }
    else
	word_wrap_in_pre = normal_word_wrap = tc_COLS - word_wrap_offset;

    if (normal_word_wrap <= 20)
	normal_word_wrap = 0;
    if (word_wrap_in_pre <= 20)
	word_wrap_in_pre = 0;
    word_wrap = (mime_section->html & HF_IN_PRE)? word_wrap_in_pre
						: normal_word_wrap;
    if (!mime_section->html_line_start)
	mime_section->html_line_start = t - artbuf;

    if (!mime_section->html_blks) {
	mime_section->html_blks = (HBLK*)safemalloc(HTML_MAX_BLOCKS
						  * sizeof (HBLK));
    }

    for (bp = t; *f; f++) {
	if (mime_section->html & HF_IN_DQUOTE) {
	    if (*f == '"')
		mime_section->html &= ~HF_IN_DQUOTE;
	    else if (tagword_len < (sizeof tagword) - 1)
		tagword[tagword_len++] = *f;
	}
	else if (mime_section->html & HF_IN_SQUOTE) {
	    if (*f == '\'')
		mime_section->html &= ~HF_IN_SQUOTE;
	    else if (tagword_len < (sizeof tagword) - 1)
		tagword[tagword_len++] = *f;
	}
	else if (mime_section->html & HF_IN_TAG) {
	    if (*f == '>') {
		mime_section->html &= ~(HF_IN_TAG | HF_IN_COMMENT);
		tagword[tagword_len] = '\0';
		if (*tagword == '/')
		    t = tag_action(t, tagword+1, CLOSING_TAG);
		else
		    t = tag_action(t, tagword, OPENING_TAG);
	    }
	    else if (*f == '-' && f[1] == '-') {
		f++;
		mime_section->html |= HF_IN_COMMENT;
	    }
	    else if (*f == '"')
		mime_section->html |= HF_IN_DQUOTE;
	    else if (*f == '\'')
		mime_section->html |= HF_IN_SQUOTE;
	    else if (tagword_len < (sizeof tagword) - 1) {
		tagword[tagword_len++] = AT_GREY_SPACE(f)? ' ' : *f;
	    }
	}
	else if (mime_section->html & HF_IN_COMMENT) {
	    if (*f == '-' && f[1] == '-') {
		f++;
		mime_section->html &= ~HF_IN_COMMENT;
	    }
	}
	else if (*f == '<') {
	    tagword_len = 0;
	    mime_section->html |= HF_IN_TAG;
	}
	else if (mime_section->html & HF_IN_HIDING)
	    ;
	else if (*f == '&' && f[1] == '#') {
	    long int ncr = 0;
	    int ncr_found = 0;
	    int is_hex = f[2] == 'x';
	    int base = is_hex? 16: 10;
	    int i;
	    for (i = 0; ; i++) {
		int c = f[2 + is_hex + i];
		int v = index_hex[c];
	    if (c == '\0' || v == XX || v > base) break;
		ncr *= base;
		ncr += v;
	    }
	    if (i) {
		char det = f[2 + is_hex + i];
		if (det == ';')
		    ncr_found = 2 + is_hex + i;
		else if (!(det == '-' || isalnum(det))) /* see html-spec.txt 3.2.1 */
		    ncr_found = 1 + is_hex + i;
	    }
	    if (ncr_found && ncr <= CODE_POINT_MAX) {
		if (ncr)
		    t += insert_unicode_at(t, ncr);
		f += ncr_found;
	    } else
		*t++ = *f;
	}
	else if (*f == '&' && isalpha(f[1])) { /* see html-spec.txt 3.2.1 */
	    int i;
	    int entity_found = 0;
	    t = output_prep(t);
	    for (i = 0; named_entities[i] != NULL; i += 2) {
		int n = strlen(named_entities[i]);
		if (strncaseEQ(f+1, named_entities[i], n)) {
		    char det = f[n+1];
		    if (det == ';')
			entity_found = n + 1;
		    else if (!(det == '-' || isalnum(det))) /* see html-spec.txt 3.2.1 */
			entity_found = n;
		}
	    if (entity_found) break;
	    }
	    if (entity_found) {
		int j;
		for (j = 0; ; j++) {
		    char c = named_entities[i + 1][j];
		if (c == '\0') break;
		    *t++ = c;
		}
		f += entity_found;
	    } else
		*t++ = *f;
	    mime_section->html |= HF_NL_OK|HF_P_OK|HF_SPACE_OK;
	}
	else if ((*f == ' ' || AT_GREY_SPACE(f)) && !(mime_section->html & HF_IN_PRE)) {
	    /* We don't want to call output_prep() here. */
	    if (*f == ' ' || (mime_section->html & HF_SPACE_OK)) {
		mime_section->html &= ~HF_SPACE_OK;
		*t++ = ' ';
	    }
	    /* In non-PRE mode spaces should be collapsed */
	    for (;;) {
		int w = byte_length_at(f);
	    if (w == 0 || f[w] == '\0' || !(f[w] == ' ' || AT_GREY_SPACE(f+w))) break;
		f += w;
	    }
	}
	else if (*f == '\n') { /* Handle the HF_IN_PRE case */
	    t = output_prep(t);
	    mime_section->html |= HF_NL_OK;
	    t = do_newline(t, HF_NL_OK);
	}
	else {
	    int w = byte_length_at(f);
	    int i;
	    t = output_prep(t);
	    for (i = 0; i < w; i++)
		*t++ = *f++;
	    f--;
	    mime_section->html |= HF_NL_OK|HF_P_OK|HF_SPACE_OK;
	}

	if (word_wrap && t - artbuf - mime_section->html_line_start > tc_COLS) {
	    char* line_start = mime_section->html_line_start + artbuf;
	    for (cp = line_start + word_wrap;
		 cp > line_start && *cp != ' ' && *cp != '\t';
		 cp--) ;
	    if (cp == line_start) {
		for (cp = line_start + word_wrap;
		     cp - line_start <= tc_COLS && *cp != ' ' && *cp != '\t';
		     cp++) ;
		if (cp - line_start > tc_COLS) {
		    mime_section->html_line_start += tc_COLS;
		    cp = NULL;
		}
	    }
	    if (cp) {
		int flag_save = mime_section->html;
		int fudge;
		char* s;
		mime_section->html |= HF_NL_OK;
		cp = line_start = do_newline(cp, HF_NL_OK);
		fudge = do_indent((char*)NULL);
		while (*cp == ' ' || *cp == '\t') cp++;
		if ((fudge -= cp - line_start) != 0) {
		    if (fudge < 0) {
			if (t - cp > 0)
			    bcopy(cp, cp + fudge, t - cp);
		    }
		    else
			for (s = t; s-- != cp; ) s[fudge] = *s;
		    (void) do_indent(line_start);
		    t += fudge;
		}
		mime_section->html = flag_save;
	    }
	}
    }
    *t = '\0';

    return t - bp;
}

static char bullets[3] = {'*', 'o', '+'};
static char letters[2] = {'a', 'A'};
static char roman_letters[] = { 'M', 'D', 'C', 'L', 'X', 'V', 'I'};
static int  roman_values[]  = {1000, 500, 100,  50, 10,   5,   1 };

static char*
tag_action(t, word, opening_tag)
char* t;
char* word;
bool_int opening_tag;
{
    char* cp;
    int i, j, tnum, len, itype, ch, cnt, num;
    bool match = 0;
    HBLK* blks = mime_section->html_blks;

    for (cp = word; *cp && *cp != ' '; cp++) ;
    len = cp - word;

    if (!isalpha(*word))
	return t;
    ch = isupper(*word)? tolower(*word) : *word;
    for (tnum = 0; tnum < LAST_TAG && *tagattr[tnum].name != ch; tnum++) ;
    for ( ; tnum < LAST_TAG && *tagattr[tnum].name == ch; tnum++) {
	if (len == tagattr[tnum].length
	 && strncaseEQ(word, tagattr[tnum].name, len)) {
	    match = 1;
	    break;
	}
    }
    if (!match)
	return t;

    if (!opening_tag && !(tagattr[tnum].flags & (TF_BLOCK|TF_HAS_CLOSE)))
	return t;

    if ((mime_section->html & HF_IN_HIDING)
     && (opening_tag || tnum != blks[mime_section->html_blkcnt-1].tnum))
	return t;

    if (tagattr[tnum].flags & TF_BR)
	mime_section->html |= HF_NL_OK;

    if (opening_tag) {
	if (tagattr[tnum].flags & TF_NL) {
	    t = output_prep(t);
	    t = do_newline(t, HF_NL_OK);
	}
	if ((num = tagattr[tnum].flags & (TF_P|TF_LIST)) == TF_P
	 || (num == (TF_P|TF_LIST) && !(mime_section->html & HF_COMPACT))) {
	    t = output_prep(t);
	    t = do_newline(t, HF_P_OK);
	}
	if (tagattr[tnum].flags & TF_SPACE) {
	    if (mime_section->html & HF_SPACE_OK) {
		mime_section->html &= ~HF_SPACE_OK;
		*t++ = ' ';
	    }
	}
	if (tagattr[tnum].flags & TF_TAB) {
	    if (mime_section->html & HF_NL_OK) {
		mime_section->html &= ~HF_SPACE_OK;
		*t++ = '\t';
	    }
	}

	if ((tagattr[tnum].flags & TF_BLOCK)
	 && mime_section->html_blkcnt < HTML_MAX_BLOCKS) {
	    j = mime_section->html_blkcnt++;
	    blks[j].tnum = tnum;
	    blks[j].indent = 0;
	    blks[j].cnt = 0;

	    if (tagattr[tnum].flags & TF_LIST)
		mime_section->html |= HF_COMPACT;
	    else
		mime_section->html &= ~HF_COMPACT;
	}
	else
	    j = mime_section->html_blkcnt - 1;

	if ((tagattr[tnum].flags & (TF_BLOCK|TF_HIDE)) == (TF_BLOCK|TF_HIDE))
	    mime_section->html |= HF_IN_HIDING;

	switch (tnum) {
	  case TAG_BLOCKQUOTE:
	    if (((cp = find_attr(word, "type")) != NULL
	      && strncaseEQ(cp, "cite", 4))
	     || ((cp = find_attr(word, "style")) != NULL
	      && strncaseEQ(cp, "border-left:", 12)))
		blks[j].indent = '>';
	    else
		blks[j].indent = ' ';
	    break;
	  case TAG_HR:
	    t = output_prep(t);
	    *t++ = '-'; *t++ = '-';
	    mime_section->html |= HF_NL_OK;
	    t = do_newline(t, HF_NL_OK);
	    break;
	  case TAG_IMG:
	    t = output_prep(t);
	    if (mime_section->html & HF_SPACE_OK)
		*t++ = ' ';
	    strcpy(t, "[Image] ");
	    t += 8;
	    mime_section->html &= ~HF_SPACE_OK;
	    break;
	  case TAG_OL:
	    itype = 4;
	    if ((cp = find_attr(word, "type")) != NULL) {
		switch (*cp) {
		  case 'a':  itype = 5;  break;
		  case 'A':  itype = 6;  break;
		  case 'i':  itype = 7;  break;
		  case 'I':  itype = 8;  break;
		  default:   itype = 4;  break;
		}
	    }
	    blks[j].indent = itype;
	    break;
	  case TAG_UL:
	    itype = 1;
	    if ((cp = find_attr(word, "type")) != NULL) {
		switch (*cp) {
		  case 'd': case 'D':  itype = 1;  break;
		  case 'c': case 'C':  itype = 2;  break;
		  case 's': case 'S':  itype = 3;  break;
		}
	    }
	    else {
		for (i = 0; i < mime_section->html_blkcnt; i++) {
		    if (blks[i].indent && blks[i].indent < ' ') {
			if (++itype == 3)
			    break;
		    }
		}
	    }
	    blks[j].indent = itype;
	    break;
	  case TAG_LI:
	    t = output_prep(t);
	    ch = j < 0? ' ' : blks[j].indent;
	    switch (ch) {
	      case 1: case 2: case 3:
		t[-2] = bullets[ch-1];
		break;
	      case 4:
		sprintf(t-4, "%2d. ", ++blks[j].cnt);
		if (*t)
		    t += strlen(t);
		break;
	      case 5: case 6:
		cnt = blks[j].cnt++;
		if (cnt >= 26*26)
		    cnt = blks[j].cnt = 0;
		if (cnt >= 26)
		    t[-4] = letters[ch-5] + (cnt / 26) - 1;
		t[-3] =	letters[ch-5] + (cnt % 26);
		t[-2] = '.';
		break;
	      case 7:
		for (i = 0; i < 7; i++) {
		    if (isupper(roman_letters[i]))
			roman_letters[i] = tolower(roman_letters[i]);
		}
		goto roman_numerals;
	      case 8:
		for (i = 0; i < 7; i++) {
		    if (islower(roman_letters[i]))
			roman_letters[i] = toupper(roman_letters[i]);
		}
	      roman_numerals:
		cp = t - 6;
		cnt = ++blks[j].cnt;
		for (i = 0; cnt && i < 7; i++) {
		    num = roman_values[i];
		    while (cnt >= num) {
			*cp++ = roman_letters[i];
			cnt -= num;
		    }
		    j = (i | 1) + 1;
		    if (j < 7) {
			num -= roman_values[j];
			if (cnt >= num) {
			    *cp++ = roman_letters[j];
			    *cp++ = roman_letters[i];
			    cnt -= num;
			}
		    }
		}
		if (cp < t - 2) {
		    t -= 2;
		    for (cnt = t - cp; cp-- != t - 4; ) cp[cnt] = *cp;
		    while (cnt--) *++cp = ' ';
		}
		else
		    t = cp;
		*t++ = '.';
		*t++ = ' ';
		break;
	      default:
		*t++ = '*';
		*t++ = ' ';
		break;
	    }
	    mime_section->html |= HF_NL_OK|HF_P_OK;
	    break;
	  case TAG_PRE:
	    mime_section->html |= HF_IN_PRE;
	    word_wrap = word_wrap_in_pre;
	    break;
	}
    }
    else {
	if ((tagattr[tnum].flags & TF_BLOCK)) {
	    for (j = mime_section->html_blkcnt; j--; ) {
		if (blks[j].tnum == tnum) {
		    for (i = mime_section->html_blkcnt; --i > j; ) {
			t = tag_action(t, tagattr[blks[i].tnum].name,
				       CLOSING_TAG);
		    }
		    mime_section->html_blkcnt = j;
		    break;
		}
	    }
	    mime_section->html &= ~HF_IN_HIDING;
	    while (j-- > 0) {
		if (tagattr[blks[j].tnum].flags & TF_HIDE) {
		    mime_section->html |= HF_IN_HIDING;
		    break;
		}
	    }
	}

	j = mime_section->html_blkcnt - 1;
	if (j >= 0 && (tagattr[blks[j].tnum].flags & TF_LIST))
	    mime_section->html |= HF_COMPACT;
	else
	    mime_section->html &= ~HF_COMPACT;

	if ((tagattr[tnum].flags & TF_NL) && (mime_section->html & HF_NL_OK)) {
	    mime_section->html |= HF_QUEUED_NL;
	    mime_section->html &= ~HF_SPACE_OK;
	}
	if ((num = tagattr[tnum].flags & (TF_P|TF_LIST)) == TF_P
	 || (num == (TF_P|TF_LIST) && !(mime_section->html & HF_COMPACT))) {
	    if (mime_section->html & HF_P_OK) {
		mime_section->html |= HF_QUEUED_P;
		mime_section->html &= ~HF_SPACE_OK;
	    }
	}

	switch (tnum) {
	  case TAG_PRE:
	    mime_section->html &= ~HF_IN_PRE;
	    word_wrap = normal_word_wrap;
	    break;
	}
    }

#ifdef DEBUGGING
    						printf("%*s %% -> ", 4 + 25, "");
    if (mime_section->html == 0)		printf("0 ");
    if (mime_section->html & HF_IN_TAG)		printf("HF_IN_TAG ");
    if (mime_section->html & HF_IN_COMMENT)	printf("HF_IN_COMMENT ");
    if (mime_section->html & HF_IN_HIDING)	printf("HF_IN_HIDING ");
    if (mime_section->html & HF_IN_PRE)		printf("HF_IN_PRE ");
    if (mime_section->html & HF_IN_DQUOTE)	printf("HF_IN_DQUOTE ");
    if (mime_section->html & HF_IN_SQUOTE)	printf("HF_IN_SQUOTE ");
    if (mime_section->html & HF_QUEUED_P)	printf("HF_QUEUED_P ");
    if (mime_section->html & HF_P_OK)		printf("HF_P_OK ");
    if (mime_section->html & HF_QUEUED_NL)	printf("HF_QUEUED_NL ");
    if (mime_section->html & HF_NL_OK)		printf("HF_NL_OK ");
    if (mime_section->html & HF_NEED_INDENT)	printf("HF_NEED_INDENT ");
    if (mime_section->html & HF_SPACE_OK)	printf("HF_SPACE_OK ");
    if (mime_section->html & HF_COMPACT)	printf("HF_COMPACT ");
    printf("\n") FLUSH;
#endif
    return t;
}

static char*
output_prep(t)
char* t;
{
    if (mime_section->html & HF_QUEUED_P) {
	mime_section->html &= ~HF_QUEUED_P;
	t = do_newline(t, HF_P_OK);
    }
    if (mime_section->html & HF_QUEUED_NL) {
	mime_section->html &= ~HF_QUEUED_NL;
	t = do_newline(t, HF_NL_OK);
    }
    return t + do_indent(t);
}

static char*
do_newline(t, flag)
char* t;
int flag;
{
    if (mime_section->html & flag) {
	mime_section->html &= ~(flag|HF_SPACE_OK);
	t += do_indent(t);
	*t++ = '\n';
	mime_section->html_line_start = t - artbuf;
	mime_section->html |= HF_NEED_INDENT;
    }
    return t;
}

static int
do_indent(t)
char* t;
{
    HBLK* blks;
    int j, ch, spaces, len = 0;

    if (!(mime_section->html & HF_NEED_INDENT))
	return len;

    if (t)
	mime_section->html &= ~HF_NEED_INDENT;

    if ((blks = mime_section->html_blks) != NULL) {
	for (j = 0; j < mime_section->html_blkcnt; j++) {
	    if ((ch = blks[j].indent) != 0) {
		switch (ch) {
		  case '>':
		    spaces = 1;
		    break;
		  case ' ':
		    spaces = 3;
		    break;
		  case 7:  case 8:
		    ch = ' ';
		    spaces = 5;
		    break;
		  default:
		    ch = ' ';
		    spaces = 3;
		    break;
		}
		len += spaces + 1;
		if (len > 64) {
		    len -= spaces + 1;
		    break;
		}
		if (t) {
		    *t++ = ch;
		    while (spaces--)
			*t++ = ' ';
		}
	    }
	}
    }

    return len;
}

static char*
find_attr(str, attr)
char* str;
char* attr;
{
    int len = strlen(attr);
    char* cp = str;
    char* s;

    while ((cp = index(cp+1, '=')) != NULL) {
	for (s = cp; s[-1] == ' '; s--) ;
	while (cp[1] == ' ') cp++;
	if (s - str > len && s[-len-1] == ' ' && strncaseEQ(s-len,attr,len))
	    return cp+1;
    }
    return NULL;
}
