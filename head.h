/* head.h
 */
/* This software is copyrighted as detailed in the LICENSE file. */


#define HEAD_FIRST SOME_LINE

/* types of header lines (if only C really believed in enums)
 * (These must stay in alphabetic order at least in the first letter.
 * Within each letter it helps to arrange in increasing likelihood.)
 */

#define PAST_HEADER	0			/* body */
#define SHOWN_LINE	(PAST_HEADER+1)		/* unrecognized but shown */
#define HIDDEN_LINE	(SHOWN_LINE+1)		/* unrecognized but hidden */
#define CUSTOM_LINE	(HIDDEN_LINE+1)		/* to isolate a custom line */
#define SOME_LINE	(CUSTOM_LINE+1)		/* default for unrecognized */
#define AUTHOR_LINE	(SOME_LINE+1)		/* Author */
#define BYTES_LINE	(AUTHOR_LINE+1)		/* Bytes */
#define CONTNAME_LINE	(BYTES_LINE+1)		/* Content-Name */
#define CONTDISP_LINE	(CONTNAME_LINE+1)	/* Content-Disposition */
#define CONTLEN_LINE	(CONTDISP_LINE+1)	/* Content-Length */
#define CONTXFER_LINE	(CONTLEN_LINE+1)	/* Content-Transfer-Encoding */
#define CONTTYPE_LINE	(CONTXFER_LINE+1)	/* Content-Type */
#define DIST_LINE	(CONTTYPE_LINE+1)	/* distribution */
#define DATE_LINE	(DIST_LINE+1)		/* date */
#define EXPIR_LINE	(DATE_LINE+1)		/* expires */
#define FOLLOW_LINE	(EXPIR_LINE+1)		/* followup-to */
#define FROM_LINE	(FOLLOW_LINE+1)		/* from */
#define INREPLY_LINE	(FROM_LINE+1)		/* in-reply-to */
#define KEYW_LINE	(INREPLY_LINE+1)	/* keywords */
#define LINES_LINE	(KEYW_LINE+1)		/* lines */
#define MIMEVER_LINE	(LINES_LINE+1)		/* mime-version */
#define MSGID_LINE	(MIMEVER_LINE+1)	/* message-id */
#define NGS_LINE	(MSGID_LINE+1)		/* newsgroups */
#define PATH_LINE	(NGS_LINE+1)		/* path */
#define RVER_LINE	(PATH_LINE+1)		/* relay-version */
#define REPLY_LINE	(RVER_LINE+1)		/* reply-to */
#define REFS_LINE	(REPLY_LINE+1)		/* references */
#define SUMRY_LINE	(REFS_LINE+1)		/* summary */
#define SUBJ_LINE	(SUMRY_LINE+1)		/* subject */
#define XREF_LINE	(SUBJ_LINE+1)		/* xref */
#define HEAD_LAST	(XREF_LINE+1)		/* total # of headers */

struct headtype {
    char* name;			/* header line identifier */
    ART_POS minpos;		/* pointer to beginning of line in article */
    ART_POS maxpos;		/* pointer to end of line in article */
    char length;		/* the header's string length */
    char flags;			/* the header's flags */
};

struct user_headtype {
    char* name;			/* user-defined headers */
    char length;		/* the header's string length */
    char flags;			/* the header's flags */
};

#define HT_HIDE	    0x01	/* hide this line */
#define HT_MAGIC    0x02	/* do any special processing on this line */
#define HT_CACHED   0x04	/* this information is cached article data */
#define HT_DEFHIDE  0x08	/* hidden by default */
#define HT_DEFMAGIC 0x10	/* magic by default */
#define HT_MAGICOK  0x20	/* magic even possible for line */

/* This array must stay in the same order as the list above */

#ifndef DOINIT
EXT struct headtype htype[HEAD_LAST];
#else

#define HIDDEN    (HT_HIDE|HT_DEFHIDE)
#define MAGIC_ON  (HT_MAGICOK|HT_MAGIC|HT_DEFMAGIC)
#define MAGIC_OFF (HT_MAGICOK)

