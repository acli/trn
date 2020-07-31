/* inews.c
 */
/* This software is copyrighted as detailed in the LICENSE file. */


#include "EXTERN.h"
#include "common.h"
#include "env.h"
#include "init.h"
#include "util2.h"
#include "util3.h"
#include "nntpclient.h"
#include "nntpinit.h"

#define	MAX_SIGNATURE	4

int	debug = 0;
int	new_connection = FALSE;
char*	server_name;
char*	nntp_auth_file;
char	nullstr[1];

char	buf[LBUFLEN+1];

int valid_header _((char*));
void append_signature _((void));

int
main(argc, argv)
int argc;
char* argv[];
{
    bool has_fromline, in_header, has_pathline;
    bool found_nl, had_nl;
    int artpos, headbuf_size, len;
    char* headbuf;
    char* line_end;
    register char* cp;
    int i;

    headbuf_size = LBUFLEN * 8;
    headbuf = safemalloc(headbuf_size);

#ifdef LAX_INEWS
    env_init(headbuf, 1);
#else
    if (!env_init(headbuf, 0)) {
	fprintf(stderr,"Can't get %s information. Please contact your system adminstrator.\n",
		(*loginName || !*realName)? "user" : "host");
	exit(1);
    }
#endif

    argv++;
    while (argc > 1) {
	if (*argv[0] != '-')
	    break;
	argv++;
	argc--;
    }
    if (argc > 1) {
	if (freopen(*argv, "r", stdin) == NULL) {
	    perror(*argv);
	    exit(1);
	}
    }

    cp = getenv("NNTPSERVER");
    if (!cp) {
	cp = filexp(SERVER_NAME);
	if (FILE_REF(cp))
	    cp = nntp_servername(cp);
    }
    if (cp && *cp && strNE(cp,"local")) {
	server_name = savestr(cp);
	cp = index(server_name, ';');
	if (cp) {
	    *cp = '\0';
	    nntplink.port_number = atoi(cp+1);
	}
	line_end = "\r\n";
	nntp_auth_file = filexp(NNTP_AUTH_FILE);
	if ((cp = getenv("NNTP_FORCE_AUTH")) != NULL
	 && (*cp == 'y' || *cp == 'Y'))
	    nntplink.flags |= NNTP_FORCE_AUTH_NEEDED;
    } else {
	server_name = NULL;
	line_end = "\n";
    }

    in_header = 0;
    has_fromline = 0;
    has_pathline = 0;
    artpos = 0;
    cp = headbuf;
    had_nl = 1;

    for (;;) {
	if (headbuf_size < artpos + LBUFLEN + 1) {
	    len = cp - headbuf;
	    headbuf_size += LBUFLEN * 4;
	    headbuf = saferealloc(headbuf,headbuf_size);
	    cp = headbuf + len;
	}
	i = getc(stdin);
	if (server_name && had_nl && i == '.')
	    *cp++ = '.';

	if (i == '\n') {
	    if (!in_header)
		continue;
	    break;
	}
	else if (i == EOF || !fgets(cp+1, LBUFLEN-1, stdin)) {
	    /* Still in header after EOF?  Hmm... */
	    fprintf(stderr,"Article was all header -- no body.\n");
	    exit(1);
	}
	*cp = (char)i;
	len = strlen(cp);
	found_nl = (len && cp[len-1] == '\n');
	if (had_nl) {
	    if ((i = valid_header(cp)) == 0) {
		fprintf(stderr,"Invalid header:\n%s",cp);
		exit(1);
	    }
	    if (i == 2) {
		if (!in_header)
		    continue;
		break;
	    }
	    in_header = 1;
	    if (strncaseEQ(cp, "From:", 5))
		has_fromline = 1;
	    else if (strncaseEQ(cp, "Path:", 5))
		has_pathline = 1;
	}
	artpos += len;
	cp += len;
	if ((had_nl = found_nl) != 0 && server_name) {
	    cp[-1] = '\r';
	    *cp++ = '\n';
	}
    }
    *cp = '\0';

    /* Well, the header looks ok, so let's get on with it. */

    if (server_name) {
	if ((cp = getenv("NNTPFDS")) != NULL) {
	    int rd_fd, wr_fd;
	    if (sscanf(cp,"%d.%d",&rd_fd,&wr_fd) == 2) {
		nntplink.rd_fp = fdopen(rd_fd, "r");
		if (nntplink.rd_fp) {
		    nntplink.wr_fp = fdopen(wr_fd, "w");
		    if (nntplink.wr_fp)
			nntplink.flags |= NNTP_NEW_CMD_OK;
		    else
			nntp_close(FALSE);
		}
	    }
	}
	if (!nntplink.wr_fp) {
	    if (init_nntp() < 0 || !nntp_connect(server_name,0))
		exit(1);
	    new_connection = TRUE;
	}
	if (nntp_command("POST") <= 0 || nntp_check() <= 0) {
	    if (new_connection)
		nntp_close(TRUE);
	    fprintf(stderr,"Sorry, you can't post from this machine.\n");
	    exit(1);
	}
    }
    else {
	sprintf(buf, "%s -h", EXTRAINEWS);
	nntplink.wr_fp = popen(buf,"w");
	if (!nntplink.wr_fp) {
	    fprintf(stderr,"Unable to execute inews for local posting.\n");
	    exit(1);
	}
    }

    fputs(headbuf, nntplink.wr_fp);
    if (!has_pathline)
	fprintf(nntplink.wr_fp,"Path: not-for-mail%s",line_end);
    if (!has_fromline) {
	fprintf(nntplink.wr_fp,"From: %s@%s (%s)%s",loginName,phostname,
		getval("NAME",realName),line_end);
    }
    if (!getenv("NO_ORIGINATOR")) {
	fprintf(nntplink.wr_fp,"Originator: %s@%s (%s)%s",loginName,phostname,
		getval("NAME",realName),line_end);
    }
    fprintf(nntplink.wr_fp,"%s",line_end);

    had_nl = 1;
    while (fgets(headbuf, headbuf_size, stdin)) {
	/* Single . is eof, so put in extra one */
	if (server_name && had_nl && *headbuf == '.')
	    fputc('.', nntplink.wr_fp);
	/* check on newline */
	cp = headbuf + strlen(headbuf);
	if (cp > headbuf && *--cp == '\n') {
	    *cp = '\0';
	    fprintf(nntplink.wr_fp, "%s%s", headbuf, line_end);
	    had_nl = 1;
	}
	else {
	    fputs(headbuf, nntplink.wr_fp);
	    had_nl = 0;
	}
    }

    if (!server_name)
	return pclose(nntplink.wr_fp);

    if (!had_nl)
        fputs(line_end, nntplink.wr_fp);

    append_signature();

    fputs(".\r\n",nntplink.wr_fp);
    (void) fflush(nntplink.wr_fp);

    if (nntp_gets(ser_line, sizeof ser_line) < 0
     || *ser_line != NNTP_CLASS_OK) {
	if (atoi(ser_line) == NNTP_POSTFAIL_VAL) {
	    fprintf(stderr,"Article not accepted by server; not posted:\n");
	    for (cp = ser_line + 4; *cp && *cp != '\r'; cp++) {
		if (*cp == '\\')
		    fputc('\n',stderr);
		else
		    fputc(*cp,stderr);
	    }
	    fputc('\n', stderr);
	}
	else
	    fprintf(stderr, "Remote error: %s\n", ser_line);
	if (new_connection)
	    nntp_close(TRUE);
	exit(1);
    }

    if (new_connection)
	nntp_close(TRUE);
    cleanup_nntp();

    return 0;
}

