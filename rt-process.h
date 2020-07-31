/* rt-process.h
*/
/* This software is copyrighted as detailed in the LICENSE file. */


/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

ARTICLE* allocate_article _((ART_NUM));
int msgid_cmp _((char*,int,HASHDATUM));
bool valid_article _((ARTICLE*));
ARTICLE* get_article _((char*));
void thread_article _((ARTICLE*,char*));
void rover_thread _((ARTICLE*,char*));
void link_child _((ARTICLE*));
void merge_threads _((SUBJECT*,SUBJECT*));
