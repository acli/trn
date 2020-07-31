/* rcstuff.h
 */
/* This software is copyrighted as detailed in the LICENSE file. */


#define TR_ONE ((ART_UNREAD) 1)
#define TR_NONE ((ART_UNREAD) 0)
#define TR_UNSUB ((ART_UNREAD) -1)
			/* keep this one as -1, some tests use >= TR_UNSUB */
#define TR_IGNORE ((ART_UNREAD) -2)
#define TR_BOGUS ((ART_UNREAD) -3)
#define TR_JUNK ((ART_UNREAD) -4)

#define NF_SEL		0x01
#define NF_DEL		0x02
#define NF_DELSEL	0x04
#define NF_INCLUDED	0x10
#define NF_UNTHREADED	0x40
#define NF_VISIT	0x80

#define ADDNEW_SUB ':'
#define ADDNEW_UNSUB '!'

#define GNG_RELOC	0x0001
#define GNG_FUZZY	0x0002

EXT HASHTABLE* newsrc_hash INIT(NULL);

struct newsrc {
    NEWSRC*	next;
    DATASRC*	datasrc;
    char*	name;		/* the name of the associated newsrc */
    char*	oldname;	/* the backup of the newsrc */
    char*	newname;	/* our working newsrc file */
    char*	infoname;	/* the time/size info file */
    char*	lockname;	/* the lock file we created */
    int		flags;
};

#define RF_ADD_NEWGROUPS 0x0001
#define RF_ADD_GROUPS	 0x0002
#define RF_OPEN		 0x0100
#define RF_ACTIVE	 0x0200
#define RF_RCCHANGED	 0x0400

struct multirc {
    NEWSRC* first;
    int num;
    int flags;
};

#define MF_SEL		0x0001
#define MF_INCLUDED	0x0010

EXT MULTIRC* sel_page_mp;
EXT MULTIRC* sel_next_mp;

#define multirc_ptr(n)  ((MULTIRC*)listnum2listitem(multirc_list,(long)(n)))
#define multirc_low()   ((MULTIRC*)listnum2listitem(multirc_list,existing_listnum(multirc_list,0L,1)))
#define multirc_high()  ((MULTIRC*)listnum2listitem(multirc_list,existing_listnum(multirc_list,multirc_list->high,-1)))
#define multirc_next(p) ((MULTIRC*)next_listitem(multirc_list,(char*)(p)))
#define multirc_prev(p) ((MULTIRC*)prev_listitem(multirc_list,(char*)(p)))

EXT LIST* multirc_list;	/* a list of all MULTIRCs */
EXT MULTIRC* multirc;		/* the current MULTIRC */

EXT bool paranoid INIT(FALSE);	/* did we detect some inconsistency in .newsrc? */
EXT int addnewbydefault INIT(0);

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

bool rcstuff_init _((void));
NEWSRC* new_newsrc _((char*,char*,char*));
bool use_multirc _((MULTIRC*));
void unuse_multirc _((MULTIRC*));
bool use_next_multirc _((MULTIRC*));
bool use_prev_multirc _((MULTIRC*));
char* multirc_name _((MULTIRC*));
void abandon_ng _((NGDATA*));
bool get_ng _((char*,int));
#ifdef RELOCATE
bool relocate_newsgroup _((NGDATA*,NG_NUM));
#endif
void list_newsgroups _((void));
NGDATA* find_ng _((char*));
void cleanup_newsrc _((NEWSRC*));
void sethash _((NGDATA*));
void checkpoint_newsrcs _((void));
bool write_newsrcs _((MULTIRC*));
void get_old_newsrcs _((MULTIRC*));
