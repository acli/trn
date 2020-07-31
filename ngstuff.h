/* ngstuff.h
 */
/* This software is copyrighted as detailed in the LICENSE file. */


#define NN_NORM 0
#define NN_INP 1
#define NN_REREAD 2
#define NN_ASK 3

EXT bool one_command INIT(FALSE);	/* no ':' processing in perform() */

/* CAA: given the new and complex universal/help possibilities,
 *      the following interlock variable may save some trouble.
 *      (if TRUE, we are currently processing options)
 */
EXT bool option_sel_ilock INIT(FALSE);

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

void ngstuff_init _((void));
int escapade _((void));
int switcheroo _((void));
int numnum _((void));
int thread_perform _((void));
int perform _((char*,int));
int ngsel_perform _((void));
int ng_perform _((char*,int));
int addgrp_sel_perform _((void));
int addgrp_perform _((ADDGROUP*,char*,int));
