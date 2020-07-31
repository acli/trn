/* rt-util.h
*/
/* This software is copyrighted as detailed in the LICENSE file. */


EXT char spin_char INIT(' ');	/* char to put back when we're done spinning */
EXT long spin_estimate;		/* best guess of how much work there is */
EXT long spin_todo;		/* the max word to do (might decrease) */
EXT int  spin_count;		/* counter for when to spin */
EXT int  spin_marks INIT(25);	/* how many bargraph marks we want */

#define SPIN_OFF	0
#define SPIN_POP	1
#define SPIN_FOREGROUND	2
#define SPIN_BACKGROUND 3
#define SPIN_BARGRAPH	4

EXT bool performed_article_loop;

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

char* extract_name _((char*));
char* compress_name _((char*,int));
char* compress_address _((char*,int));
char* compress_from _((char*,int));
char* compress_date _((ARTICLE*,int));
bool subject_has_Re _((char*,char**));
char* compress_subj _((ARTICLE*,int));
void setspin _((int));
void spin _((int));
bool inbackground _((void));
void perform_status_init _((long));
void perform_status _((long,int));
int perform_status_end _((long,char*));
