/* This file Copyright 1993 by Clifford A. Adams */
/* sorder.h
 *
 * scan ordering
 */

/* If true, resort next time order is considered */
EXT bool s_order_changed INIT(FALSE);

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

int s_compare _((long,long));
void s_sort_basic _((void));
void s_sort _((void));
void s_order_clean _((void));
void s_order_add _((long));
long s_prev _((long));
long s_next _((long));
long s_prev_elig _((long));
long s_next_elig _((long));
long s_first _((void));
long s_last _((void));
