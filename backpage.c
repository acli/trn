/* backpage.c
 */
/* This software is copyrighted as detailed in the LICENSE file. */


#include "EXTERN.h"
#include "common.h"
#include "intrp.h"
#include "util2.h"
#include "final.h"
#include "INTERN.h"
#include "backpage.h"

ART_LINE maxindx = -1;

void
backpage_init()
{
    char* varyname;
    
    varyname = filexp(VARYNAME);
    close(creat(varyname,0600));
    varyfd = open(varyname,2);
    UNLINK(varyname);
    if (varyfd < 0) {
	printf(cantopen,varyname) FLUSH;
	sig_catcher(0);
    }
    
}

/* virtual array read */

ART_POS
vrdary(indx)
ART_LINE indx;
{
    int subindx;
    long offset;

#ifdef DEBUG
    if (indx > maxindx) {
	printf("vrdary(%ld) > %ld\n",(long)indx, (long)maxindx) FLUSH;
	return 0;
    }
#endif
    if (indx < 0)
	return 0;
    subindx = indx % VARYSIZE;
    offset = (indx - subindx) * sizeof(varybuf[0]);
    if (offset != oldoffset) {
	if (oldoffset >= 0) {
#ifndef lint
	    (void)lseek(varyfd,oldoffset,0);
	    write(varyfd, (char*)varybuf,sizeof(varybuf));
#endif /* lint */
	}
#ifndef lint
	(void)lseek(varyfd,offset,0);
	read(varyfd,(char*)varybuf,sizeof(varybuf));
#endif /* lint */
	oldoffset = offset;
    }
    return varybuf[subindx];
}

/* write to virtual array */

void
vwtary(indx,newvalue)
ART_LINE indx;
ART_POS newvalue;
{
    int subindx;
    long offset;

#ifdef DEBUG
    if (indx < 0)
	printf("vwtary(%ld)\n",(long)indx) FLUSH;
    if (!indx)
	maxindx = 0;
    if (indx > maxindx) {
	if (indx != maxindx + 1)
	    printf("indx skipped %d-%d\n",maxindx+1,indx-1) FLUSH;
	maxindx = indx;
    }
#endif
    subindx = indx % VARYSIZE;
    offset = (indx - subindx) * sizeof(varybuf[0]);
    if (offset != oldoffset) {
	if (oldoffset >= 0) {
#ifndef lint
	    (void)lseek(varyfd,oldoffset,0);
	    write(varyfd,(char*)varybuf,sizeof(varybuf));
#endif /* lint */
	}
#ifndef lint
	(void)lseek(varyfd,offset,0);
	read(varyfd,(char*)varybuf,sizeof(varybuf));
#endif /* lint */
	oldoffset = offset;
    }
    varybuf[subindx] = newvalue;
}

