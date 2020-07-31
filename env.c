/* env.c
 */
/* This software is copyrighted as detailed in the LICENSE file. */


#include "EXTERN.h"
#include "common.h"
#include "init.h"
#include "final.h"
#include "util.h"
#include "util2.h"
#include "INTERN.h"
#include "env.h"
#include "env.ih"

#ifdef HAS_RES_INIT
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
#endif

bool
env_init(tcbuf, lax)
char* tcbuf;
bool_int lax;
{
    bool fully_successful = TRUE;

    if ((homedir = getenv("HOME")) == NULL)
	homedir = getenv("LOGDIR");

    if ((tmpdir = getenv("TMPDIR")) == NULL)
	tmpdir = getval("TMP","/tmp");

    /* try to set loginName */
    if (lax) {
	loginName = getenv("USER");
	if (!loginName)
	    loginName = getenv("LOGNAME");
    }
#ifndef MSDOS
    if (!lax || !loginName) {
	loginName = getlogin();
	if (loginName)
	    loginName = savestr(loginName);
    }
#endif

    /* Set realName, and maybe set loginName and homedir (if NULL). */
    if (!setusername(tcbuf)) {
	if (!loginName)
	    loginName = nullstr;
	if (!realName)
	    realName = nullstr;
	fully_successful = FALSE;
    }
    env_init2();

    /* set phostname to the hostname of our local machine */
    if (!setphostname(tcbuf))
	fully_successful = FALSE;

#ifdef SUPPORT_NNTP
    {
	char* cp = getval("NETSPEED","5");
	if (*cp == 'f')
	    netspeed = 10;
	else if (*cp == 's')
	    netspeed = 1;
	else {
	    netspeed = atoi(cp);
	    if (netspeed < 1)
		netspeed = 1;
	}
    }
#endif

    return fully_successful;
}

static void
env_init2()
{
    if (dotdir)		/* Avoid running multiple times. */
	return;
    if (!homedir)
	homedir = "/";
    dotdir = getval("DOTDIR",homedir);
    trndir = savestr(filexp(getval("TRNDIR",TRNDIR)));
    lib = savestr(filexp(NEWSLIB));
    rnlib = savestr(filexp(PRIVLIB));
}

/* Set loginName to the user's login name and realName to the user's
** real name.
*/
bool
setusername(tmpbuf)
char* tmpbuf;
{
    char* s;
    char* c;

#ifdef HAS_GETPWENT
    struct passwd* pwd;

    if (loginName == NULL)
	pwd = getpwuid(getuid());
    else
	pwd = getpwnam(loginName);
    if (!pwd)
	return 0;
    if (!loginName)
	loginName = savestr(pwd->pw_name);
    if (!homedir)
	homedir = savestr(pwd->pw_dir);
    s = pwd->pw_gecos;
#else /* !HAS_GETPWENT */
    int i;

    if (getpw(getuid(), tmpbuf+1) != 0)
	return 0;
    if (!loginName) {
	cpytill(buf,tmpbuf+1,':');
	loginName = savestr(buf);
    }
    for (s = tmpbuf, i = GCOSFIELD-1; i; i--) {
	if (s)
	    s = index(s+1,':');
    }
    if (!s)
	return 0;
    s = cpytill(tmpbuf,s+1,':');
    if (!homedir) {
	cpytill(buf,s+1,':');
	homedir = savestr(buf);
    }
    s = tmpbuf;
#endif /* !HAS_GETPWENT */
#ifdef PASSNAMES
#ifdef BERKNAMES
#ifdef BERKJUNK
    while (*s && !isalnum(*s) && *s != '&') s++;
#endif
    if ((c = index(s, ',')) != NULL)
	*c = '\0';
    if ((c = index(s, ';')) != NULL)
	*c = '\0';
    s = cpytill(buf,s,'&');
    if (*s == '&') {			/* whoever thought this one up was */
	c = buf + strlen(buf);		/* in the middle of the night */
	strcat(c,loginName);		/* before the morning after */
	strcat(c,s+1);
	if (islower(*c))
	    *c = toupper(*c);		/* gack and double gack */
    }
    realName = savestr(buf);
#else /* !BERKNAMES */
    if ((c = index(s, '(')) != NULL)
	*c = '\0';
    if ((c = index(s, '-')) != NULL)
	s = c;
    realName = savestr(s);
#endif /* !BERKNAMES */
#else /* !PASSNAMES */
    {
	FILE* fp;
	env_init2(); /* Make sure homedir/dotdir/etc. are set. */
	if ((fp = fopen(filexp(FULLNAMEFILE),"r")) != NULL) {
	    fgets(buf,sizeof buf,fp);
	    fclose(fp);
	    buf[strlen(buf)-1] = '\0';
	    realName = savestr(buf);
	}
	else
	    s = "PUT_YOUR_NAME_HERE";
    }
#endif /* !PASSNAMES */
#ifdef HAS_GETPWENT
    endpwent();
#endif
    return 1;
}

