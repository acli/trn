/* univ.h
*/
/* Universal selector
 *
 */

#define UN_NONE		0
/* textual placeholder */
#define UN_TXT		1
#define UN_DATASRC	2
#define UN_NEWSGROUP	3
#define UN_GROUPMASK	4
/* an individual article */
#define UN_ARTICLE	5
/* filename for a configuration file */
#define UN_CONFIGFILE	6
/* Virtual newsgroup file (reserved for compatability with strn) */
#define UN_VIRTUAL1	7
/* virtual newsgroup marker (for pass 2) */
#define UN_VGROUP	8
/* text file */
#define UN_TEXTFILE	9
/* keystroke help functions from help.c */
#define UN_HELPKEY	10

/* quick debugging: just has data */
#define UN_DEBUG1	-1
/* group that is deselected (with !group) */
#define UN_GROUP_DESEL  -2
/* virtual newsgroup deselected (with !group) */
#define UN_VGROUP_DESEL -3
/* generic deleted item -- no per-item memory */
#define UN_DELETED	-4

/* selector flags */
#define UF_SEL		0x01
#define UF_DEL		0x02
#define UF_DELSEL	0x04
#define UF_INCLUDED	0x10
#define UF_EXCLUDED	0x20

/* virtual/merged group flags (UNIV_VIRT_GROUP.flags) */
/* articles use minimum score */
#define UF_VG_MINSCORE	0x01
/* articles use maximum score */
#define UF_VG_MAXSCORE	0x02

struct univ_groupmask_data {
    char* title;
    char* masklist;
};

struct univ_configfile_data {
    char* title;
    char* fname;
    char* label;
};

struct univ_virt_data {
    char* ng;
    char* id;
    char* from;
    char* subj;
    ART_NUM num;
};

struct univ_virt_group {
    char* ng;
#ifdef SCORE
    int minscore;
    int maxscore;
#endif
    char flags;
};

struct univ_newsgroup {
    char* ng;
};

struct univ_textfile {
    char* fname;
};

union univ_data {
    char* str;
    int i;
    UNIV_GROUPMASK_DATA gmask;
    UNIV_CONFIGFILE_DATA cfile;
    UNIV_NEWSGROUP group;
    UNIV_VIRT_DATA virt;
    UNIV_VIRT_GROUP vgroup;
    UNIV_TEXTFILE textfile;
};

struct univ_item {
    UNIV_ITEM* next;
    UNIV_ITEM* prev;
    int num;				/* natural order (for sort) */
    int flags;				/* for selector */
    int type;				/* what kind of object is it? */
    char* desc;				/* default description */
#ifdef SCORE
    int score;
#endif
    UNIV_DATA data;			/* describes the object */
};

/* have we ever been initialized? */
EXT int univ_ever_init;

/* How deep are we in the tree? */
EXT int univ_level;

/* if TRUE, we are in the "virtual group" second pass */
EXT bool univ_ng_virtflag INIT(FALSE);

/* if TRUE, we are reading an article from a "virtual group" */
EXT bool univ_read_virtflag INIT(FALSE);

/* "follow"-related stuff (virtual groups) */
EXT bool univ_default_cmd INIT(FALSE);
EXT bool univ_follow INIT(TRUE);
EXT bool univ_follow_temp INIT(FALSE);

/* if TRUE, the user has loaded their own top univ. config file */
EXT bool univ_usrtop;

/* items which must be saved in context */
EXT UNIV_ITEM* first_univ;
EXT UNIV_ITEM* last_univ;
EXT UNIV_ITEM* sel_page_univ;
EXT UNIV_ITEM* sel_next_univ;
EXT char* univ_fname;			/* current filename (may be null) */
EXT char* univ_label;			/* current label (may be null) */
EXT char* univ_title;			/* title of current level */
EXT char* univ_tmp_file;		/* temp. file (may be null) */
EXT HASHTABLE* univ_ng_hash INIT(0);
EXT HASHTABLE* univ_vg_hash INIT(0);
/* end of items that must be saved */

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

void univ_init _((void));
void univ_startup _((void));
void univ_open _((void));
void univ_close _((void));
UNIV_ITEM* univ_add _((int,char*));
int univ_lines _((UNIV_ITEM*));
char* univ_desc_line _((UNIV_ITEM*,int));
void univ_add_text _((char*));
void univ_add_debug _((char*,char*));
void univ_add_group _((char*,char*));
void univ_add_mask _((char*,char*));
void univ_add_file _((char*,char*,char*));
UNIV_ITEM* univ_add_virt_num _((char*,char*,ART_NUM));
void univ_add_textfile _((char*,char*));
void univ_add_virtgroup _((char*));
void univ_use_pattern _((char*,int));
void univ_use_group_line _((char*,int));
bool univ_file_load _((char*,char*,char*));
void univ_mask_load _((char*,char*));
void univ_redofile _((void));
void univ_edit _((void));
void univ_page_file _((char*));
void univ_ng_virtual _((void));
int univ_visit_group_main _((char*));
void univ_virt_pass _((void));
void sort_univ _((void));
char* univ_article_desc _((UNIV_ITEM*));
void univ_help_main _((int));
void univ_help _((int));
char* univ_keyhelp_modestr _((UNIV_ITEM*));
