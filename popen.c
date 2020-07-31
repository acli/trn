/* popen/pclose:
 *
 * simple MS-DOS piping scheme to imitate UNIX pipes
 */

#include "EXTERN.h"
#include "common.h"
#include <setjmp.h>
#include "util2.h"
#include "util3.h"

#ifndef	_NFILE
# define _NFILE 5			/* Number of open files */
#endif	_NFILE

#define READIT	1			/* Read pipe */
#define WRITEIT	2			/* Write pipe */

static char* prgname[_NFILE];		/* program name if write pipe */
static int pipetype[_NFILE];		/* 1=read 2=write */
static char* pipename[_NFILE];		/* pipe file name */

/*
 *------------------------------------------------------------------------
 * run: Execute command via SHELL or COMSPEC
 *------------------------------------------------------------------------
 */

static int
run(char* command)
{
    jmp_buf panic;			/* How to recover from errors */
    char* shell;			/* Command processor */
    char* s = NULL;			/* Holds the command */
    int s_is_malloced = 0;		/* True if need to free 's' */
    static char* command_com = "COMMAND.COM";
    int status;				/* Return codes */
    char* shellpath;			/* Full command processor path */
    char* bp;				/* Generic string pointer */
    static char dash_c[] = "/c";

    s = savestr(command);
    /* Determine the command processor */
    if ((shell = getenv("SHELL")) == NULL
     && (shell = getenv("COMSPEC")) == NULL)
	shell = command_com;
    strupr(shell);
    shellpath = shell;
    /* Strip off any leading backslash directories */
    shell = rindex(shellpath, '\\');
    if (shell != NULL)
	shell++;
    else
	shell = shellpath;
    /* Strip off any leading slash directories */
    bp = rindex(shell, '/');
    if (bp != NULL)
	shell = ++bp;
    if (strcmp(shell, command_com) != 0) {
	/* MKS Shell needs quoted argument */
	char* bp;
	bp = s = safemalloc(strlen(command) + 3);
	*bp++ = '\'';
	while ((*bp++ = *command++) != '\0') ;
	*(bp - 1) = '\'';
	*bp = '\0';
	s_is_malloced = 1;
    } else
	s = command;
    /* Run the program */
    status = spawnl(P_WAIT, shellpath, shell, dash_c, s, NULL);
    if (s_is_malloced)
	free(s);
    return status;
}

/* 
 *------------------------------------------------------------------------
 * uniquepipe: returns a unique file name 
 *------------------------------------------------------------------------
 */

static char*
uniquepipe(void)
{ 
    static char name[14];
    static short int num = 0;
    (void) sprintf(name, "pipe%04d.tmp", num++);
    return name;
}

/*
 *------------------------------------------------------------------------
 * resetpipe: Private routine to cancel a pipe
 *------------------------------------------------------------------------
 */
static void
resetpipe(int fd)
{
    char* bp;
    if (fd >= 0 && fd < _NFILE) {
	pipetype[fd] = 0;
	if ((bp = pipename[fd]) != NULL) {
	    (void) unlink(bp);
	    free(bp);
	    pipename[fd] = NULL;
	}
	if ((bp = prgname[fd]) != NULL) {
	    free(bp);
	    prgname[fd] = NULL;
	}
    }
}

/* 
 *------------------------------------------------------------------------
 * popen: open a pipe 
 *------------------------------------------------------------------------
 */
FILE* popen(prg, type)
char* prg;			/* The command to be run */
char* type;			/* "w" or "r" */
{ 
    FILE* p = NULL;		/* Where we open the pipe */
    int ostdin;			/* Where our stdin is now */
    int pipefd = -1;		/* fileno(p) -- for convenience */
    char* tmpfile;		/* Holds name of pipe file */
    jmp_buf panic;		/* Where to go if there's an error */
    int lineno;			/* Line number where panic happened */

    /* Get a unique pipe file name */
    tmpfile = filexp("%Y/");
    strcat(tmpfile, uniquepipe());
    if ((lineno = setjmp(panic)) != 0) {
	/* An error has occurred, so clean up */
	int E = errno;
	if (p != NULL)
	    (void) fclose(p);
	resetpipe(pipefd);
	errno = E;
	lineno = lineno;
	return NULL;
    }
    if (strcmp(type, "w") == 0) {
	/* for write style pipe, pclose handles program execution */
	if ((p = fopen(tmpfile, "w")) != NULL) {
	    pipefd = fileno(p);
	    pipetype[pipefd] = WRITEIT;
	    pipename[pipefd] = savestr(tmpfile);
	    prgname[pipefd]  = savestr(prg);
	}
    } else if (strcmp(type, "r") == 0) {
	/* read pipe must create tmp file, set up stdout to point to the temp
	 * file, and run the program.  note that if the pipe file cannot be
	 * opened, it'll return a condition indicating pipe failure, which is
	 * fine. */
	if ((p = fopen(tmpfile, "w")) != NULL) {
	    int ostdout;
	    pipefd = fileno(p);
	    pipetype[pipefd]= READIT;
	    pipename[pipefd] = savestr(tmpfile);
	    /* Redirect stdin for the new command */
	    ostdout = dup(fileno(stdout));
	    if (dup2(fileno(stdout), pipefd) < 0) {
		int E = errno;
		(void) dup2(fileno(stdout), ostdout);
		errno = E;
		longjmp(panic, __LINE__);
	    }
	    if (run(prg) != 0)
		longjmp(panic, __LINE__);
	    if (dup2(fileno(stdout), ostdout) < 0)
		longjmp(panic, __LINE__);
	    if (fclose(p) < 0)
		longjmp(panic, __LINE__);
	    if ((p = fopen(tmpfile, "r")) == NULL)
		longjmp(panic, __LINE__);
	}
    } else {
	/* screwy call or unsupported type */
	errno = EINVFNC;
	longjmp(panic, __LINE__);
    }
    return p;
}

/* close a pipe */
int
pclose(p)
FILE* p;
{
    int pipefd = -1;		/* Fildes where pipe is opened */
    int ostdout;		/* Where our stdout points now */
    int ostdin;			/* Where our stdin points now */
    jmp_buf panic;		/* Context to return to if error */
    int lineno;			/* Line number where panic happened */
    if ((lineno = setjmp(panic)) != 0) {
	/* An error has occurred, so clean up and return */
	int E = errno;
	if (p != NULL)
	    (void) fclose(p);
	resetpipe(pipefd);
	errno = E;
	lineno = lineno;
	return -1;
    }
    pipefd = fileno(p);
    if (fclose(p) < 0)
	longjmp(panic, __LINE__);
    switch (pipetype[pipefd]) {
    case WRITEIT:
	/* open the temp file again as read, redirect stdin from that
	 * file, run the program, then clean up. */
	if ((p = fopen(pipename[pipefd],"r")) == NULL) 
	    longjmp(panic, __LINE__);
	ostdin = dup(fileno(stdin));
	if (dup2(fileno(stdin), fileno(p)) < 0)
	    longjmp(panic, __LINE__);
	if (run(prgname[pipefd]) != 0)
	    longjmp(panic, __LINE__);
	if (dup2(fileno(stdin), ostdin) < 0)
	    longjmp(panic, __LINE__);
	if (fclose(p) < 0)
	    longjmp(panic, __LINE__);
	resetpipe(pipefd);
	break;
    case READIT:
	/* close the temp file and remove it */
	resetpipe(pipefd);
	break;
    default:
	errno = EINVFNC;
	longjmp(panic, __LINE__);
	/*NOTREACHED*/
    }
    return 0;
}
