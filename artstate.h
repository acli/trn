/* artstate.h
 */
/* This software is copyrighted as detailed in the LICENSE file. */


EXT bool reread INIT(FALSE);		/* consider current art temporarily */
					/* unread? */

EXT bool do_fseek INIT(FALSE);	/* should we back up in article file? */

EXT bool oldsubject INIT(FALSE);	/* not 1st art in subject thread */
EXT ART_LINE topline INIT(-1);		/* top line of current screen */
EXT bool do_hiding INIT(TRUE);		/* hide header lines with -h? */
EXT bool is_mime INIT(FALSE);		/* process mime in an article? */
EXT bool multimedia_mime INIT(FALSE);	/* images/audio to see/hear? */
EXT bool rotate INIT(FALSE);		/* has rotation been requested? */
EXT char* prompt;			/* pointer to current prompt */

EXT char* firstline INIT(NULL);		/* special first line? */
#ifdef CUSTOMLINES
EXT char* hideline INIT(NULL);		/* custom line hiding? */
EXT char* pagestop INIT(NULL);		/* custom page terminator? */
EXT COMPEX hide_compex;
EXT COMPEX page_compex;
#endif
