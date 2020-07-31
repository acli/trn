/* nntpauth.c
*/
/* This software is copyrighted as detailed in the LICENSE file. */


#include "EXTERN.h"
#include "common.h"
#include "list.h"
#include "hash.h"
#include "ngdata.h"
#include "nntpclient.h"
#include "nntp.h"
#include "final.h"
#include "util.h"
#include "env.h"
#include "INTERN.h"
#include "nntpauth.h"

#ifdef SUPPORT_NNTP

int
nntp_handle_auth_err()
{
    char last_command_save[NNTP_STRLEN];
#ifdef USE_GENAUTH
    char* auth_command;
#endif

    /* save previous command */
    strcpy(last_command_save, last_command);

#ifdef USE_GENAUTH
    if ((auth_command = get_auth_command()) != NULL) {
	/* issue authentication request */
	sprintf(ser_line, "AUTHINFO GENERIC %s", auth_command);
	if (nntp_command(ser_line) <= 0
	 || nntp_auth(auth_command) <= 0)
	    return -2;
    }
    else
#endif
    {
	char* auth_user = get_auth_user();
	char* auth_pass = get_auth_pass();
	if (!auth_user || !auth_pass)
	    return -2;
	sprintf(ser_line, "AUTHINFO USER %s", auth_user);
	if (nntp_command(ser_line) <= 0 || nntp_check() <= 0)
	    return -2;
	sprintf(ser_line, "AUTHINFO PASS %s", auth_pass);
	if (nntp_command(ser_line) <= 0 || nntp_check() <= 0)
	    return -2;
    }

    if (nntp_command(last_command_save) <= 0)
	return -2;

    return 1;
}

#ifdef USE_GENAUTH
int
nntp_auth(authc)
char* authc;
{
    int ret;

    if (nntplink.cookiefd == 0) {
	FILE* fp = tmpfile();
	if (fp)
	    nntplink.cookiefd = fileno(fp);
    }

#if 0
    /*termlib_reset();*/
    resetty();		/* restore tty state */
#endif
    export_nntp_fds = TRUE;
    ret = doshell(sh,authc);
    export_nntp_fds = FALSE;
#if 0
    noecho();		/* revert to cbreaking */
    crmode();
    /*termlib_init();*/
#endif
    if (ret) {
	strcpy(ser_line, "502 Authentication failed");
	return -1;
    }
    strcpy(ser_line, "281 Ok");
    return 1;
}
#endif /* USE_GENAUTH */

#endif /* SUPPORT_NNTP */
