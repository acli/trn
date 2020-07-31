/* This file Copyright 1992, 1993 by Clifford A. Adams */
/* spage.h
 *
 * functions to manage a page of entries.
 */

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

bool s_fillpage_backward _((long));
bool s_fillpage_forward _((long));
bool s_refillpage _((void));
int s_fillpage _((void));
void s_cleanpage _((void));
void s_go_top_page _((void));
void s_go_bot_page _((void));
bool s_go_top_ents _((void));
bool s_go_bot_ents _((void));
void s_go_next_page _((void));
void s_go_prev_page _((void));