bool
setphostname(tmpbuf)
char* tmpbuf;
{
    FILE* fp;
    bool hostname_ok = TRUE;

    /* Find the local hostname */

#ifdef HAS_GETHOSTNAME
    gethostname(tmpbuf,TCBUF_SIZE);
#else
# ifdef HAS_UNAME
    /* get sysname */
    uname(&utsn);
    strcpy(tmpbuf,utsn.nodename);
# else
#  ifdef PHOSTCMD
    {
	FILE* popen();
	FILE* pipefp = popen(PHOSTCMD,"r");
	
	if (pipefp == NULL) {
	    printf("Can't find hostname\n");
	    finalize(1);
	}
	fgets(tmpbuf,TCBUF_SIZE,pipefp);
	tmpbuf[strlen(tmpbuf)-1] = '\0';	/* wipe out newline */
	pclose(pipefp);
    }
#  else
    strcpy(tmpbuf, "!INVALID!");
#  endif /* PHOSTCMD */
# endif /* HAS_UNAME */
#endif /* HAS_GETHOSTNAME */
    localhost = savestr(tmpbuf);

    /* Build the host name that goes in postings */

    phostname = PHOSTNAME;
    if (FILE_REF(phostname) || *phostname == '~') {
	phostname = filexp(phostname);
	if ((fp = fopen(phostname,"r")) == NULL)
	    strcpy(tmpbuf,".");
	else {
	    fgets(tmpbuf,TCBUF_SIZE,fp);
	    fclose(fp);
	    phostname = tmpbuf + strlen(tmpbuf) - 1;
	    if (*phostname == '\n')
		*phostname = '\0';
	}
    }
    else
	strcpy(tmpbuf,phostname);

    if (*tmpbuf == '.') {
	if (tmpbuf[1] != '\0')
	    strcpy(buf,tmpbuf);
	else
	    *buf = '\0';
	strcpy(tmpbuf,localhost);
	strcat(tmpbuf,buf);
    }

    if (!index(tmpbuf,'.')) {
	if (*tmpbuf)
	    strcat(tmpbuf, ".");
#ifdef HAS_RES_INIT
	if (!(_res.options & RES_INIT))
	    res_init();
	if (_res.defdname != NULL)
	    strcat(tmpbuf,_res.defdname);
	else
#endif
#ifdef HAS_GETDOMAINNAME
	if (getdomainname(buf,LBUFLEN) == 0)
	    strcat(tmpbuf,buf);
	else
#endif
	{
	    strcat(tmpbuf,"UNKNOWN.HOST");
	    hostname_ok = FALSE;
	}
    }
    phostname = savestr(tmpbuf);
    return hostname_ok;
}

char*
getval(nam,def)
char* nam;
char* def;
{
    char* val;

    if ((val = getenv(nam)) == NULL || !*val)
	return def;
    return val;
}

static bool firstexport = TRUE;
extern char** environ;

char*
export(nam,val)
char* nam;
char* val;
{
    int namlen = strlen(nam);
    register int i=envix(nam,namlen);	/* where does it go? */

    if (!environ[i]) {			/* does not exist yet */
	if (firstexport) {		/* need we copy environment? */
	    int j;
#ifndef lint
	    char** tmpenv = (char**)	/* point our wand at memory */
		safemalloc((MEM_SIZE) (i+2) * sizeof(char*));
#else
	    char** tmpenv = NULL;
#endif /* lint */
    
	    firstexport = FALSE;
	    for (j = 0; j < i; j++)	/* copy environment */
		tmpenv[j] = environ[j];
	    environ = tmpenv;		/* tell exec where it is now */
	}
#ifndef lint
	else
	    environ = (char**) saferealloc((char*) environ,
		(MEM_SIZE) (i+2) * sizeof(char*));
					/* just expand it a bit */
#endif /* lint */
	environ[i+1] = NULL;	/* make sure it's null terminated */
    }
    environ[i] = safemalloc((MEM_SIZE)(namlen + strlen(val) + 2));
					/* this may or may not be in */
					/* the old environ structure */
    sprintf(environ[i],"%s=%s",nam,val);/* all that work just for this */
    return environ[i] + namlen + 1;
}

void
un_export(export_val)
char* export_val;
{
    if (export_val[-1] == '=' && export_val[-2] != '_') {
	export_val[0] = export_val[-2];
	export_val[1] = '\0';
	export_val[-2] = '_';
    }
}

void
re_export(export_val, new_val, limit)
char* export_val;
char* new_val;
int limit;
{
    if (export_val[-1] == '=' && export_val[-2] == '_' && !export_val[1])
	export_val[-2] = export_val[0];
    safecpy(export_val, new_val, limit+1);
}

static int
envix(nam, len)
char* nam;
int len;
{
    register int i;

    for (i = 0; environ[i]; i++) {
	if (strnEQ(environ[i],nam,len) && environ[i][len] == '=')
	    break;			/* strnEQ must come first to avoid */
    }					/* potential SEGV's */
    return i;
}

#ifdef MSDOS

char*
GetEnv(var)
char* var;
{
#undef getenv
    char* s = getenv(var);
    if (s && isalpha(*s) && s[1] == ':') {
	char* t = index(s,'\\');
	if (t) {
	    char ebuf[MAXDIR+32];
	    strcpy(ebuf,s);
	    t = ebuf + (t-s);
	    do {
		*t = '/';
	    } while ((t = index(t,'\\')) != NULL);
	    s = export(var,ebuf);
	}
    }
    return s;
}
#endif
