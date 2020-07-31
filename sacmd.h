/* This file Copyright 1992 by Clifford A. Adams */
/* sacmd.h
 *
 * main command loop
 */

#define SA_KILL 1
#define SA_MARK 2
#define SA_SELECT 3
#define SA_KILL_UNMARKED 4
#define SA_KILL_MARKED 5
#define SA_EXTRACT 6

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

int sa_docmd _((void));
bool sa_extract_start _((void));
void sa_art_cmd_prim _((int,long));
int sa_art_cmd _((int,int,long));
long sa_wrap_next_author _((long));
