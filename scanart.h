/* This file Copyright 1992 by Clifford A. Adams */
/* scanart.h
 *
 * Interface to rest of [t]rn
 */

/* return codes for sa_main */
	/* read article pointed to by art (always) */
#define SA_READ (-7)
	/* quit, and return to previous selector (backtick) */
#define SA_QUIT_SEL (-6)
	/* go to and enter prior newsgroup */
#define SA_PRIOR (-5)
	/* go to and enter next newsgroup */
#define SA_NEXT (-4)
	/* Fake a command (buf and art already set up) */
#define SA_FAKE (-3)
	/* error, quit out one level */
#define SA_ERR (-2)
	/* quit out one level and clean up... */
#define SA_QUIT (-1)
	/* do the normal thing (usually read article pointed to by art) */
#define SA_NORM 0

/* per-entry data */
struct sa_entrydata {
    ART_NUM artnum;
    long subj_thread_num;
    char sa_flags;		/* status bitmap (marked, select, etc...) */
};

EXT SA_ENTRYDATA* sa_ents INIT((SA_ENTRYDATA*)0);
EXT int sa_num_ents INIT(0);
EXT int sa_ents_alloc INIT(0);

EXT bool sa_initialized INIT(FALSE);	/* Have we initialized? */
EXT bool sa_never_initialized INIT(TRUE); /* Have we ever initialized? */

/* note: sa_in should be checked for returning to SA */
EXT bool sa_in INIT(FALSE);		/* Are we "in" SA? */

EXT bool sa_go INIT(FALSE);		/* go to sa.  Do not pass GO (:-) */
EXT bool sa_go_explicit INIT(FALSE);	/* want to bypass read-next-marked */

EXT bool sa_context_init INIT(FALSE);	/* has context been initialized? */

/* used to pass an article number to read soon */
EXT ART_NUM sa_art INIT(0);

/* reimplement later */
/* select threads from TRN thread selector */
EXT bool sa_do_selthreads INIT(FALSE);

/* TRUE if read articles are eligible */
/* in scanart.h for world-visibilty */
EXT bool sa_mode_read_elig INIT(FALSE);

/* Options */
/* Display order variable:
 *
 * 1: Arrival order
 * 2: Descending score
 */
#ifdef SCORE
EXT int sa_mode_order INIT(2);
#else
EXT int sa_mode_order INIT(1);
#endif

/* if TRUE, don't move the cursor after marking or selecting articles */
EXT bool sa_mark_stay INIT(FALSE);

/* if TRUE, re-"fold" after an un-zoom operation. */
/* This flag is useful for very slow terminals */
EXT bool sa_unzoomrefold INIT(FALSE);

/* TRUE if in "fold" mode */
EXT bool sa_mode_fold INIT(FALSE);

/* Follow threads by default? */
EXT bool sa_follow INIT(TRUE);

/* Options: what to display */
EXT bool sa_mode_desc_artnum INIT(FALSE);	/* show art#s */
EXT bool sa_mode_desc_author INIT(TRUE);	/* show author */
#ifdef SCORE
EXT bool sa_mode_desc_score INIT(TRUE);		/* show score */
#else
EXT bool sa_mode_desc_score INIT(FALSE);	/* show score */
#endif
/* flags to determine whether to display various things */
EXT bool sa_mode_desc_threadcount INIT(FALSE);
EXT bool sa_mode_desc_subject INIT(TRUE);
EXT bool sa_mode_desc_summary INIT(FALSE);
EXT bool sa_mode_desc_keyw INIT(FALSE);

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

int sa_main _((void));
void sa_grow _((ART_NUM,ART_NUM));
void sa_cleanup _((void));
