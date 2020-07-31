/* This file Copyright 1992 by Clifford A. Adams */
/* scan.c
 *
 * Scan initialization/cleanup and context control.
 */

/* CLEANUP: make a scontext file for the scan context stuff. */

#include "EXTERN.h"
#include "common.h"
#ifdef SCAN
#include "final.h"		/* for assert statements */
#include "util.h"		/* allocation */
#include "INTERN.h"
#include "scan.h"
#include "EXTERN.h"
#include "sorder.h"

void
s_init_context(cnum,type)
int cnum,type;
{
    SCONTEXT *p;
    int i;

    /* s_num_contexts not incremented until last moment */
    if (cnum < 0 || cnum > s_num_contexts) {
	printf("s_init_context: illegal context number %d!\n",cnum) FLUSH;
	assert(FALSE);
    }
    p = s_contexts + cnum;
    p->type = type;
    p->ent_sort = (long*)NULL;
    p->ent_sort_max = -1;
    p->ent_sorted_max = -1;
    p->ent_index = (long*)NULL;
    p->ent_index_max = -1;
    p->page_size = MAX_PAGE_SIZE;
    p->top_ent = -1;
    p->bot_ent = -1;
    p->refill = TRUE;
    p->ref_all = TRUE;
    p->ref_top = TRUE;
    p->ref_bot = TRUE;
    p->ref_status = -1;
    p->ref_desc = -1;
    /* next ones should be reset later */
    p->top_lines = 0;
    p->bot_lines = 0;
    p->status_cols = 0;
    p->cursor_cols = 0;
    p->desc_cols = 0;
    p->itemnum_cols = 0;
    p->ptr_page_line = 0;
    p->flags = 0;
    /* clear the page entries */
    for (i = 0; i < MAX_PAGE_SIZE; i++) {
	p->page_ents[i].entnum = 0;
	p->page_ents[i].lines = 0;
	p->page_ents[i].start_line = 0;
	p->page_ents[i].pageflags = (char)0;
    }
}

/* allocate a new context number and initialize it */
int	/* context number */
s_new_context(type)
int type;			/* context type */
{
    int i;

    /* check for deleted contexts */
    for (i = 0; i < s_num_contexts; i++)
	if (s_contexts[i].type == 0)	/* deleted context */
	    break;
    if (i < s_num_contexts) {	/* a deleted one was found */
	s_init_context(i,type);
	return i;
    }
    /* none deleted, so allocate a new one */
    i = s_num_contexts;
    i++;
    if (i == 1) {	/* none allocated before */
	s_contexts = (SCONTEXT*)safemalloc(sizeof (SCONTEXT));
    } else {
	s_contexts = (SCONTEXT*)saferealloc((char*)s_contexts,
					i * sizeof (SCONTEXT));
    }
    s_contexts[i-1].page_ents =
			(PAGE_ENT*)safemalloc(MAX_PAGE_SIZE*sizeof(PAGE_ENT));
    s_init_context(i-1,type);
    s_num_contexts++;			/* now safe to increment */
    return s_num_contexts-1;
}

/* saves the current context */
void
s_save_context()
{
    SCONTEXT *p;

    p = s_contexts + s_cur_context;

    p->type = s_cur_type;
    p->page_ents = page_ents;

    p->ent_sort = s_ent_sort;
    p->ent_sort_max = s_ent_sort_max;
    p->ent_sorted_max = s_ent_sorted_max;
    p->ent_index = s_ent_index;
    p->ent_index_max = s_ent_index_max;

    p->page_size = s_page_size;
    p->top_ent = s_top_ent;
    p->bot_ent = s_bot_ent;
    p->refill = s_refill;
    p->ref_all = s_ref_all;
    p->ref_top = s_ref_top;
    p->ref_bot = s_ref_bot;
    p->ref_status = s_ref_status;
    p->ref_desc = s_ref_desc;

    p->top_lines = s_top_lines;
    p->bot_lines = s_bot_lines;
    p->status_cols = s_status_cols;
    p->cursor_cols = s_cursor_cols;
    p->desc_cols = s_desc_cols;
    p->itemnum_cols = s_itemnum_cols;
    p->ptr_page_line = s_ptr_page_line;
    p->flags = s_flags;
}


void
s_change_context(newcontext)
int newcontext;			/* context number to activate */
{
    SCONTEXT *p;

    if (newcontext < 0 || newcontext >= s_num_contexts) {
	printf("s_change_context: bad context number %d!\n",newcontext) FLUSH;
	assert(FALSE);
    }
    s_cur_context = newcontext;
    p = s_contexts + newcontext;
    s_cur_type = p->type;
    page_ents = p->page_ents;

    s_ent_sort = p->ent_sort;
    s_ent_sort_max = p->ent_sort_max;
    s_ent_sorted_max = p->ent_sorted_max;
    s_ent_index = p->ent_index;
    s_ent_index_max = p->ent_index_max;

    s_page_size = p->page_size;
    s_top_ent = p->top_ent;
    s_bot_ent = p->bot_ent;
    s_refill = p->refill;
    s_ref_all = p->ref_all;
    s_ref_top = p->ref_top;
    s_ref_bot = p->ref_bot;
    s_ref_status = p->ref_status;
    s_ref_desc = p->ref_desc;
    /* next ones should be reset later */
    s_top_lines = p->top_lines;
    s_bot_lines = p->bot_lines;
    s_status_cols = p->status_cols;
    s_cursor_cols = p->cursor_cols;
    s_desc_cols = p->desc_cols;
    s_itemnum_cols = p->itemnum_cols;
    s_ptr_page_line = p->ptr_page_line;
    s_flags = p->flags;
}

/* implement later? */
void
s_clean_contexts()
{
}

void
s_delete_context(cnum)
int cnum;		/* context number to delete */
{
    if (cnum < 0 || cnum >= s_num_contexts) {
	printf("s_delete_context: illegal context number %d!\n",cnum) FLUSH;
	assert(FALSE);
    }
    s_order_clean();
    /* mark the context as empty */
    s_contexts[cnum].type = 0;
}
#endif /* SCAN */
