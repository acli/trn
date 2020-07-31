/* trn.h
 */
/* This software is copyrighted as detailed in the LICENSE file. */


EXT char* ngname INIT(NULL);	/* name of current newsgroup */
EXT int ngnlen INIT(0);		/* current malloced size of ngname */
EXT int ngname_len;		/* length of current ngname */
EXT char* ngdir INIT(NULL);	/* same thing in directory name form */
EXT int ngdlen INIT(0);		/* current malloced size of ngdir */

#define ING_NORM	0
#define ING_ASK		1
#define ING_INPUT	2
#define ING_ERASE	3
#define ING_QUIT	4
#define ING_ERROR	5
#define ING_SPECIAL	6
#define ING_BREAK	7
#define ING_RESTART	8
#define ING_NOSERVER	9
#define ING_DISPLAY	10
#define ING_MESSAGE	11

EXT int ing_state;

#define INGS_CLEAN	0
#define INGS_DIRTY	1

EXT bool  write_less INIT(FALSE);	/* write .newsrc less often */

EXT char* auto_start_cmd INIT(NULL);	/* command to auto-start with */
EXT bool  auto_started INIT(FALSE);	/* have we auto-started? */

EXT bool  is_strn INIT(FALSE);		/* Is this "strn", or trn/rn? */

EXT char patchlevel[] INIT(PATCHLEVEL);

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

void trn_init _((void));
int main _((int,char**));
void do_multirc _((void));
int input_newsgroup _((void));
#ifdef SUPPORT_NNTP
void check_active_refetch _((bool_int));
#endif
void trn_version _((void));
void set_ngname _((char*));
char* getngdir _((char*));
