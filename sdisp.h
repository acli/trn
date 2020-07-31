/* This file Copyright 1992 by Clifford A. Adams */
/* sdisp.h
 *
 * scan display functions
 */

	/* height of screen in characters */
EXT int scr_height INIT(0);
	/* width of screen in characters */
EXT int scr_width INIT(0);

/* has the window been resized? */
EXT bool s_resized INIT(FALSE);

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

void s_goxy _((int,int));
void s_mail_and_place _((void));
void s_refresh_top _((void));
void s_refresh_bot _((void));
void s_refresh_entzone _((void));
void s_place_ptr _((void));
void s_refresh_status _((int));
void s_refresh_description _((int));
void s_ref_entry _((int,int));
void s_rub_ptr _((void));
void s_refresh _((void));
int s_initscreen _((void));
void s_ref_status_onpage _((long));
void s_resize_win _((void));
