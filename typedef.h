/* typedef.h
 */

/* some important types */

typedef int		NG_NUM;		/* newsgroup number */
typedef long		ART_NUM;	/* article number */
typedef long		ART_UNREAD;	/* could be short to save space */
typedef long		ART_POS;	/* char position in article file */
typedef int		ART_LINE;	/* line position in article file */
typedef long		ACT_POS;	/* char position in active file */
typedef unsigned int	MEM_SIZE;	/* for passing to malloc */
typedef unsigned char	Uchar;		/* more space-efficient */

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

/* addng.h */

typedef struct addgroup ADDGROUP;

/* cache.h */

typedef struct subject SUBJECT;
typedef struct article ARTICLE;

/* color.ih */

typedef struct color_obj COLOR_OBJ;

/* datasrc.h */

typedef struct srcfile SRCFILE;
typedef struct datasrc DATASRC;

/* hash.h */

typedef struct hashdatum HASHDATUM;

/* hash.ih */

typedef struct hashent HASHENT;
typedef struct hashtable HASHTABLE;

/* head.h */

typedef struct headtype HEADTYPE;
typedef struct user_headtype USER_HEADTYPE;

/* list.h */

typedef struct listnode LISTNODE;
typedef struct list LIST;

/* mime.h */

typedef struct hblk HBLK;
typedef struct mime_sect MIME_SECT;
typedef struct html_tags HTML_TAGS;
typedef struct mimecap_entry MIMECAP_ENTRY;

/* ngdata.h */

typedef struct ngdata NGDATA;

/* nntpclient.h */

typedef struct nntplink NNTPLINK;

/* rcstuff.h */

typedef struct newsrc NEWSRC;
typedef struct multirc MULTIRC;

/* rt-mt.ih */

typedef struct packed_root PACKED_ROOT;
typedef struct packed_article PACKED_ARTICLE;
typedef struct total TOTAL;
typedef struct bmap BMAP;

/* rt-page.h */

typedef union sel_union SEL_UNION;
typedef struct sel_item SEL_ITEM;

/* scan.h */

typedef struct page_ent PAGE_ENT;
typedef struct scontext SCONTEXT;

/* scanart.h */

typedef struct sa_entrydata SA_ENTRYDATA;

/* scorefile.h */

typedef struct sf_entry SF_ENTRY;
typedef struct sf_file SF_FILE;

/* search.h */

typedef struct compex COMPEX;

/* term.ih */

typedef struct keymap KEYMAP;

/* univ.h */

typedef struct univ_groupmask_data UNIV_GROUPMASK_DATA;
typedef struct univ_configfile_data UNIV_CONFIGFILE_DATA;
typedef struct univ_virt_data UNIV_VIRT_DATA;
typedef struct univ_virt_group UNIV_VIRT_GROUP;
typedef struct univ_newsgroup UNIV_NEWSGROUP;
typedef struct univ_textfile UNIV_TEXTFILE;
typedef union univ_data UNIV_DATA;
typedef struct univ_item UNIV_ITEM;

/* util.h */

typedef struct ini_words INI_WORDS;