/* valid_header -- determine if a line is a valid header line */
int
valid_header(h)
register char* h;
{
    char* colon;
    char* space;

    /* Blank or tab in first position implies this is a continuation header */
    if (h[0] == ' ' || h[0] == '\t') {
	while (*++h == ' ' || *h == '\t') ;
	return *h && *h != '\n'? 1 : 2;
    }

    /* Just check for initial letter, colon, and space to make
     * sure we discard only invalid headers. */
    colon = index(h, ':');
    space = index(h, ' ');
    if (isalpha(h[0]) && colon && space == colon + 1)
	return 1;

    /* Anything else is a bad header */
    return 0;
}

/* append_signature -- append the person's .signature file if
 * they have one.  Limit .signature to MAX_SIGNATURE lines.
 * The rn-style DOTDIR environmental variable is used if present.
 */
void
append_signature()
{
    char* cp;
    FILE* fp;
    int count = 0;

#ifdef NO_INEWS_DOTDIR
    dotdir = homedir;
#endif
    if (dotdir == NULL)
	return;

    fp = fopen(filexp(SIGNATURE_FILE), "r");
    if (fp == NULL)
	return;

    fprintf(nntplink.wr_fp, "-- \r\n");
    while (fgets(ser_line, sizeof ser_line, fp)) {
	count++;
	if (count > MAX_SIGNATURE) {
	    fprintf(stderr,"Warning: .signature files should be no longer than %d lines.\n",
		    MAX_SIGNATURE);
	    fprintf(stderr,"(Only %d lines of your .signature were posted.)\n",
		    MAX_SIGNATURE);
	    break;
	}
	/* Strip trailing newline */
	cp = ser_line + strlen(ser_line) - 1;
	if (cp >= ser_line && *cp == '\n')
	    *cp = '\0';
	fprintf(nntplink.wr_fp, "%s\r\n", ser_line);
    }
    (void) fclose(fp);
}

#ifdef SUPPORT_NNTP
int
nntp_handle_timeout()
{
    if (!new_connection) {
	static bool handling_timeout = FALSE;
	char last_command_save[NNTP_STRLEN];

	if (strcaseEQ(last_command,"quit"))
	    return 0;
	if (handling_timeout)
	    return -1;
	handling_timeout = TRUE;
	strcpy(last_command_save, last_command);
	nntp_close(FALSE);
	if (init_nntp() < 0 || nntp_connect(server_name,0) <= 0)
	    exit(1);
	if (nntp_command(last_command_save) <= 0)
	    return -1;
	handling_timeout = FALSE;
	new_connection = TRUE;
	return 1;
    }
    fputs("\n503 Server timed out.\n",stderr);
    return -2;
}
#endif
