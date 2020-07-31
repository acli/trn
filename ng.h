/* ng.h
 */
/* This software is copyrighted as detailed in the LICENSE file. */


EXT ART_NUM art INIT(0);	/* current or prospective article # */

EXT ART_NUM recent_art;		/* previous article # for '-' command */
EXT ART_NUM curr_art;		/* current article # */
EXT ARTICLE* recent_artp INIT(0);/* article_ptr equivilents */
EXT ARTICLE* curr_artp INIT(0);
EXT ARTICLE* artp INIT(0);	/* the article ptr we use when art is 0 */

EXT int checkcount INIT(0);	/* how many articles have we read */
				/*   in the current newsgroup since */
				/*   the last checkpoint? */
EXT int docheckwhen INIT(20);	/* how often to do checkpoint */

EXT char* subjline INIT(NULL);	/* what format to use for '=' */
#ifdef MAILCALL
EXT int mailcount INIT(0);	/* check for mail when 0 mod 10 */
#endif
EXT char* mailcall INIT(nullstr);
EXT bool forcelast INIT(FALSE);	/* ought we show "End of newsgroup"? */
EXT bool forcegrow INIT(FALSE);	/* do we want to recalculate size */
				/*   of newsgroup, e.g. after posting? */

#define NG_ERROR -1
#define NG_NORM 0
#define NG_ASK 1
#define NG_MINUS 2
#define NG_SELPRIOR 3
#define NG_SELNEXT 4
#define NG_NOSERVER 5
#define NG_NEXT 6
#define NG_GO_ARTICLE 7

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

void ng_init _((void));
int do_newsgroup _((char*));
int art_switch _((void));
#ifdef MAILCALL
void setmail _((bool_int));
#endif
void setdfltcmd _((void));
char ask_catchup _((void));
bool output_subject _((char*,int));
char ask_memorize _((char_int));