#ifdef DBM_XREFS
#define XREF_CACHED 0
#define NGS_CACHED  HT_CACHED
#else
#define XREF_CACHED HT_CACHED
#define NGS_CACHED  0
#endif
#ifdef USE_FILTER
#define FILT_CACHED HT_CACHED
#else
#define FILT_CACHED 0
#endif

struct headtype htype[HEAD_LAST] = {
 /* name             minpos   maxpos  length   flag */
    {nullstr,/*BODY*/	0,	0,	0,	0		},
    {nullstr,/*SHOWN*/	0,	0,	0,	0		},
    {nullstr,/*HIDDEN*/	0,	0,	0,	HIDDEN		},
    {nullstr,/*CUSTOM*/	0,	0,	0,	0		},
    {"unrecognized",	0,	0,	12,	HIDDEN		},
    {"author",		0,	0,	6,	0		},
    {"bytes",		0,	0,	5,	HIDDEN|HT_CACHED},
    {"content-name",	0,	0,	12,	HIDDEN		},
    {"content-disposition",
			0,	0,	19,	HIDDEN		},
    {"content-length",	0,	0,	14,	HIDDEN		},
    {"content-transfer-encoding",
			0,	0,	25,	HIDDEN		},
    {"content-type",	0,	0,	12,	HIDDEN		},
    {"distribution",	0,	0,	12,	0		},
    {"date",		0,	0,	4,	MAGIC_ON	},
    {"expires",		0,	0,	7,	HIDDEN|MAGIC_ON	},
    {"followup-to",	0,	0,	11,	0		},
    {"from",		0,	0,	4,	MAGIC_OFF|HT_CACHED},
    {"in-reply-to",	0,	0,	11,	HIDDEN		},
    {"keywords",	0,	0,	8,	0		},
    {"lines",		0,	0,	5,	HT_CACHED	},
    {"mime-version",	0,	0,	12,	MAGIC_ON|HIDDEN	},
    {"message-id",	0,	0,	10,	HIDDEN|HT_CACHED},
    {"newsgroups",	0,	0,	10,	MAGIC_ON|HIDDEN|NGS_CACHED},
    {"path",		0,	0,	4,	HIDDEN		},
    {"relay-version",	0,	0,	13,	HIDDEN		},
    {"reply-to",	0,	0,	8,	0		},
    {"references",	0,	0,	10,	HIDDEN|FILT_CACHED},
    {"summary",		0,	0,	7,	0		},
    {"subject",		0,	0,	7,	MAGIC_ON|HT_CACHED},
    {"xref",		0,	0,	4,	HIDDEN|XREF_CACHED},
};

#undef HIDDEN
#undef MAGIC_ON
#undef MAGIC_OFF
#undef NGS_CACHED
#undef XREF_CACHED

#endif

EXT struct user_headtype* user_htype INIT(NULL);
EXT short user_htypeix[26];
EXT int user_htype_cnt INIT(0);
EXT int user_htype_max INIT(0);

EXT ART_NUM parsed_art INIT(0);		/* the article number we've parsed */
EXT ARTICLE* parsed_artp INIT(NULL);	/* the article ptr we've parsed */
EXT int in_header INIT(0);		/* are we decoding the header? */
EXT char* headbuf;
EXT long headbuf_size;

#define PREFETCH_SIZE 5

#define fetchsubj(artnum,copy) prefetchlines(artnum,SUBJ_LINE,copy)
#define fetchfrom(artnum,copy) prefetchlines(artnum,FROM_LINE,copy)
#ifdef DBM_XREFS
#define fetchxref(artnum,copy) prefetchlines(artnum,NGS_LINE,copy)
#else
#define fetchxref(artnum,copy) prefetchlines(artnum,XREF_LINE,copy)
#endif

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

void head_init _((void));
#ifdef DEBUG
void dumpheader _((char*));
#endif
int set_line_type _((char*,char*));
int get_header_num _((char*));
void start_header _((ART_NUM));
void end_header_line _((void));
bool parseline _((char*,int,int));
void end_header _((void));
bool parseheader _((ART_NUM));
char* fetchlines _((ART_NUM,int));
#ifdef SCAN
char* mp_fetchlines _((ART_NUM,int,int));
#endif
char* prefetchlines _((ART_NUM,int,bool_int));
