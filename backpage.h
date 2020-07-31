/* backpage.h
 */
/* This software is copyrighted as detailed in the LICENSE file. */


/* things for doing the 'back page' command */

EXT int varyfd INIT(0);			/* virtual array file for storing  */
					/* file offsets */
EXT ART_POS varybuf[VARYSIZE];		/* current window onto virtual array */

EXT long oldoffset INIT(-1);		/* offset to block currently in window */

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

void backpage_init _((void));
ART_POS vrdary _((ART_LINE));
void vwtary _((ART_LINE,ART_POS));
