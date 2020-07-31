/* ngdata.h
 */
/* This software is copyrighted as detailed in the LICENSE file. */


struct ngdata {
    NGDATA* prev;
    NGDATA* next;
    NEWSRC* rc;		/* which rc is this line from? */
    char* rcline;	/* pointer to group's .newsrc line */
    ART_NUM abs1st;	/* 1st real article in newsgroup */
    ART_NUM ngmax;	/* high message num for the group */
    ART_UNREAD toread;	/* number of articles to be read in newsgroup */
			/* < 0 is invalid or unsubscribed newsgroup */
    NG_NUM num;		/* a possible sort order for this group */
    int numoffset;	/* offset from rcline to numbers on line */
    char subscribechar;	/* holds the character : or ! while spot is \0 */
    char flags;  	/* flags for each group */
};

EXT LIST* ngdata_list INIT(NULL); /* a list of NGDATA */
EXT int ngdata_cnt INIT(0);
EXT NG_NUM newsgroup_cnt INIT(0); /* all newsgroups in our current newsrc(s) */
EXT NG_NUM newsgroup_toread INIT(0);
EXT ART_UNREAD ng_min_toread INIT(1); /* == TR_ONE or TR_NONE */

EXT NGDATA* first_ng INIT(NULL);
EXT NGDATA* last_ng INIT(NULL);
EXT NGDATA* ngptr INIT(NULL);	/* current newsgroup data ptr */

EXT NGDATA* current_ng INIT(NULL);/* stable current newsgroup so we can ditz with ngptr */
EXT NGDATA* recent_ng INIT(NULL); /* the prior newsgroup we visited */
EXT NGDATA* starthere INIT(NULL); /* set to the first newsgroup with unread news on startup */

#define ngdata_ptr(ngnum) ((NGDATA*)listnum2listitem(ngdata_list,(long)(ngnum)))
/*#define ngdata_num(ngptr) listitem2listnum(ngdata_list,(char*)ngptr)*/

EXT NGDATA* sel_page_np;
EXT NGDATA* sel_next_np;

EXT ART_NUM absfirst INIT(0);	/* 1st real article in current newsgroup */
EXT ART_NUM firstart INIT(0);	/* minimum unread article number in newsgroup */
EXT ART_NUM lastart INIT(0);	/* maximum article number in newsgroup */
EXT ART_UNREAD missing_count;	/* for reports on missing articles */

EXT char* moderated;
EXT char* redirected;
EXT bool ThreadedGroup;

/* CAA goto-newsgroup extensions */
EXT NGDATA* ng_go_ngptr INIT(NULL);
EXT ART_NUM ng_go_artnum INIT(0);
EXT char* ng_go_msgid INIT(NULL);

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

void ngdata_init _((void));
void set_ng _((NGDATA*));
int access_ng _((void));
void chdir_newsdir _((void));
void grow_ng _((ART_NUM));
void sort_newsgroups _((void));
void ng_skip _((void));
ART_NUM getngsize _((NGDATA*));
