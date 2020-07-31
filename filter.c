/* filter.c -- external kill/score processing
 */
/* This software is copyrighted as detailed in the LICENSE file. */


#include "EXTERN.h"
#include "common.h"

#ifdef USE_FILTER

#include "ngdata.h"
#include "util2.h"
#include "hash.h"
#include "cache.h"
#include "list.h"
#include "env.h"
#include "trn.h"
#include "final.h"
#include "rt-ov.h"
#include "nntpclient.h"
#include "datasrc.h"
#include "INTERN.h"
#include "filter.h"
#include "filter.ih"

static FILE* filt_wr;
static FILE* filt_rd;
static int pipe1[2], pipe2[2];
static int filter_restarting;
static int need_to_filter = 0;
static int want_filter = 1;	/* consider making an option later */
static pid_t filter_pid = 0;

#ifdef FILTER_DEBUG
extern FILE* filter_error_file;
#endif

void
filter_init()
{
    filt_wr = filt_rd = NULL;
    pipe1[0] = pipe1[1] = pipe2[0] = pipe2[1] = -1;
    filter_restarting = 0;
}

static void
filter_restart()
{
    char* filter;				/* external filter file */
    struct stat f_inode;			/* stat buf */
    static int recursing = 0;

    /* prevent unbounded recursion (e.g. filter_restart -> filter_nginit ->
       filter_send -> filter_restart) */

    if (recursing)
	return;
    recursing = 1;

    filter = filexp(getval("FILTER", FILTERPROG));

    want_filter = (strNE(filter,"NOFILTER") && stat(filter, &f_inode) == 0);
    if (!want_filter) {
	recursing = 0;
	return;
    }

    if (!(f_inode.st_mode & S_IXUSR)) {
	perror("opening filter");
	recursing = 0;
	return;
    }

    if (verbose) {
	printf("\nStarting filter `%s'", filter);
	fflush(stdout);		/* print it *now* */
    }
    if (pipe(pipe1) < 0)
	perror("creating output pipe");
    if (pipe(pipe2) < 0)
	perror("creating input pipe");
    if ((filter_pid = fork()) < 0)
	perror("filter_restart");
    else if (filter_pid > 0) {
	if ((filt_wr = fdopen(pipe1[1], "w")) == NULL) {
	    perror("configuring output pipe");
	    filter_cleanup();
	    return;
	}
	if ((filt_rd = fdopen(pipe2[0], "r")) == NULL) {
	    perror("configuring input pipe");
	    filter_cleanup();
	    return;
	}
	setvbuf(filt_wr, NULL, _IOLBF, BUFSIZ);	/* line buffering */
	setvbuf(filt_rd, NULL, _IOLBF, BUFSIZ);
	close(pipe1[0]);
	close(pipe2[1]);
    }
    else {
	close(STDIN_FILENO);
	close(STDOUT_FILENO);

	sigignore(SIGINT);

	if (dup2(pipe1[0], STDIN_FILENO) < 0) {
	    perror("reopening child input stream");
	    finalize(1);
	}

	if (dup2(pipe2[1], STDOUT_FILENO) < 0) {
	    perror("reopening child output stream");
	    finalize(1);
	}

	close(pipe1[1]);
	close(pipe2[0]);
	if (execl(filter, filter, NULL) < 0) {
	    perror("switching to filter process");
	    finalize(1);
	}
    }

    filter_nginit();

    recursing = 0;
}

void
filter_nginit()
{
    static int recursing = 0;
    char buf[1024];

    if (recursing)
	return;

    need_to_filter = 0;

    if (!want_filter)
	return;

    recursing = 1;

    sprintf(buf, "newsgroup %s\n", ngname);
    if (filter_send(buf) == 0) {
	if (filter_recv(buf) != NULL) {
	    Uchar* fieldflags = datasrc->fieldflags;
	    int i;
	    if (strncaseEQ(buf, "overview", 8)) {
		for (i = 1; i < OV_MAX_FIELDS; i++) {
		    if (fieldflags[i] & (FF_HAS_FIELD | FF_CHECK4FIELD))
			fieldflags[i] |= FF_FILTERSEND;
		}
		need_to_filter = 1;
	    }
    	    else if (strncaseEQ(buf, "want", 4)) {
		char* s = buf + strlen(buf) - 1;
		if (*s == '\n')
		    *s = '\0';
		s = buf + 4;
		for (i = 1; i < OV_MAX_FIELDS; i++)
		    fieldflags[i] &= ~FF_FILTERSEND;
		for (;;) {
		    while (*s == ' ') s++;
		    if (!*s)
			break;
		    s = cpytill(buf,s,' ');
		    i = ov_num(buf,(char*)NULL);
		    if (i) {
			fieldflags[i] |= FF_FILTERSEND;
			need_to_filter = 1;
		    }
		}
	    }
	}
    }

    recursing = 0;
}

int
filter(a)
ART_NUM a;
{
    char buf[256];
    long score = 0, sc = 0;
    ARTICLE* ap;
    ART_NUM i;
    char* s;

    if (!need_to_filter)
	return 0;

    ap = article_find(a);
    if (ap && ap->refs) {
	Uchar* fieldflags = datasrc->fieldflags;
	sprintf(buf, "art %ld", ap->num);
	if (filter_send(buf) < 0)
	    return 0;
	for (i = 1; i < OV_MAX_FIELDS; i++) {
	    if (filter_send("\t") < 0)
		return 0;
	    if (fieldflags[i] & FF_FILTERSEND) {
		if (fieldflags[i] & FF_HAS_HDR) {
		    if (filter_send(ov_fieldname(i)) < 0
		     || filter_send(": ") < 0)
			return 0;
		}
		if ((s = ov_field(ap, i)) != NULL) {
		    if (filter_send(s) < 0)
			return 0;
		}
	    }
	}
	if (filter_send("\n") < 0 || filter_send("scores\n") < 0)
	    return 0;
	while (filter_recv(buf) != NULL && strncaseNE(buf, "done", 4)) {
	    sscanf(buf, "%ld %ld", &i, &sc);
	    if (i == a)
		score = sc;
#ifdef FILTER_DEBUG
	    else {
		fprintf(filter_error_file, "article %d: filter returned %s",
			a, buf);
	    }
#endif
	}
    }

    return score;
}

static int
filter_send(cmd)
char* cmd;
{
    if (filt_wr == NULL || fputs(cmd, filt_wr) == EOF) {
	filter_restart();
	if (filt_wr == NULL) {
#ifdef FILTER_DEBUG
	    fprintf (stderr, "filt_wr is still NULL\n");
#endif
	    return -1;
	}
	else if (fputs (cmd, filt_wr) == EOF) {
	    fprintf(stderr, "Could not restart filter process.\n");
	    return -1;
	}
    }

    return 0;
}

static char*
filter_recv(buf)
char* buf;
{
    if (filt_rd == NULL)
	return NULL;

    return (fgets(buf, 256, filt_rd));
}

void
filter_cleanup()
{
    if (filter_pid > 0) {
        kill(filter_pid, SIGTERM);
	filter_pid = 0;
    }

    if (filt_wr != NULL) {
	fputs("bye\n", filt_wr);
	fclose(filt_wr);
	filt_wr = NULL;
    }

    if (filt_rd != NULL) {
	fclose(filt_rd);
	filt_rd = NULL;
    }

    pipe1[0] = pipe1[1] = 0;
    pipe2[0] = pipe2[1] = 0;
}

#endif /* USE_FILTER */
