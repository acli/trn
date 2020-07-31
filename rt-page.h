/* rt-page.h
*/
/* This software is copyrighted as detailed in the LICENSE file. */


#define PRESERVE_PAGE     0
#define FILL_LAST_PAGE    1

EXT int sel_total_obj_cnt;
EXT int sel_prior_obj_cnt;
EXT int sel_page_obj_cnt;
EXT int sel_page_item_cnt;
EXT int sel_max_per_page;
EXT int sel_max_line_cnt;

EXT ARTICLE** sel_page_app;
EXT ARTICLE** sel_next_app;
EXT ARTICLE* sel_last_ap;
EXT SUBJECT* sel_page_sp;
EXT SUBJECT* sel_next_sp;
EXT SUBJECT* sel_last_sp;

EXT char* sel_grp_dmode INIT("*slm");
EXT char* sel_art_dmode INIT("*lmds");

EXT bool group_init_done INIT(TRUE);

union sel_union {
    ARTICLE* ap;
    SUBJECT* sp;
    ADDGROUP* gp;
    MULTIRC* mp;
    NGDATA* np;
    UNIV_ITEM* un;
    int op;
};

struct sel_item {
    SEL_UNION u;
    int line;
    int sel;
};

#define MAX_SEL 99
EXT SEL_ITEM sel_items[MAX_SEL];

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

bool set_sel_mode _((char_int));
char* get_sel_order _((int));
bool set_sel_order _((int,char*));
bool set_sel_sort _((int,char_int));
void set_selector _((int,int));
void init_pages _((bool_int));
bool first_page _((void));
bool last_page _((void));
bool next_page _((void));
bool prev_page _((void));
bool calc_page _((SEL_UNION));
void display_page_title _((bool_int));
void display_page _((void));
void update_page _((void));
void output_sel _((int,int,bool_int));
void display_option _((int,int));
