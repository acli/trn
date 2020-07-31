/* This file Copyright 1993 by Clifford A. Adams */
/* scoresave.h
 *
 */

EXT long sc_save_new INIT(0);	/* new articles (unloaded) */

EXT int sc_loaded_count INIT(0);	/* how many articles were loaded? */

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

void sc_sv_add _((char*));
void sc_sv_delgroup _((char*));
void sc_sv_getfile _((void));
void sc_sv_savefile _((void));
ART_NUM sc_sv_use_line _((char*,ART_NUM));
ART_NUM sc_sv_make_line _((ART_NUM));
void sc_load_scores _((void));
void sc_save_scores _((void));
