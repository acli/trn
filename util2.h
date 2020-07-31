/* util2.h
 */
/* This software is copyrighted as detailed in the LICENSE file. */


/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

char* savestr _((char*));
char* safecpy _((char*,char*,int));
char* cpytill _((char*,char*,int));
char* filexp _((char*));
char* instr _((char*,char*,bool_int));
#ifndef HAS_STRCASECMP
int trn_casecmp _((char*,char*));
int trn_ncasecmp _((char*,char*,int));
#endif
#ifdef SUPPORT_NNTP
char* read_auth_file _((char*,char**));
#endif
#ifdef MSDOS
int ChDir _((char*));
int getuid _((void));
#endif
