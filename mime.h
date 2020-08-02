/* mime.h
 */

struct hblk {
    int	    tnum;
    short   cnt;
    char    indent;
};

struct mime_sect {
    MIME_SECT* prev;
    char*   filename;
    char*   type_name;
    char*   type_params;
    char*   boundary;
    int     html_line_start;
    HBLK*   html_blks;
    short   type;
    short   encoding;
    short   part;
    short   total;
    short   boundary_len;
    short   flags;
    short   html;
    short   html_blkcnt;
};

#define MSF_INLINE	0x0001
#define MSF_ALTERNATIVE	0x0002
#define MSF_ALTERNADONE	0x0004

/* Only used with HTMLTEXT_MIME */
#define HF_IN_TAG	0x0001
#define HF_IN_COMMENT	0x0002
#define HF_IN_HIDING	0x0004
#define HF_IN_PRE	0x0008
#define HF_IN_DQUOTE	0x0010
#define HF_IN_SQUOTE	0x0020
#define HF_QUEUED_P	0x0040
#define HF_P_OK		0x0080
#define HF_QUEUED_NL	0x0100
#define HF_NL_OK	0x0200
#define HF_NEED_INDENT	0x0400
#define HF_SPACE_OK	0x0800
#define HF_COMPACT	0x1000

#define HTML_MAX_BLOCKS	256

#define TF_BLOCK	0x0001		/* This implies TF_HAS_CLOSE */
#define TF_HAS_CLOSE	0x0002
#define TF_NL		0x0004
#define TF_P		0x0008
#define TF_BR		0x0010
#define TF_LIST		0x0020
#define TF_HIDE		0x0040
#define TF_SPACE	0x0080
#define TF_TAB		0x0100

/* NOTE: This must match tagattr below */
#define TAG_BLOCKQUOTE	0
#define TAG_BR		(TAG_BLOCKQUOTE+1)
#define TAG_DIV		(TAG_BR+1)
#define TAG_HR		(TAG_DIV+1)
#define TAG_IMG		(TAG_HR+1)
#define TAG_LI		(TAG_IMG+1)
#define TAG_OL		(TAG_LI+1)
#define TAG_P		(TAG_OL+1)
#define TAG_PRE		(TAG_P+1)
#define TAG_SCRIPT	(TAG_PRE+1)
#define TAG_STYLE	(TAG_SCRIPT+1)
#define TAG_TD		(TAG_STYLE+1)
#define TAG_TH		(TAG_TD+1)
#define TAG_TR		(TAG_TH+1)
#define TAG_TITLE	(TAG_TR+1)
#define TAG_UL		(TAG_TITLE+1)
#define TAG_XML		(TAG_UL+1)
#define LAST_TAG	(TAG_XML+1)

#define CLOSING_TAG	0
#define OPENING_TAG	1

struct html_tags {
    char* name;
    char length;
    int flags;
};

#ifndef DOINIT
EXT HTML_TAGS tagattr[LAST_TAG];
#else

HTML_TAGS tagattr[LAST_TAG] = {
 /* name               length   flags */
    {"blockquote",	10,	TF_BLOCK | TF_P | TF_NL			},
    {"br",		 2,	TF_NL | TF_BR				},
    {"div",		 3,	TF_BLOCK | TF_NL			},
    {"hr",		 2,	TF_NL					},
    {"img",		 3,	0					},
    {"li",		 2,	TF_NL					},
    {"ol",		 2,	TF_BLOCK | TF_P | TF_NL | TF_LIST	},
    {"p",		 1,	TF_HAS_CLOSE | TF_P | TF_NL		},
    {"pre",		 3,	TF_BLOCK | TF_P | TF_NL			},
    {"script",		 6,	TF_BLOCK | TF_HIDE			},
    {"style",		 5,	TF_BLOCK | TF_HIDE			},
    {"td",		 2,	TF_TAB					},
    {"th",		 2,	TF_TAB					},
    {"tr",		 2,	TF_NL					},
    {"title",		 5,	TF_BLOCK | TF_HIDE			},
    {"ul",		 2,	TF_BLOCK | TF_P | TF_NL | TF_LIST	},
    {"xml",		 3,	TF_BLOCK | TF_HIDE			}, /* non-standard but seen in the wild */
};
#endif

EXT LIST* mimecap_list;

#define mimecap_ptr(n) ((MIMECAP_ENTRY*)listnum2listitem(mimecap_list,(long)(n)))

EXT MIME_SECT mime_article;
EXT MIME_SECT* mime_section INIT(&mime_article);
EXT short mime_state;
EXT char* multipart_separator INIT("-=-=-=-=-=-");

#define NOT_MIME	0
#define TEXT_MIME	1
#define ISOTEXT_MIME	2
#define MESSAGE_MIME	3
#define MULTIPART_MIME	4
#define IMAGE_MIME	5
#define AUDIO_MIME	6
#define APP_MIME	7
#define UNHANDLED_MIME	8
#define SKIP_MIME	9
#define DECODE_MIME	10
#define BETWEEN_MIME	11
#define END_OF_MIME	12
#define HTMLTEXT_MIME	13
#define ALTERNATE_MIME	14

#define MENCODE_NONE		0
#define MENCODE_BASE64		1
#define MENCODE_QPRINT		2
#define MENCODE_UUE		3
#define MENCODE_UNHANDLED	4

struct mimecap_entry {
    char* contenttype;
    char* command;
    char* testcommand;
    char* label;
    int flags;
};

#define MCF_NEEDSTERMINAL	0x0001
#define MCF_COPIOUSOUTPUT	0x0002

EXT bool auto_view_inline INIT(FALSE);
EXT char* mime_getc_line INIT(NULL);

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

void mime_init _((void));
void mime_ReadMimecap _((char*));
MIMECAP_ENTRY* mime_FindMimecapEntry _((char*,int));
bool mime_TypesMatch _((char*,char*));
int mime_Exec _((char*));
void mime_InitSections _((void));
void mime_PushSection _((void));
bool mime_PopSection _((void));
void mime_ClearStruct _((MIME_SECT*));
void mime_SetArticle _((void));
void mime_ParseType _((MIME_SECT*,char*));
void mime_ParseDisposition _((MIME_SECT*,char*));
void mime_ParseEncoding _((MIME_SECT*,char*));
void mime_ParseSubheader _((FILE*,char*));
void mime_SetState _((char*));
int mime_EndOfSection _((char*));
char* mime_ParseParams _((char*));
char* mime_FindParam _((char*,char*));
char* mime_SkipWhitespace _((char*));
void mime_DecodeArticle _((bool_int));
void mime_Description _((MIME_SECT*,char*,int));
int qp_decodestring _((char*,char*,bool_int));
int qp_decode _((FILE*,int));
int b64_decodestring _((char*,char*));
int b64_decode _((FILE*,int));
int cat_decode _((FILE*,int));
int filter_html _((char*,char*));
