/* This file Copyright 1992 by Clifford A. Adams */
/* score.h
 *
 */

/* RETHINK LOWSCORE: (especially for 16-bit scores?) */
/* score given to unavailable articles */
#define LOWSCORE (-999999)

/* specific scoreflag meanings:  (note: bad placement, but where else?) */
/* author has a score (match on FROM: line) */
#define SFLAG_AUTHOR 1
/* if TRUE, the article has been scored */
#define SFLAG_SCORED 16
#define SCORED(a) (article_ptr(a)->scoreflags & SFLAG_SCORED)

EXT bool kill_thresh_active INIT(FALSE);
EXT int kill_thresh INIT(LOWSCORE);   /* KILL articles at or below this score */

EXT ART_NUM sc_fill_max;	   /* maximum art# scored by fill-routine */
EXT bool sc_fill_read INIT(FALSE); /* TRUE if also scoring read arts... */

/* has score been initialized (are we "in" scoring?) */
EXT bool sc_initialized INIT(FALSE);

/* are we currently scoring an article (prevents loops) */
EXT bool sc_scoring INIT(FALSE);

/* changes order of sorting (artnum comparison) when scores are equal */
EXT bool score_newfirst INIT(FALSE);

/* if nice background available, use it */
EXT bool sc_mode_nicebg INIT(TRUE);

/* If true, save the scores for this group on exit. */
EXT bool sc_savescores INIT(FALSE);

/* If true, delay initialization of scoring until explicitly required */
EXT bool sc_delay INIT(FALSE);

EXT bool sc_rescoring INIT(FALSE);	/* are we rescoring now? */

EXT bool sc_do_spin INIT(FALSE);	/* actually do the score spinner */

EXT bool sc_sf_delay INIT(FALSE);	/* if TRUE, delay loading rule files */
EXT bool sc_sf_force_init INIT(FALSE);	/* If true, always sf_init() */

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

void sc_init _((bool_int));
void sc_cleanup _((void));
void sc_set_score _((ART_NUM,int));
void sc_score_art_basic _((ART_NUM));
int sc_score_art _((ART_NUM,bool_int));
void sc_fill_scorelist _((ART_NUM,ART_NUM));
void sc_lookahead _((bool_int,bool_int));
int sc_percent_scored _((void));
void sc_rescore_arts _((void));
void sc_append _((char*));
void sc_rescore _((void));
void sc_score_cmd _((char*));
void sc_kill_threshold _((int));
