/* respond.h
 */
/* This software is copyrighted as detailed in the LICENSE file. */


EXT char* savedest INIT(NULL);		/* value of %b */
EXT char* extractdest INIT(NULL);	/* value of %E */
EXT char* extractprog INIT(NULL);	/* value of %e */
EXT ART_POS savefrom INIT(0);		/* value of %B */

#define SAVE_ABORT 0
#define SAVE_DONE 1

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

void respond_init _((void));
int save_article _((void));
int view_article _((void));
int cancel_article _((void));
int supersede_article _((void));
void reply _((void));
void forward _((void));
void followup _((void));
int invoke _((char*,char*));
