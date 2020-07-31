/* cache.h
 */
/* This software is copyrighted as detailed in the LICENSE file. */


/* Subjects get their own structure */

struct subject {
    SUBJECT* next;
    SUBJECT* prev;
    ARTICLE* articles;
    ARTICLE* thread;
    SUBJECT* thread_link;
    char* str;
    time_t date;
    short flags;
    short misc;		/* used for temporary totals and subject numbers */
};

/* subject flags */

#define SF_SEL		0x0001
#define SF_DEL		0x0002
#define SF_DELSEL	0x0004
#define SF_OLDVISIT	0x0008
#define SF_INCLUDED	0x0010

#define SF_VISIT	0x0200
#define SF_WASSELECTED  0x0400
#define SF_SUBJTRUNCED	0x1000
#define SF_ISOSUBJ	0x2000

/* This is our article-caching structure */

struct article {
    ART_NUM num;
    time_t date;
    SUBJECT* subj;
    char* from;
    char* msgid;
    char* xrefs;
#ifdef USE_FILTER
    char* refs;
#endif
    ARTICLE* parent;		/* parent article */
    ARTICLE* child1;		/* first child of a chain */
    ARTICLE* sibling;		/* our next sibling */
    ARTICLE* subj_next;		/* next article in subject order */
    long bytes;
    long lines;
#ifdef SCORE
    int score;
    unsigned short scoreflags;
#endif
    unsigned short flags;	/* article state flags */
    unsigned short flags2;	/* more state flags */
    unsigned short autofl;	/* auto-processing flags */
};

/* article flags */

#define AF_SEL		0x0001
#define AF_DEL		0x0002
#define AF_DELSEL	0x0004
#define AF_OLDSEL	0x0008
#define AF_INCLUDED	0x0010

#define AF_UNREAD	0x0020
#define AF_CACHED	0x0040
#define AF_THREADED	0x0080
#define AF_EXISTS	0x0100
#define AF_HAS_RE	0x0200
#define AF_KCHASE	0x0400
#define AF_MCHASE	0x0800
#define AF_YANKBACK	0x1000
#define AF_FROMTRUNCED	0x2000
#define AF_TMPMEM	0x4000
#define AF_FAKE 	0x8000

#define AF2_WASUNREAD   0x0001
#define AF2_NODEDRAWN   0x0002
#define AF2_CHANGED     0x0004
#define AF2_BOGUS	0x0008

/* See kfile.h for the AUTO_* flags */

#define article_ptr(an)      ((ARTICLE*)listnum2listitem(article_list,(long)(an)))
#define article_num(ap)      ((ap)->num)
#define article_find(an)     ((an) <= lastart && article_hasdata(an)? \
			      article_ptr(an) : NULL)
#define article_walk(cb,ag)  walk_list(article_list,cb,ag)
#define article_hasdata(an)  existing_listnum(article_list,(long)(an),0)
#define article_first(an)    existing_listnum(article_list,(long)(an),1)
#define article_next(an)     existing_listnum(article_list,(long)(an)+1,1)
#define article_last(an)     existing_listnum(article_list,(long)(an),-1)
#define article_prev(an)     existing_listnum(article_list,(long)(an)-1,-1)
#define article_nextp(ap)    ((ARTICLE*)next_listitem(article_list,(char*)(ap)))

#define article_exists(an)   (article_ptr(an)->flags & AF_EXISTS)
#define article_unread(an)   (article_ptr(an)->flags & AF_UNREAD)

#define was_read(an)	    (!article_hasdata(an) || !article_unread(an))
#define is_available(an)    ((an) <= lastart && article_hasdata(an) \
			  && article_exists(an))
#define is_unavailable(an)  (!is_available(an))

EXT LIST* article_list INIT(0);		/* a list of ARTICLEs */
EXT ARTICLE** artptr_list INIT(0);	/* the article-selector creates this */
EXT ARTICLE** artptr;			/* ditto -- used for article order */
EXT ART_NUM artptr_list_size INIT(0);

#ifdef ARTSEARCH
EXT ART_NUM srchahead INIT(0); 	/* are we in subject scan mode? */
				/* (if so, contains art # found or -1) */
#endif

EXT ART_NUM first_cached;
EXT ART_NUM last_cached;
EXT bool cached_all_in_range;
EXT ARTICLE* sentinel_artp;

#define DONT_FILL_CACHE	0
#define FILL_CACHE	1

EXT SUBJECT* first_subject INIT(0);
EXT SUBJECT* last_subject INIT(0);

EXT bool untrim_cache INIT(FALSE);

#ifdef PENDING
EXT ART_NUM subj_to_get;
EXT ART_NUM xref_to_get;
#endif

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

void cache_init _((void));
void build_cache _((void));
void close_cache _((void));
void cache_article _((ARTICLE*));
void check_for_near_subj _((ARTICLE*));
void change_join_subject_len _((int));
void check_poster _((ARTICLE*));
void uncache_article _((ARTICLE*,bool_int));
char* fetchcache _((ART_NUM,int,bool_int));
char* get_cached_line _((ARTICLE*,int,bool_int));
void set_subj_line _((ARTICLE*,char*,int));
int decode_header _((char*,char*,int));
void dectrl _((char*));
void set_cached_line _((ARTICLE*,int,char*));
int subject_cmp _((char*,int,HASHDATUM));
#ifdef PENDING
void look_ahead _((void));
#endif
void cache_until_key _((void));
#ifdef PENDING
bool cache_subjects _((void));
#endif
bool cache_xrefs _((void));
bool cache_all_arts _((void));
bool cache_unread_arts _((void));
bool art_data _((ART_NUM,ART_NUM,bool_int,bool_int));
bool cache_range _((ART_NUM,ART_NUM));
void clear_article _((ARTICLE*));
