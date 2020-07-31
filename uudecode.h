/* uudecode.h
 */
/* This software is copyrighted as detailed in the LICENSE file. */

/* Length of a normal uuencoded line, including newline */
#define UULENGTH 62

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

int uue_prescan _((char*,char**,int*,int*));
int uudecode _((FILE*,int));
