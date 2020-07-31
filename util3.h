/* util3.h
 */
/* This software is copyrighted as detailed in the LICENSE file. */


extern char* homedir;

/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

int doshell _((char*,char*));
void finalize _((int))
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR >= 5)
  __attribute__((noreturn))
#endif
  ;
#ifndef USE_DEBUGGING_MALLOC
char* safemalloc _((MEM_SIZE));
char* saferealloc _((char*,MEM_SIZE));
#endif
char* dointerp _((char*,int,char*,char*,char*));
#ifdef SUPPORT_NNTP
int nntp_handle_nested_lists _((void));
char* get_auth_user _((void));
char* get_auth_pass _((void));
#endif
#if defined(USE_GENAUTH) && defined(SUPPORT_NNTP)
char* get_auth_command _((void));
#endif
