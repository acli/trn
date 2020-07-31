/* last.h
 */
/* This software is copyrighted as detailed in the LICENSE file. */


EXT char* lastngname INIT(NULL);	/* last newsgroup read */
EXT long lasttime INIT(0);		/* time last we ran */
EXT long lastactsiz INIT(0);		/* last known size of active file */
EXT long lastnewtime INIT(0);		/* time of last newgroup request */
EXT long lastextranum INIT(0);

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

void last_init _((void));
void readlast _((void));
void writelast _((void));
