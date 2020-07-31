/* last.c
 */
/* This software is copyrighted as detailed in the LICENSE file. */


#include "EXTERN.h"
#include "common.h"
#include "trn.h"
#include "list.h"
#include "util.h"
#include "util2.h"
#include "intrp.h"
#include "init.h"
#include "INTERN.h"
#include "last.h"

static char* lastfile = NULL;	/* path name of .rnlast file */
static long starttime;

void
last_init()
{
    lastfile = savestr(filexp(LASTNAME));

    starttime = (long)time((time_t*)NULL);
    readlast();
}

void
readlast()
{
    if ((tmpfp = fopen(lastfile,"r")) != NULL) {
	if (fgets(buf,sizeof buf,tmpfp) != NULL) {
	    long old_last = lasttime;
	    buf[strlen(buf)-1] = '\0';
	    if (*buf) {
		safefree0(lastngname);
		lastngname = savestr(buf);
	    }
	    fscanf(tmpfp,"%ld %ld %ld %ld",&lasttime,&lastactsiz,
					   &lastnewtime,&lastextranum);
	    if (!lastnewtime)
		lastnewtime = starttime;
	    if (old_last > lasttime)
		lasttime = old_last;
	}
	fclose(tmpfp);
    }
}

/* Put out certain values for next run of trn */

void
writelast()
{
    sprintf(buf,"%s.%ld", lastfile, our_pid);
    if ((tmpfp = fopen(buf,"w")) != NULL) {
	if (lasttime < starttime)
	    lasttime = starttime;
	fprintf(tmpfp,"%s\n%ld\n%ld\n%ld\n%ld\n",
		ngname? ngname : nullstr,lasttime,
		lastactsiz,lastnewtime,lastextranum);
	fclose(tmpfp);
	UNLINK(lastfile);
	RENAME(buf,lastfile);
    }
    else {
	printf(cantcreate,buf) FLUSH;
	/*termdown(1);*/
    }
}
