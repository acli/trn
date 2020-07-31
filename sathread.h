/* This file Copyright 1992,1995 by Clifford A. Adams */
/* sathread.h
 *
 */

/* this define will save a *lot* of function calls. */
#define sa_subj_thread(e) \
 (sa_ents[e].subj_thread_num? sa_ents[e].subj_thread_num : \
 sa_get_subj_thread(e))

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

void sa_init_threads _((void));
long sa_get_subj_thread _((long));
int sa_subj_thread_count _((long));
long sa_subj_thread_prev _((long));
long sa_subj_thread_next _((long));
