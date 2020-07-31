/* rt-select.h
*/
/* This software is copyrighted as detailed in the LICENSE file. */


EXT bool sel_rereading INIT(0);
EXT char sel_disp_char[] INIT(" +-*");

#define SM_THREAD	1
#define SM_SUBJECT	2
#define SM_ARTICLE	3
#define SM_NEWSGROUP	4
#define SM_ADDGROUP	5
#define SM_MULTIRC	6
#define SM_OPTIONS	7
#define SM_UNIVERSAL	8

EXT int sel_mode;
EXT int sel_defaultmode INIT(SM_THREAD);
EXT int sel_threadmode INIT(SM_THREAD);

#define SS_DATE		1
#define SS_STRING	2
#define SS_AUTHOR	3
#define SS_COUNT	4
#define SS_NATURAL	5
#define SS_GROUPS	6
#define SS_LINES	7
/* NOTE: The score order is still valid even without scoring enabled. */
/*       (The real order is then something like natural or date.) */
#define SS_SCORE	8

EXT char* sel_mode_string;
EXT int sel_sort;
EXT int sel_artsort INIT(SS_GROUPS);
EXT int sel_threadsort INIT(SS_DATE);
EXT int sel_newsgroupsort INIT(SS_NATURAL);
EXT int sel_addgroupsort INIT(SS_NATURAL);
EXT int sel_univsort INIT(SS_NATURAL);

EXT char* sel_sort_string;
EXT int sel_direction INIT(1);
EXT bool sel_exclusive INIT(FALSE);
EXT int sel_mask INIT(1);

EXT bool selected_only INIT(FALSE);
EXT ART_UNREAD selected_count INIT(0);
EXT int selected_subj_cnt INIT(0);
EXT int added_articles INIT(0);

EXT char* sel_chars;
EXT int sel_item_index;
EXT int sel_last_line;
EXT bool sel_at_end;
EXT bool art_sel_ilock INIT(FALSE);

#define DS_ASK  	1
#define DS_UPDATE	2
#define DS_DISPLAY	3
#define DS_RESTART	4
#define DS_STATUS	5
#define DS_QUIT 	6
#define DS_DOCOMMAND	7
#define DS_ERROR	8


#define UR_NORM		1
#define UR_BREAK	2	/* request return to selector */
#define UR_ERROR	3	/* non-normal return */

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

char article_selector _((char_int));
char multirc_selector _((void));
char newsgroup_selector _((void));
char addgroup_selector _((int));
char option_selector _((void));
char universal_selector _((void));
void selector_mouse _((int,int,int,int,int,int));
int univ_visit_group _((char*));
void univ_visit_help _((int));
