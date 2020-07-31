/* This file Copyright 1992,1993 by Clifford A. Adams */
/* scan.h
 *
 * (Mostly scan context declarations.)
 */

/* return codes.  First two should be the same as article scan codes */
#define S_QUIT (-1)
#define S_ERR (-2)
/* command was not found in common scan subset */
#define S_NOTFOUND (-3)

/* number of entries allocated for a page */
#define MAX_PAGE_SIZE 256

/* different context types */
#define S_ART 1
#define S_GROUP 2
#define S_HELP 3
#define S_VIRT 4

struct page_ent {
    long entnum;	/* entry (article/message/newsgroup) number */
    short lines;	/* how many screen lines to describe? */
    short start_line;	/* screen line (0 = line under top status bar) */
    char pageflags;	/* not currently used. */
};

struct scontext {
    int type;			/* context type */

    /* ordering information */
    long* ent_sort;		/* sorted list of entries in the context */
    long ent_sort_max;		/* maximum index of sorted array */
    long ent_sorted_max;	/* maximum index *that is sorted* */
    long* ent_index;		/* indexes into ent_sorted */
    long ent_index_max;		/* maximum entry number added */

    int page_size;		/* number of entries allocated for page */
				/* (usually fixed, > max screen lines) */
    PAGE_ENT* page_ents;	/* array of entries on page */
    /* -1 means not initialized for top and bottom entry */
    long top_ent;		/* top entry on page */
    long bot_ent;		/* bottom entry (note change) */
    bool refill;		/* does the page need refilling? */
    /* refresh entries */
    bool ref_all;		/* refresh all on page */
    bool ref_top;		/* top status bar */
    bool ref_bot;		/* bottom status bar */
    /* -1 for the next two entries means don't refresh */
    short ref_status;		/* line to start refreshing status from */
    short ref_desc;		/* line to start refreshing descript. from */
    /* screen sizes */
    short top_lines;		/* lines for top status bar */
    short bot_lines;		/* lines for bottom status bar */
    short status_cols;		/* characters for status column */
    short cursor_cols;		/* characters for cursor column */
    short itemnum_cols;		/* characters for item number column */
    short desc_cols;		/* characters for description column */
    /* pointer info */
    short ptr_page_line;	/* page_ent index */
    long flags;
};

/* the current values */

long* s_ent_sort;		/* sorted list of entries in the context */
long s_ent_sort_max;		/* maximum index of sorted array */
long s_ent_sorted_max;		/* maximum index *that is sorted* */
long* s_ent_index;		/* indexes into ent_sorted */
long s_ent_index_max;		/* maximum entry number added */

int s_page_size;		/* number of entries allocated for page */
				/* (usually fixed, > max screen lines) */
PAGE_ENT* page_ents;		/* array of entries on page */
/* -1 means not initialized for top and bottom entry */
long s_top_ent;		/* top entry on page */
long s_bot_ent;		/* bottom entry (note change) */
bool s_refill;			/* does the page need refilling? */
/* refresh entries */
bool s_ref_all;		/* refresh all on page */
bool s_ref_top;		/* top status bar */
bool s_ref_bot;		/* bottom status bar */
/* -1 for the next two entries means don't refresh */
short s_ref_status;		/* line to start refreshing status from */
short s_ref_desc;		/* line to start refreshing descript. from */
/* screen sizes */
short s_top_lines;		/* lines for top status bar */
short s_bot_lines;		/* lines for bottom status bar */
short s_status_cols;		/* characters for status column */
short s_cursor_cols;		/* characters for cursor column */
short s_itemnum_cols;		/* characters for item number column */
short s_desc_cols;		/* characters for description column */
/* pointer info */
short s_ptr_page_line;		/* page_ent index */
long  s_flags;			/* misc. flags */

EXT int s_num_contexts INIT(0);
/* array of context structures */
EXT SCONTEXT* s_contexts INIT((SCONTEXT*)NULL);

/* current context number */
EXT int s_cur_context INIT(0);
/* current context type (for fast switching) */
int s_cur_type;

/* options */
/* show item numbers by default */
EXT int s_itemnum INIT(TRUE);
EXT int s_mode_vi INIT(0);

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

void s_init_context _((int,int));
int s_new_context _((int));
void s_save_context _((void));
void s_change_context _((int));
void s_clean_contexts _((void));
void s_delete_context _((int));
