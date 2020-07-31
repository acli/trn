/* rthread.h
*/
/* This software is copyrighted as detailed in the LICENSE file. */


EXT ART_NUM obj_count INIT(0);
EXT int subject_count INIT(0);
EXT bool output_chase_phrase;

EXT HASHTABLE* msgid_hash INIT(NULL);

/* Values to pass to count_subjects() */
#define CS_RETAIN      0
#define CS_NORM        1
#define CS_RESELECT    2
#define CS_UNSELECT    3
#define CS_UNSEL_STORE 4

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

void thread_init _((void));
void thread_open _((void));
void thread_grow _((void));
void thread_close _((void));
void top_article _((void));
ARTICLE* first_art _((SUBJECT*));
ARTICLE* last_art _((SUBJECT*));
void inc_art _((bool_int,bool_int));
void dec_art _((bool_int,bool_int));
ARTICLE* bump_art _((ARTICLE*));
ARTICLE* next_art _((ARTICLE*));
ARTICLE* prev_art _((ARTICLE*));
bool next_art_with_subj _((void));
bool prev_art_with_subj _((void));
SUBJECT* next_subj _((SUBJECT*,int));
SUBJECT* prev_subj _((SUBJECT*,int));
void select_article _((ARTICLE*,int));
void select_arts_subject _((ARTICLE*,int));
void select_subject _((SUBJECT*,int));
void select_arts_thread _((ARTICLE*,int));
void select_thread _((ARTICLE*,int));
void select_subthread _((ARTICLE*,int));
void deselect_article _((ARTICLE*,int));
void deselect_arts_subject _((ARTICLE*));
void deselect_subject _((SUBJECT*));
void deselect_arts_thread _((ARTICLE*));
void deselect_thread _((ARTICLE*));
void deselect_all _((void));
void kill_arts_subject _((ARTICLE*,int));
void kill_subject _((SUBJECT*,int));
void kill_arts_thread _((ARTICLE*,int));
void kill_thread _((ARTICLE*,int));
void kill_subthread _((ARTICLE*,int));
void unkill_subject _((SUBJECT*));
void unkill_thread _((ARTICLE*));
void unkill_subthread _((ARTICLE*));
void clear_subject _((SUBJECT*));
void clear_thread _((ARTICLE*));
void clear_subthread _((ARTICLE*));
ARTICLE* subj_art _((SUBJECT*));
void visit_next_thread _((void));
void visit_prev_thread _((void));
bool find_parent _((bool_int));
bool find_leaf _((bool_int));
bool find_next_sib _((void));
bool find_prev_sib _((void));
void count_subjects _((int));
void sort_subjects _((void));
void sort_articles _((void));
