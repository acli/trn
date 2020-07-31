/* util3.c
 */
/* This software is copyrighted as detailed in the LICENSE file. */


#include <stdio.h>
#include "config.h"
#include "config2.h"
#include "typedef.h"
#include "EXTERN.h"
#include "config.h"
#include "config2.h"
#include "nntpclient.h"
#include "util2.h"
#include "INTERN.h"
#include "util3.h"

char* sh = NULL;
bool export_nntp_fds = FALSE;

#ifdef SUPPORT_NNTP
char* nntp_password;
#endif

int
doshell(sh,cmd)
char* sh;
char* cmd;
{
    return system(cmd);
}

void
finalize(num)
int num;
{
#ifdef SUPPORT_NNTP
    nntp_close(TRUE);
#endif
    exit(num);
}

static char nomem[] = "trn: out of memory!\n";

/* paranoid version of malloc */

#ifndef USE_DEBUGGING_MALLOC
char*
safemalloc(size)
MEM_SIZE size;
{
    char* ptr;

    ptr = malloc(size ? size : (MEM_SIZE)1);
    if (!ptr) {
	fputs(nomem,stdout);
	finalize(1);
    }
    return ptr;
}
#endif

/* paranoid version of realloc.  If where is NULL, call malloc */

#ifndef USE_DEBUGGING_MALLOC
char*
saferealloc(where,size)
char* where;
MEM_SIZE size;
{
    char* ptr;

    ptr = realloc(where, size ? size : (MEM_SIZE)1);
    if (!ptr) {
	fputs(nomem,stdout);
	finalize(1);
    }
    return ptr;
}
#endif

char*
dointerp(dest, destsize, pattern, stoppers, cmd)
char* dest;
int destsize;
char* pattern;
char* stoppers;
char* cmd;
{
    extern char* dotdir;
    if (*pattern == '%' && pattern[1] == '.') {
	int len = strlen(dotdir);
	safecpy(dest, dotdir, destsize);
	if (len < destsize)
	    safecpy(dest+len, pattern+2, destsize - len);
    }
    else
	safecpy(dest, pattern, destsize);
    return stoppers; /* This is wrong on purpose */
}

#ifdef SUPPORT_NNTP
int
nntp_handle_nested_lists()
{
    fputs("Programming error! Nested NNTP calls detected.\n",stderr);
    return -1;
}
#endif

#ifdef SUPPORT_NNTP
char*
get_auth_user()
{
    extern char* nntp_auth_file;
    return read_auth_file(nntp_auth_file, &nntp_password);
}
#endif

#ifdef SUPPORT_NNTP
char*
get_auth_pass()
{
    return nntp_password;
}
#endif

#if defined(USE_GENAUTH) && defined(SUPPORT_NNTP)
char*
get_auth_command()
{
    return NULL;
}
#endif
