/* This file is Copyright 1993 by Clifford A. Adams */
/* scmd.h
 *
 * Scan command interpreter/router
 */

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

void s_go_bot _((void));
int s_finish_cmd _((char*));
int s_cmdloop _((void));
void s_lookahead _((void));
int s_docmd _((void));
bool s_match_description _((long));
long s_forward_search _((long));
long s_backward_search _((long));
void s_search _((void));
void s_jumpnum _((char_int));
