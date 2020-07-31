/* nntpauth.h
*/ 
/* This software is copyrighted as detailed in the LICENSE file. */


/* DON'T EDIT BELOW THIS LINE OR YOUR CHANGES WILL BE LOST! */

#ifdef SUPPORT_NNTP
int nntp_handle_auth_err _((void));
#endif
#ifdef USE_GENAUTH
int nntp_auth _((char*));
#endif
